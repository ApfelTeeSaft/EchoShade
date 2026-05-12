#include "echoshade/AudioOutput.hpp"
#include "PortAudioContext.hpp"
#include <portaudio.h>
#include <cstring>

namespace echoshade {

struct PAOutputState {
    AudioOutput::Callback* callback = nullptr;
    int                    channels = 1;
};

static int paOutputCallback(const void*                     /* inBuf */,
                             void*                          outBuf,
                             unsigned long                  frameCount,
                             const PaStreamCallbackTimeInfo* /* time */,
                             PaStreamCallbackFlags          /* flags */,
                             void*                          userData) noexcept {
    const auto* st = static_cast<const PAOutputState*>(userData);
    auto* out      = static_cast<float*>(outBuf);
    const auto& cb = *st->callback;
    if (cb) {
        cb(out, static_cast<int>(frameCount), st->channels);
    } else {
        std::memset(out, 0, sizeof(float)
                    * static_cast<std::size_t>(frameCount)
                    * static_cast<std::size_t>(st->channels));
    }
    return paContinue;
}

struct AudioOutput::Impl {
    PaStream*      stream = nullptr;
    PAOutputState* state  = nullptr;
};

AudioOutput::AudioOutput(const AudioOutputConfig& cfg)
    : cfg_(cfg)
    , impl_(new Impl{})
{
    PortAudioContext::require();
}

AudioOutput::~AudioOutput() {
    stop();
    delete impl_;
}

void AudioOutput::setCallback(Callback cb) {
    callback_ = std::move(cb);
}

bool AudioOutput::start() {
    if (impl_->stream) return true;

    const int devIdx = (cfg_.deviceIndex < 0)
        ? Pa_GetDefaultOutputDevice()
        : cfg_.deviceIndex;

    if (devIdx == paNoDevice) return false;

    PaStreamParameters params{};
    params.device                    = devIdx;
    params.channelCount              = cfg_.channels;
    params.sampleFormat              = paFloat32;
    params.suggestedLatency          =
        Pa_GetDeviceInfo(devIdx)->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    impl_->state = new PAOutputState{&callback_, cfg_.channels};

    PaError err = Pa_OpenStream(
        &impl_->stream,
        nullptr,   // input params (none)
        &params,
        cfg_.sampleRate,
        static_cast<unsigned long>(cfg_.framesPerBuffer),
        paNoFlag,
        paOutputCallback,
        impl_->state
    );

    if (err != paNoError) {
        delete impl_->state;
        impl_->state  = nullptr;
        impl_->stream = nullptr;
        return false;
    }

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

void AudioOutput::stop() {
    if (!impl_->stream) return;

    Pa_StopStream(impl_->stream);
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
    
    delete impl_->state;
    impl_->state = nullptr;
}

bool AudioOutput::isRunning() const {
    return impl_->stream && Pa_IsStreamActive(impl_->stream) == 1;
}

} // namespace echoshade
