#include "echoshade/STFTProcessor.hpp"
#include <kiss_fftr.h>
#include <cstring>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace echoshade {

struct STFTProcessor::Impl {
    STFTConfig cfg;

    kiss_fftr_cfg fwdCfg = nullptr;
    kiss_fftr_cfg invCfg = nullptr;

    std::vector<float> window;
    std::vector<float> normEnv;

    std::vector<float> inBuf;
    int                inFill = 0;

    std::vector<float>              timeBuf;
    std::vector<kiss_fft_cpx>       freqBuf;
    std::vector<float>              ifftOut;

    std::vector<float> olaBuf;
    int                olaBufWritePos = 0;

    std::vector<float> outBuf;
    int                outRead  = 0;
    int                outWrite = 0;
    int                outCount = 0;

    void init(const STFTConfig& c) {
        cfg = c;
        assert(cfg.fftSize > 0 && (cfg.fftSize & 1) == 0 && "fftSize must be even");
        assert(cfg.hopSize > 0 && cfg.hopSize <= cfg.fftSize);

        fwdCfg = kiss_fftr_alloc(cfg.fftSize, 0, nullptr, nullptr);
        invCfg = kiss_fftr_alloc(cfg.fftSize, 1, nullptr, nullptr);
        if (!fwdCfg || !invCfg)
            throw std::runtime_error("STFTProcessor: kiss_fftr_alloc failed");

        window  = STFTProcessor::makeWindow(cfg.windowType, cfg.fftSize, cfg.kaiserBeta);
        normEnv = STFTProcessor::computeNormEnv(window, cfg.hopSize);

        inBuf.assign(cfg.fftSize, 0.0f);
        inFill = 0;

        timeBuf.resize(cfg.fftSize);
        freqBuf.resize(cfg.fftSize / 2 + 1);
        ifftOut.resize(cfg.fftSize);

        olaBuf.assign(static_cast<size_t>(cfg.fftSize + cfg.hopSize), 0.0f);
        olaBufWritePos = 0;

        const int kOutCapacity = cfg.fftSize * 8;
        outBuf.resize(kOutCapacity, 0.0f);
        outRead = outWrite = outCount = 0;
    }

    void cleanup() noexcept {
        if (fwdCfg) { kiss_fftr_free(fwdCfg); fwdCfg = nullptr; }
        if (invCfg) { kiss_fftr_free(invCfg); invCfg = nullptr; }
    }

    int outCapacity() const noexcept { return static_cast<int>(outBuf.size()); }

    void outPush(const float* data, int n) noexcept {
        for (int i = 0; i < n; ++i) {
            if (outCount >= outCapacity()) break;
            outBuf[outWrite] = data[i];
            outWrite = (outWrite + 1) % outCapacity();
            ++outCount;
        }
    }

    int outPop(float* out, int maxN) noexcept {
        const int n = std::min(maxN, outCount);
        for (int i = 0; i < n; ++i) {
            out[i] = outBuf[outRead];
            outRead = (outRead + 1) % outCapacity();
        }
        outCount -= n;
        return n;
    }

    void processHop(const SpectralCallback& cb, double sampleRate) noexcept {
        for (int i = 0; i < cfg.fftSize; ++i)
            timeBuf[i] = inBuf[i] * window[i];

        kiss_fftr(fwdCfg, timeBuf.data(), freqBuf.data());

        auto* spec = reinterpret_cast<std::complex<float>*>(freqBuf.data());

        if (cb)
            cb(spec, cfg.fftSize / 2 + 1, sampleRate);


        kiss_fftri(invCfg, freqBuf.data(), ifftOut.data());

        const float scale = 1.0f / static_cast<float>(cfg.fftSize);
        for (int i = 0; i < cfg.fftSize; ++i)
            ifftOut[i] *= scale;

        const int sz = cfg.fftSize;
        const int cap = static_cast<int>(olaBuf.size());
        for (int i = 0; i < sz; ++i) {
            const int pos = (olaBufWritePos + i) % cap;
            olaBuf[pos] += ifftOut[i];
        }

        float hopOut[4096];
        const int hop = cfg.hopSize;
        for (int k = 0; k < hop; ++k) {
            const int pos = (olaBufWritePos + k) % cap;
            const float norm = normEnv[k];
            hopOut[k] = (norm > 1e-9f) ? olaBuf[pos] / norm : 0.0f;
            olaBuf[pos] = 0.0f;
        }
        outPush(hopOut, hop);

        olaBufWritePos = (olaBufWritePos + hop) % cap;
    }
};

