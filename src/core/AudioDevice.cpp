#include "echoshade/AudioDevice.hpp"
#include "PortAudioContext.hpp"
#include <portaudio.h>

namespace echoshade {

namespace {

AudioDeviceInfo fromPaInfo(int idx, const PaDeviceInfo* info,
                            bool defIn, bool defOut) {
    AudioDeviceInfo d;
    d.index             = idx;
    d.name              = info->name ? info->name : "(unknown)";
    d.maxInputChannels  = info->maxInputChannels;
    d.maxOutputChannels = info->maxOutputChannels;
    d.defaultSampleRate = info->defaultSampleRate;
    d.defaultLowInputLatencyMs  = info->defaultLowInputLatency  * 1000.0;
    d.defaultLowOutputLatencyMs = info->defaultLowOutputLatency * 1000.0;
    d.isDefaultInput    = defIn;
    d.isDefaultOutput   = defOut;
    return d;
}

void ensureInit() { PortAudioContext::require(); }

} // namespace

bool AudioDevice::isAvailable() noexcept {
    try {
        PortAudioContext::require();
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<AudioDeviceInfo> AudioDevice::enumerateInputs() {
    ensureInit();
    const int defIn  = Pa_GetDefaultInputDevice();
    const int count  = Pa_GetDeviceCount();
    std::vector<AudioDeviceInfo> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0)
            result.push_back(fromPaInfo(i, info, i == defIn, false));
    }
    return result;
}

std::vector<AudioDeviceInfo> AudioDevice::enumerateOutputs() {
    ensureInit();
    const int defOut = Pa_GetDefaultOutputDevice();
    const int count  = Pa_GetDeviceCount();
    std::vector<AudioDeviceInfo> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxOutputChannels > 0)
            result.push_back(fromPaInfo(i, info, false, i == defOut));
    }
    return result;
}

std::optional<AudioDeviceInfo> AudioDevice::defaultInput() {
    ensureInit();
    const int idx = Pa_GetDefaultInputDevice();
    if (idx == paNoDevice) return std::nullopt;
    const PaDeviceInfo* info = Pa_GetDeviceInfo(idx);
    if (!info) return std::nullopt;
    return fromPaInfo(idx, info, true, false);
}

std::optional<AudioDeviceInfo> AudioDevice::defaultOutput() {
    ensureInit();
    const int idx = Pa_GetDefaultOutputDevice();
    if (idx == paNoDevice) return std::nullopt;
    const PaDeviceInfo* info = Pa_GetDeviceInfo(idx);
    if (!info) return std::nullopt;
    return fromPaInfo(idx, info, false, true);
}

} // namespace echoshade
