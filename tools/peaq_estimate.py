"""
tools/peaq_estimate.py
Simplified PEAQ-like perceptual quality estimator for EchoShade.

Computes a subset of PEAQ (ITU-T BS.1387) Objective Difference Grade (ODG)
indicators between an original and perturbed audio file.  This is NOT a
standards-compliant PEAQ implementation — it implements the key inner-ear
model and select Model Output Variables (MOVs) sufficient for catching
gross perceptual degradations during development.

Implemented indicators:
  - Bandwidth (BW): fraction of audible frequency range used
  - Noise-to-Mask Ratio (NMR): dB excess of perturbation over masking threshold
  - Modulation Difference (MDiff): difference in temporal envelope modulation
  - Estimated ODG: weighted sum of MOVs -> mapped to [-4, 0] scale

Usage:
    python3 peaq_estimate.py original.wav perturbed.wav [--fft 2048] [--hop 512]

Outputs:
    - NMR per 1/3-octave band
    - Overall estimated ODG (Objective Difference Grade):
        0     = imperceptible
       -1     = perceptible but not annoying
       -2     = slightly annoying
       -3     = annoying
       -4     = very annoying
    - Target: ODG >= -1 for EchoShade perturbations

References:
  ITU-T Recommendation BS.1387 (2001)
  Thiede et al., "PEAQ - The ITU Standard for Objective Measurement of
  Perceived Audio Quality," J. AES, 2000.
"""

import argparse
import sys
import math
import os

try:
    import numpy as np
    import scipy.io.wavfile as wav
    import scipy.signal as sig
except ImportError:
    sys.exit("ERROR: numpy and scipy are required.  pip install numpy scipy")

def load_mono(path: str):
    rate, data = wav.read(path)
    if data.ndim > 1:
        data = data[:, 0]
    if data.dtype == np.int16:
        data = data.astype(np.float64) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float64) / 2147483648.0
    else:
        data = data.astype(np.float64)
    return data, rate

def hz_to_bark(f):
    return 13.0 * math.atan(0.00076 * f) + 3.5 * math.atan((f / 7500.0) ** 2)


def bark_edges_hz(n_bands=24, sr=48000):
    barks = np.linspace(0.0, n_bands, n_bands + 1)
    hz = 1960.0 * (barks + 0.53) / (26.28 - barks)
    hz = np.clip(hz, 0.0, sr / 2)
    return hz

def ath_dbfs(f_hz: np.ndarray, ref_spl_dbfs: float = -90.0) -> np.ndarray:
    f = np.where(f_hz > 0, f_hz / 1000.0, 0.001)
    ath = (3.64 * f ** (-0.8)
           - 6.5 * np.exp(-0.6 * (f - 3.3) ** 2)
           + 1e-3 * f ** 4)
    return ath + ref_spl_dbfs

def build_spread_matrix(n_bands=24):
    S = np.zeros((n_bands, n_bands))
    for src in range(n_bands):
        for dst in range(n_bands):
            dz = dst - src
            if dz >= 0:
                S[src, dst] = 10.0 ** (-10.0 * dz / 10.0)
            else:
                S[src, dst] = 10.0 ** (-25.0 * abs(dz) / 10.0)
    return S

def compute_stft(x: np.ndarray, sr: int, fft_size: int, hop_size: int):
    freqs, times, Zxx = sig.stft(x, fs=sr, window="hann",
                                  nperseg=fft_size, noverlap=fft_size - hop_size,
                                  padded=True, boundary="zeros")
    return freqs, np.abs(Zxx)

def compute_nmr(ref_mags, pert_mags, freqs, sr, n_bands=24):
    spread_mtx = build_spread_matrix(n_bands)
    edges_hz   = bark_edges_hz(n_bands, sr)

    n_frames = ref_mags.shape[1]
    nmr_accum = np.zeros(n_bands)

    for t in range(n_frames):
        ref_p  = ref_mags[:, t] ** 2 + 1e-15
        pert_p = pert_mags[:, t] ** 2 + 1e-15
        noise_p = np.abs(pert_p - ref_p)

        excit = np.zeros(n_bands)
        noise = np.zeros(n_bands)
        bin_hz = freqs

        for b in range(n_bands):
            mask = (bin_hz >= edges_hz[b]) & (bin_hz < edges_hz[b + 1])
            if mask.sum() == 0:
                continue
            excit[b] = ref_p[mask].max()
            noise[b] = noise_p[mask].sum()

        spread_excit = spread_mtx.T @ excit

        band_centers_hz = 0.5 * (edges_hz[:-1] + edges_hz[1:])
        ath_p = 10.0 ** (ath_dbfs(band_centers_hz) / 10.0)
        threshold_p = np.maximum(spread_excit, ath_p)

        nmr_frame = 10.0 * np.log10(noise / threshold_p + 1e-15)
        nmr_accum += nmr_frame

    band_centers_hz = 0.5 * (edges_hz[:-1] + edges_hz[1:])
    return band_centers_hz, nmr_accum / max(n_frames, 1)


