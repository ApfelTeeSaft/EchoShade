#pragma once
#include <string>
#include <vector>
#include <optional>

namespace echoshade {

struct AudioDeviceInfo {
    int         index       = -1;
    std::string name;
    int         maxInputChannels  = 0;
    int         maxOutputChannels = 0;
    double      defaultSampleRate = 48000.0;
    double      defaultLowInputLatencyMs  = 10.0;
    double      defaultLowOutputLatencyMs = 10.0;
    bool        isDefaultInput    = false;
    bool        isDefaultOutput   = false;
};

/// Queries PortAudio for available audio devices.
/// Initialises the PA context as a side-effect.
class AudioDevice {
public:
    /// Returns all devices that have at least one input channel.
    static std::vector<AudioDeviceInfo> enumerateInputs();

    /// Returns all devices that have at least one output channel.
    static std::vector<AudioDeviceInfo> enumerateOutputs();

    /// System default input device, or nullopt if none exists.
    static std::optional<AudioDeviceInfo> defaultInput();

    /// System default output device, or nullopt if none exists.
    static std::optional<AudioDeviceInfo> defaultOutput();

    /// Returns true if PortAudio could be initialised successfully.
    static bool isAvailable() noexcept;
};

} // namespace echoshade
