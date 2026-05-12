#pragma once
#include <functional>
#include <cstdint>

namespace echoshade {

struct AudioOutputConfig {
    int    deviceIndex     = -1;
    int    channels        = 1;
    double sampleRate      = 48000.0;
    int    framesPerBuffer = 512;
};

/// Wraps a PortAudio output stream. Pulls processed frames from a provider callback.
class AudioOutput {
public:
    using Callback = std::function<void(float* samples, int frameCount, int channels)>;

    explicit AudioOutput(const AudioOutputConfig& cfg);
    ~AudioOutput();

    AudioOutput(const AudioOutput&)            = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    void setCallback(Callback cb);
    bool start();
    void stop();
    bool isRunning() const;

    double sampleRate()      const { return cfg_.sampleRate; }
    int    channels()        const { return cfg_.channels; }
    int    framesPerBuffer() const { return cfg_.framesPerBuffer; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    AudioOutputConfig cfg_;
    Callback callback_;
};

} // namespace echoshade
