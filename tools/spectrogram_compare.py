"""
tools/spectrogram_compare.py
Visualise and compare spectrograms of two audio files to verify perturbation
quality: the perturbed file should look similar to the original while having
measurable differences in the higher frequency bands.

Usage:
    python3 spectrogram_compare.py original.wav perturbed.wav [--fft 1024] [--hop 256]

Output:
    - Side-by-side spectrogram PNG saved to <perturbed_basename>_compare.png
    - Difference map (dB) saved to <perturbed_basename>_diff.png
    - Console summary: mean dB difference per 1/3-octave band
"""

import argparse
import sys
import os
import math

try:
    import numpy as np
    import scipy.io.wavfile as wav
    import scipy.signal as sig
except ImportError:
    sys.exit("ERROR: numpy and scipy are required. Install with:  pip install numpy scipy")

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MPL = True
except ImportError:
    HAS_MPL = False


def load_mono(path: str):
    rate, data = wav.read(path)
    if data.ndim > 1:
        data = data[:, 0]  # take left channel
    if data.dtype == np.int16:
        data = data.astype(np.float32) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float32) / 2147483648.0
    elif data.dtype != np.float32:
        data = data.astype(np.float32)
    return data, rate


def compute_stft(samples: np.ndarray, sample_rate: int, fft_size: int, hop_size: int):
    freqs, times, Zxx = sig.stft(
        samples,
        fs=sample_rate,
        window="hann",
        nperseg=fft_size,
        noverlap=fft_size - hop_size,
        padded=True,
        boundary="zeros",
    )
    mag_db = 20.0 * np.log10(np.abs(Zxx) + 1e-9)
    return freqs, times, mag_db


def third_octave_bands(fmin=20.0, fmax=20000.0):
    bands = []
    fc = fmin
    factor = 2.0 ** (1.0 / 3.0)
    while fc <= fmax:
        f_lo = fc / (factor ** 0.5)
        f_hi = fc * (factor ** 0.5)
        bands.append((f_lo, fc, f_hi))
        fc *= factor
    return bands


def band_mean_diff(freqs, diff_db):
    bands = third_octave_bands()
    results = []
    for f_lo, f_c, f_hi in bands:
        mask = (freqs >= f_lo) & (freqs < f_hi)
        if mask.sum() == 0:
            continue
        mean_diff = float(np.mean(np.abs(diff_db[mask, :])))
        results.append((f_c, mean_diff))
    return results


def main():
    parser = argparse.ArgumentParser(description="Compare spectrograms of two audio files")
    parser.add_argument("original",  help="Original WAV file")
    parser.add_argument("perturbed", help="Perturbed WAV file")
    parser.add_argument("--fft",  type=int, default=1024, help="FFT size (default 1024)")
    parser.add_argument("--hop",  type=int, default=256,  help="Hop size (default 256)")
    parser.add_argument("--vmin", type=float, default=-80.0, help="Colour scale min dB")
    parser.add_argument("--vmax", type=float, default=0.0,   help="Colour scale max dB")
    args = parser.parse_args()

    print(f"Loading  original: {args.original}")
    orig_samples, orig_sr = load_mono(args.original)

    print(f"Loading perturbed: {args.perturbed}")
    pert_samples, pert_sr = load_mono(args.perturbed)

    if orig_sr != pert_sr:
        print(f"WARNING: sample rates differ ({orig_sr} vs {pert_sr}), results may be inaccurate")

    sr = orig_sr

    n = min(len(orig_samples), len(pert_samples))
    orig_samples = orig_samples[:n]
    pert_samples = pert_samples[:n]

    print(f"Signal length: {n} samples ({n/sr:.2f} s) @ {sr} Hz")
    print(f"FFT size: {args.fft}  Hop size: {args.hop}")

    freqs, times, orig_db  = compute_stft(orig_samples, sr, args.fft, args.hop)
    _,     _,     pert_db  = compute_stft(pert_samples, sr, args.fft, args.hop)

    diff_db = pert_db - orig_db

    print("\n── 1/3-octave band mean |ΔdB|")
    print(f"{'Band (Hz)':>12}  {'|ΔdB| mean':>12}")
    print("-" * 28)
    band_diffs = band_mean_diff(freqs, diff_db)
    for f_c, mean_diff in band_diffs:
        marker = " ◄" if mean_diff > 3.0 else ""
        print(f"{f_c:12.1f}  {mean_diff:12.3f}{marker}")

    total_rms = float(np.sqrt(np.mean(diff_db ** 2)))
    print(f"\nOverall RMS ΔdB : {total_rms:.3f} dB")
    print(f"Overall max |ΔdB|: {float(np.max(np.abs(diff_db))):.3f} dB")

    if not HAS_MPL:
        print("\nmatplotlib not available — skipping PNG output.")
        return

    base = os.path.splitext(os.path.basename(args.perturbed))[0]
    out_cmp  = f"{base}_compare.png"
    out_diff = f"{base}_diff.png"

    # Side-by-side comparison.
    fig, axes = plt.subplots(1, 2, figsize=(14, 5), sharey=True)
    for ax, db, title in zip(axes, [orig_db, pert_db], ["Original", "Perturbed"]):
        img = ax.pcolormesh(times, freqs, db, cmap="inferno",
                            vmin=args.vmin, vmax=args.vmax, shading="auto")
        ax.set_title(title)
        ax.set_ylabel("Frequency (Hz)")
        ax.set_xlabel("Time (s)")
        ax.set_yscale("symlog", linthresh=100)
        plt.colorbar(img, ax=ax, label="dBFS")
    fig.suptitle(f"Spectrogram comparison: {os.path.basename(args.original)}")
    fig.tight_layout()
    fig.savefig(out_cmp, dpi=150)
    plt.close(fig)
    print(f"\nSaved comparison plot → {out_cmp}")

    # Difference map.
    fig, ax = plt.subplots(figsize=(10, 4))
    clim = max(3.0, float(np.percentile(np.abs(diff_db), 99)))
    img = ax.pcolormesh(times, freqs, diff_db, cmap="RdBu_r",
                        vmin=-clim, vmax=clim, shading="auto")
    ax.set_title(f"Perturbation ΔdB (perturbed − original)")
    ax.set_ylabel("Frequency (Hz)")
    ax.set_xlabel("Time (s)")
    ax.set_yscale("symlog", linthresh=100)
    plt.colorbar(img, ax=ax, label="ΔdB")
    fig.tight_layout()
    fig.savefig(out_diff, dpi=150)
    plt.close(fig)
    print(f"Saved difference map  → {out_diff}")


if __name__ == "__main__":
    main()