# Audio Forensic Native Core (C++20)

High-performance, standalone native implementation of the Audio Forensic DSP engine with zero external CLI dependencies (no Python, no ffmpeg, no sox, no mediainfo required).

---

## Included Components

- **`spectral_engine.hpp` / `spectral_engine.cpp`**: 11-rule forensic engine, auCDtect bound & high-band phase entropy, Derrien JAES 2019 MDCT quantization lattice, Vorbis transform grid, 0–100 Main Score and categorical verdicts.
- **`dsp_math.hpp`**: Self-contained Cooley-Tukey Radix-2 & Bluestein FFT, Butterworth IIR filter design with Second-Order Sections (SOS), autocorrelation, temporal variance, Kaiser-Bessel Derived (KBD) window, DCT-IV.
- **`audio_decoder.hpp` / `audio_decoder.cpp`**:
  - `dr_flac`: Native bit-exact FLAC decoding.
  - `dr_mp3`: Native MP3 decoding.
  - `wav_decoder.hpp`: Native WAV/AIFF PCM parser.
  - `dsd_decoder.hpp`: Streaming DSD (DSF/DFF) 1-bit decimation to 88.2 kHz PCM with flat memory usage.
- **`jni_bridge.cpp`**: JNI bridge for Android integration with live progress callbacks and FFT spectrogram bitmap generation.
- **`test_dsp_native.cpp`**: Mathematical verification test suite matching `test_dsp.py`.
- **`cli_main.cpp`**: Portable desktop CLI executable (`audio_forensic_native.exe`).

---

## Verification Test

To build and run the mathematical verification suite on Windows:

```powershell
g++ -std=c++20 -O3 -mconsole test_dsp_native.cpp spectral_engine.cpp audio_decoder.cpp -o test_dsp_native.exe
.\test_dsp_native.exe
```

## Running the Desktop CLI

```powershell
.\audio_forensic_native.exe "path/to/track.flac"
.\audio_forensic_native.exe "path/to/track.wav" --fast
```
