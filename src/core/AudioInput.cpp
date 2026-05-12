#include "echoshade/AudioInput.hpp"
#include "PortAudioContext.hpp"
#include <portaudio.h>
#include <cstring>

namespace echoshade {

// Per-stream state stored on the heap and passed as PortAudio user data.
// Lifetime: created in start(), destroyed in stop().
struct PAInputState {
    AudioInput::Callback* callback = nullptr;
    int                   channels = 1;
};

static int paInputCallback(const void*                     inBuf,
                            void*                          /* outBuf */,
                            unsigned long                  frameCount,
                            const PaStreamCallbackTimeInfo* /* time */,
                            PaStreamCallbackFlags          /* flags */,
                            void*                          userData) noexcept {
    const auto* st  = static_cast<const PAInputState*>(userData);
    const auto& cb  = *st->callback;
    if (cb)
        cb(static_cast<const float*>(inBuf),
           static_cast<int>(frameCount),
           st->channels);
    return paContinue;
}

struct AudioInput::Impl {
    PaStream*     stream = nullptr;
    PAInputState* state  = nullptr;   // non-null only while stream is open
};

AudioInput::AudioInput(const AudioInputConfig& cfg)
    : cfg_(cfg)
    , impl_(new Impl{})
{
    PortAudioContext::require();
}

AudioInput::~AudioInput() {
    stop();
    delete impl_;
}

void AudioInput::setCallback(Callback cb) {
    callback_ = std::move(cb);
}

bool AudioInput::start() {
    if (impl_->stream) return true;

    const int devIdx = (cfg_.deviceIndex < 0)
        ? Pa_GetDefaultInputDevice()
        : cfg_.deviceIndex;

    if (devIdx == paNoDevice) return false;

    PaStreamParameters params{};
    params.device                    = devIdx;
    params.channelCount              = cfg_.channels;
    params.sampleFormat              = paFloat32;
    params.suggestedLatency          =
        Pa_GetDeviceInfo(devIdx)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    impl_->state = new PAInputState{&callback_, cfg_.channels};

    PaError err = Pa_OpenStream(
        &impl_->stream,
        &params,
        nullptr,   // output params (none)
        cfg_.sampleRate,
        static_cast<unsigned long>(cfg_.framesPerBuffer),
        paNoFlag,
        paInputCallback,
        impl_->state
    );

    err = Pa_StartStream(impl_->stream);
    if (err != paNoError) {
        Pa_CloseStream(impl_->stream);
        delete impl_->state;
        impl_->state  = nullptr;
        impl_->stream = nullptr;
        return false;
    }

    return true;
}

void AudioInput::stop() {
    if (!impl_->stream) return;

    Pa_StopStream(impl_->stream);
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
    delete impl_->state;
    impl_->state = nullptr;
}

bool AudioInput::isRunning() const {
    return impl_->stream && Pa_IsStreamActive(impl_->stream) == 1;
}

} // namespace echoshade
