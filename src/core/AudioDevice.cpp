#include "echoshade/AudioDevice.hpp"
#include <portaudio.h>
#include <stdexcept>
#include <cstring>

namespace echoshade {

namespace {

void ensurePA() {
    static bool initialized = false;
    if (!initialized) {
        PaError err = Pa_Initialize();
        if (err != paNoError)
            throw std::runtime_error(std::string("PortAudio init failed: ") + Pa_GetErrorText(err));
        initialized = true;
    }
}

AudioDeviceInfo fromPa(int idx, const PaDeviceInfo* info, bool defIn, bool defOut) {
    AudioDeviceInfo d;
    d.index               = idx;
    d.name                = info->name ? info->name : "(unknown)";
    d.maxInputChannels    = info->maxInputChannels;
    d.maxOutputChannels   = info->maxOutputChannels;
    d.defaultSampleRate   = info->defaultSampleRate;
    d.isDefaultInput      = defIn;
    d.isDefaultOutput     = defOut;
    return d;
}

} // namespace

std::vector<AudioDeviceInfo> AudioDevice::enumerateInputs() {
    ensurePA();
    std::vector<AudioDeviceInfo> result;
    const int defIn  = Pa_GetDefaultInputDevice();
    const int count  = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0)
            result.push_back(fromPa(i, info, i == defIn, false));
    }
    return result;
}

std::vector<AudioDeviceInfo> AudioDevice::enumerateOutputs() {
    ensurePA();
    std::vector<AudioDeviceInfo> result;
    const int defOut = Pa_GetDefaultOutputDevice();
    const int count  = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxOutputChannels > 0)
            result.push_back(fromPa(i, info, false, i == defOut));
    }
    return result;
}

AudioDeviceInfo AudioDevice::defaultInput() {
    ensurePA();
    const int idx  = Pa_GetDefaultInputDevice();
    const auto* i  = Pa_GetDeviceInfo(idx);
    return i ? fromPa(idx, i, true, false) : AudioDeviceInfo{};
}

AudioDeviceInfo AudioDevice::defaultOutput() {
    ensurePA();
    const int idx  = Pa_GetDefaultOutputDevice();
    const auto* i  = Pa_GetDeviceInfo(idx);
    return i ? fromPa(idx, i, false, true) : AudioDeviceInfo{};
}

} // namespace echoshade