def compute_bandwidth(ref_mags, freqs, threshold_db=-60.0):
    ref_db = 20.0 * np.log10(ref_mags.mean(axis=1) + 1e-15)
    above  = (ref_db > threshold_db).sum()
    return above / len(freqs)


def compute_mod_diff(ref_mags, pert_mags):
    ref_env  = ref_mags.mean(axis=0)
    pert_env = pert_mags.mean(axis=0)
    ref_env  = ref_env  / (ref_env.max()  + 1e-15)
    pert_env = pert_env / (pert_env.max() + 1e-15)
    return float(np.mean(np.abs(ref_env - pert_env)))


def estimate_odg(nmr_db_mean: float, bw_frac: float, mod_diff: float) -> float:
    nmr_score = max(0.0, nmr_db_mean) / 10.0

    bw_score = max(0.0, 0.3 - bw_frac)

    mod_score = max(0.0, mod_diff - 0.05) * 4.0

    raw = -(nmr_score + bw_score * 2.0 + mod_score)
    return max(-4.0, min(0.0, raw))


def odg_description(odg: float) -> str:
    if odg > -0.5:  return "imperceptible (0)"
    if odg > -1.5:  return "perceptible but not annoying (-1)"
    if odg > -2.5:  return "slightly annoying (-2)"
    if odg > -3.5:  return "annoying (-3)"
    return "very annoying (-4)"

def main():
    parser = argparse.ArgumentParser(
        description="Estimate perceptual quality degradation (simplified PEAQ)")
    parser.add_argument("original",  help="Original WAV file")
    parser.add_argument("perturbed", help="Perturbed WAV file")
    parser.add_argument("--fft",     type=int, default=2048, help="FFT size (default 2048)")
    parser.add_argument("--hop",     type=int, default=512,  help="Hop size (default 512)")
    args = parser.parse_args()

    print(f"Loading  original: {args.original}")
    ref_x,  ref_sr  = load_mono(args.original)
    print(f"Loading perturbed: {args.perturbed}")
    pert_x, pert_sr = load_mono(args.perturbed)

    if ref_sr != pert_sr:
        print(f"WARNING: sample rates differ ({ref_sr} vs {pert_sr})")
    sr = ref_sr

    n = min(len(ref_x), len(pert_x))
    ref_x, pert_x = ref_x[:n], pert_x[:n]
    print(f"Length: {n} samples ({n/sr:.2f} s) @ {sr} Hz")
    print(f"FFT={args.fft}, hop={args.hop}\n")

    freqs, ref_mags  = compute_stft(ref_x,  sr, args.fft, args.hop)
    _,     pert_mags = compute_stft(pert_x, sr, args.fft, args.hop)

    band_hz, nmr_db = compute_nmr(ref_mags, pert_mags, freqs, sr)

    print("NMR per Bark band (positive = audible noise)")
    print(f"{'Band (Hz)':>10}  {'NMR (dB)':>10}  {'Status':>20}")
    print("-" * 44)
    for hz, nmr in zip(band_hz, nmr_db):
        status = "AUDIBLE !" if nmr > 0 else "masked"
        print(f"{hz:10.1f}  {nmr:10.2f}  {status:>20}")

    nmr_mean     = float(nmr_db.mean())
    nmr_max      = float(nmr_db.max())
    bw_frac      = compute_bandwidth(ref_mags, freqs)
    mod_diff     = compute_mod_diff(ref_mags, pert_mags)
    odg          = estimate_odg(nmr_mean, bw_frac, mod_diff)

    print("\nSummary")
    print(f"  Mean NMR       : {nmr_mean:+.2f} dB")
    print(f"  Max  NMR       : {nmr_max:+.2f} dB")
    print(f"  Bandwidth frac : {bw_frac:.3f}")
    print(f"  Modulation Δ   : {mod_diff:.4f}")
    print(f"  Estimated ODG  : {odg:.2f}  ->  {odg_description(odg)}")

    if odg >= -1.0:
        print("\n   Quality target met (ODG >= -1, perceptible but not annoying)")
    else:
        print(f"\n Quality degradation exceeds target (ODG = {odg:.2f} < -1.0)")
        print("    Consider reducing perturbation intensity or narrowing the")
        print("    frequency bands targeted by the perturbation engine.")

    return 0 if odg >= -1.0 else 1


if __name__ == "__main__":
    sys.exit(main())