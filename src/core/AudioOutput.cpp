#include "echoshade/AudioOutput.hpp"
#include <portaudio.h>
#include <stdexcept>
#include <cstring>

namespace echoshade {

struct PAOutputState {
    AudioOutput::Callback* callback;
    int channels;
};

static int paOutputCb(const void* /*in*/, void* outBuf,
                      unsigned long frames,
                      const PaStreamCallbackTimeInfo*,
                      PaStreamCallbackFlags,
                      void* userData) {
    auto* state = static_cast<PAOutputState*>(userData);
    float* out  = static_cast<float*>(outBuf);
    if (state->callback && *state->callback)
        (*state->callback)(out, static_cast<int>(frames), state->channels);
    else
        std::memset(out, 0, sizeof(float) * frames * static_cast<std::size_t>(state->channels));
    return paContinue;
}

struct AudioOutput::Impl {
    PaStream*      stream = nullptr;
    PAOutputState* state  = nullptr;
};

AudioOutput::AudioOutput(const AudioOutputConfig& cfg) : cfg_(cfg) {
    static bool paInit = false;
    if (!paInit) {
        PaError err = Pa_Initialize();
        if (err != paNoError)
            throw std::runtime_error(std::string("PortAudio: ") + Pa_GetErrorText(err));
        paInit = true;
    }
    impl_ = new Impl{};
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

    int devIdx = cfg_.deviceIndex < 0
        ? Pa_GetDefaultOutputDevice()
        : cfg_.deviceIndex;

    impl_->state = new PAOutputState{&callback_, cfg_.channels};

    PaStreamParameters params{};
    params.device                    = devIdx;
    params.channelCount              = cfg_.channels;
    params.sampleFormat              = paFloat32;
    params.suggestedLatency          =
        Pa_GetDeviceInfo(devIdx)->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&impl_->stream, nullptr, &params,
                                cfg_.sampleRate,
                                static_cast<unsigned long>(cfg_.framesPerBuffer),
                                paNoFlag, paOutputCb, impl_->state);
    if (err != paNoError) {
        delete impl_->state;
        impl_->state  = nullptr;
        impl_->stream = nullptr;
        return false;
    }
    Pa_StartStream(impl_->stream);
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