static double besselI0(double x) noexcept {
    if (x < 0.0) x = -x;
    if (x <= 3.75) {
        const double t = (x / 3.75);
        const double t2 = t * t;
        return 1.0 + t2*(3.5156229 + t2*(3.0899424 + t2*(1.2067492
             + t2*(0.2659732 + t2*(0.0360768 + t2*0.0045813)))));
    } else {
        const double t = 3.75 / x;
        const double poly = 0.39894228 + t*(0.01328592 + t*(0.00225319
            + t*(-0.00157565 + t*(0.00916281 + t*(-0.02057706
            + t*(0.02635537 + t*(-0.01647633 + t*0.00392377)))))));
        return poly * std::exp(x) / std::sqrt(x);
    }
}

std::vector<float> STFTProcessor::makeWindow(WindowType type, int size, double kaiserBeta) {
    assert(size > 0);
    std::vector<float> w(size);
    const double N = static_cast<double>(size);

    switch (type) {
    case WindowType::Hann:
        for (int n = 0; n < size; ++n)
            w[n] = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * M_PI * n / N)));
        break;

    case WindowType::BlackmanHarris: {
        static constexpr double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
        for (int n = 0; n < size; ++n) {
            const double phi = 2.0 * M_PI * n / N;
            w[n] = static_cast<float>(a0 - a1*std::cos(phi) + a2*std::cos(2*phi) - a3*std::cos(3*phi));
        }
        break;
    }

    case WindowType::Kaiser: {
        const double denom = besselI0(kaiserBeta);
        const double Nm1   = static_cast<double>(size - 1);
        const double half  = Nm1 / 2.0;
        for (int n = 0; n < size; ++n) {
            const double x = (n - half) / half; // [-1, 1]
            const double arg = kaiserBeta * std::sqrt(std::max(0.0, 1.0 - x*x));
            w[n] = static_cast<float>(besselI0(arg) / denom);
        }
        break;
    }
    }
    return w;
}

std::vector<float> STFTProcessor::computeNormEnv(const std::vector<float>& window, int hopSize) {
    const int fftSize = static_cast<int>(window.size());
    std::vector<float> env(hopSize, 0.0f);

    for (int k = 0; k < hopSize; ++k) {
        float sum = 0.0f;
        for (int l = 0; k + l * hopSize < fftSize; ++l)
            sum += window[k + l * hopSize];
        env[k] = sum;
    }
    return env;
}

STFTProcessor::STFTProcessor(const STFTConfig& cfg) {
    impl_ = new Impl();
    impl_->init(cfg);
}

STFTProcessor::~STFTProcessor() {
    if (impl_) {
        impl_->cleanup();
        delete impl_;
    }
}

int STFTProcessor::fftSize()  const noexcept { return impl_->cfg.fftSize; }
int STFTProcessor::hopSize()  const noexcept { return impl_->cfg.hopSize; }
int STFTProcessor::numBins()  const noexcept { return impl_->cfg.fftSize / 2 + 1; }

double STFTProcessor::binFrequency(int bin, double sampleRate) const noexcept {
    return static_cast<double>(bin) * sampleRate / static_cast<double>(impl_->cfg.fftSize);
}

int STFTProcessor::available() const noexcept { return impl_->outCount; }

void STFTProcessor::pushSamples(const float* samples, int count,
                                 const SpectralCallback& spectralCb,
                                 double sampleRate) noexcept {
    const int fftSz = impl_->cfg.fftSize;
    const int hop   = impl_->cfg.hopSize;

    int consumed = 0;
    while (consumed < count) {
        const int room    = fftSz - impl_->inFill;
        const int canTake = std::min(room, count - consumed);

        std::memcpy(impl_->inBuf.data() + (fftSz - room), samples + consumed,
                    static_cast<size_t>(canTake) * sizeof(float));
        impl_->inFill += canTake;
        consumed      += canTake;

        if (impl_->inFill >= fftSz) {
            impl_->processHop(spectralCb, sampleRate);

            std::memmove(impl_->inBuf.data(),
                         impl_->inBuf.data() + hop,
                         static_cast<size_t>(fftSz - hop) * sizeof(float));
            impl_->inFill = fftSz - hop;
        }
    }
}

int STFTProcessor::pullSamples(float* out, int maxCount) noexcept {
    return impl_->outPop(out, maxCount);
}

void STFTProcessor::reset() noexcept {
    std::fill(impl_->inBuf.begin(), impl_->inBuf.end(), 0.0f);
    impl_->inFill = 0;
    std::fill(impl_->olaBuf.begin(), impl_->olaBuf.end(), 0.0f);
    impl_->olaBufWritePos = 0;
    impl_->outRead = impl_->outWrite = impl_->outCount = 0;
}

} // namespace echoshade