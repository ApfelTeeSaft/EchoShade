#pragma once
#include <functional>
#include <cstdint>

namespace echoshade {

struct AudioInputConfig {
    int    deviceIndex  = -1;   // -1 = system default
    int    channels     = 1;
    double sampleRate   = 48000.0;
    int    framesPerBuffer = 512;
};

/// Wraps a PortAudio input stream. Delivers audio frames to a user callback.
class AudioInput {
public:
    using Callback = std::function<void(const float* samples, int frameCount, int channels)>;

    explicit AudioInput(const AudioInputConfig& cfg);
    ~AudioInput();

    AudioInput(const AudioInput&)            = delete;
    AudioInput& operator=(const AudioInput&) = delete;

    void setCallback(Callback cb);
    bool start();
    void stop();
    bool isRunning() const;

    double sampleRate()  const { return cfg_.sampleRate; }
    int    channels()    const { return cfg_.channels; }
    int    framesPerBuffer() const { return cfg_.framesPerBuffer; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    AudioInputConfig cfg_;
    Callback callback_;
};

} // namespace echoshade
