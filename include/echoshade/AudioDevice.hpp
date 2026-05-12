#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace echoshade {

enum class AudioDirection { Input, Output };

struct AudioDeviceInfo {
    int         index       = -1;
    std::string name;
    int         maxInputChannels  = 0;
    int         maxOutputChannels = 0;
    double      defaultSampleRate = 48000.0;
    bool        isDefaultInput    = false;
    bool        isDefaultOutput   = false;
};

/// Enumerates and queries audio devices via the PortAudio backend.
class AudioDevice {
public:
    static std::vector<AudioDeviceInfo> enumerateInputs();
    static std::vector<AudioDeviceInfo> enumerateOutputs();
    static AudioDeviceInfo              defaultInput();
    static AudioDeviceInfo              defaultOutput();
};

} // namespace echoshade
