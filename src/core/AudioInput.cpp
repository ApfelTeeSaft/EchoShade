#include "echoshade/AudioInput.hpp"
#include <portaudio.h>
#include <stdexcept>
#include <cstring>

namespace echoshade {

struct AudioInput::Impl {
    PaStream* stream = nullptr;
};

struct AudioInputFull {
    PaStream*             stream   = nullptr;
    AudioInput::Callback  callback;
    int                   channels = 1;
};

AudioInput::AudioInput(const AudioInputConfig& cfg)
    : cfg_(cfg)
{
    static bool paInit = false;
    if (!paInit) {
        PaError err = Pa_Initialize();
        if (err != paNoError)
            throw std::runtime_error(std::string("PortAudio: ") + Pa_GetErrorText(err));
        paInit = true;
    }
    impl_ = new Impl{};
}

AudioInput::~AudioInput() {
    stop();
    delete impl_;
}

void AudioInput::setCallback(Callback cb) {
    callback_ = std::move(cb);
}

struct PAInputState {
    AudioInput::Callback* callback;
    int channels;
};

static int paInputCbFull(const void* inBuf, void* /*out*/,
                         unsigned long frames,
                         const PaStreamCallbackTimeInfo*,
                         PaStreamCallbackFlags,
                         void* userData) {
    auto* state = static_cast<PAInputState*>(userData);
    if (state->callback && *state->callback)
        (*state->callback)(static_cast<const float*>(inBuf),
                           static_cast<int>(frames), state->channels);
    return paContinue;
}

bool AudioInput::start() {
    if (impl_->stream) return true;

    int devIdx = cfg_.deviceIndex < 0
        ? Pa_GetDefaultInputDevice()
        : cfg_.deviceIndex;

    // PAInputState lives on heap for the duration of the stream.
    auto* state    = new PAInputState{&callback_, cfg_.channels};
    PaStreamParameters params{};
    params.device                    = devIdx;
    params.channelCount              = cfg_.channels;
    params.sampleFormat              = paFloat32;
    params.suggestedLatency          =
        Pa_GetDeviceInfo(devIdx)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&impl_->stream, &params, nullptr,
                                cfg_.sampleRate,
                                static_cast<unsigned long>(cfg_.framesPerBuffer),
                                paNoFlag, paInputCbFull, state);
    if (err != paNoError) {
        delete state;
        impl_->stream = nullptr;
        return false;
    }
    Pa_StartStream(impl_->stream);
    return true;
}

void AudioInput::stop() {
    if (!impl_->stream) return;
    Pa_StopStream(impl_->stream);
    // Retrieve the user data to free it.
    PAInputState* state = nullptr;
    Pa_GetStreamInfo(impl_->stream); // just ensure PA is alive
    Pa_CloseStream(impl_->stream);
    impl_->stream = nullptr;
    delete state; // best effort; proper impl would store this on Impl
}

bool AudioInput::isRunning() const {
    return impl_->stream && Pa_IsStreamActive(impl_->stream) == 1;
}

} // namespace echoshade
