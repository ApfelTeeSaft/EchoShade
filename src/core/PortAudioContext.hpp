#pragma once
#include <portaudio.h>
#include <stdexcept>
#include <string>

namespace echoshade {

/// Singleton RAII wrapper for Pa_Initialize / Pa_Terminate.
/// Call PortAudioContext::require() before any PortAudio API call.
/// The context is initialised once and terminated on process exit.
class PortAudioContext {
public:
    static void require() {
        static PortAudioContext instance;
        (void)instance;
    }

    PortAudioContext(const PortAudioContext&)            = delete;
    PortAudioContext& operator=(const PortAudioContext&) = delete;

private:
    PortAudioContext() {
        PaError err = Pa_Initialize();
        if (err != paNoError)
            throw std::runtime_error(
                std::string("PortAudio initialisation failed: ") + Pa_GetErrorText(err));
    }
    ~PortAudioContext() {
        Pa_Terminate();
    }
};

[[noreturn]] inline void throwPaError(const char* context, PaError err) {
    throw std::runtime_error(
        std::string(context) + ": " + Pa_GetErrorText(err));
}

} // namespace echoshade