# Audio Forensic App 🔍🎵

[![Build Android APK](https://github.com/spideyonmoon/audio-forensic-app/actions/workflows/build-apk.yml/badge.svg)](https://github.com/spideyonmoon/audio-forensic-app/actions/workflows/build-apk.yml)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Android%20%7C%20Windows%20%7C%20Linux-blue.svg)]()

**State-of-the-art lossless-authenticity forensics for music on Android and Desktop.**

Detects fake lossless files (lossy transcodes hiding in FLAC, WAV, ALAC, or DSD containers), resampled fake hi-res, and lossy encoder fingerprints using calibrated DSP algorithms, high-precision STFT, auCDtect scatter analysis, Derrien MDCT quantization lattices, and a unified 0–100 Main Score.

---

## 📱 Download for Android (No Android Studio Needed!)

You don't need Android Studio or any build tools to install the app on your phone:

1. Go to the [**Releases**](https://github.com/spideyonmoon/audio-forensic-app/releases) section (or check the [**Actions**](https://github.com/spideyonmoon/audio-forensic-app/actions) tab for the latest build).
2. Download **`AudioForensic-debug.apk`** directly on your Android phone or DAP (Digital Audio Player).
3. Tap the downloaded file to install and start scanning!

---

## 🏗️ Repository Architecture

- **`android/`**: Complete native Android app built with **Kotlin + Jetpack Compose (Material 3)**.
  - Storage Access Framework (SAF) integration for picking individual tracks or batch scanning entire albums.
  - Real-time animated FFT spectrogram visualizer.
  - Background scanning via Android `WorkManager`.
- **`native/`**: High-performance **C++20** DSP forensic engine and decoders.
  - Built-in zero-dependency decoders: `dr_flac` (FLAC), `dr_mp3` (MP3), WAV/AIFF, and streaming DSD (DSF/DFF) decimation.
  - Self-contained FFT, Butterworth IIR filter design, auCDtect scatter, and MDCT lattice analysis.
  - JNI bridge (`jni_bridge.cpp`) and standalone desktop CLI executable (`audio_forensic_native.exe`).
- **`audio-forensic/`**: The original Python desktop reference implementation.

---

## ⚡ Safe DSD & High-Res Streaming

Scanning multi-gigabyte **DSD (DSF/DFF)** files or 24-bit/192 kHz FLAC files on mobile devices can easily freeze a phone or trigger an Out-Of-Memory (OOM) crash if decoded naively into RAM. 

This engine implements **streaming 1-bit Sigma-Delta decimation**:
- Blocks of 1-bit samples are filtered and decimated on the fly to **88.2 kHz PCM**.
- RAM usage remains completely flat (<15 MB) regardless of whether the file is 50 MB or 10 GB.

---

## 💻 Running the Desktop CLI

A pre-compiled native Windows binary is available in `native/`:

```powershell
cd native

# Scan a single audio file
.\audio_forensic_native.exe "path\to\song.flac"

# Fast 60s probe mode
.\audio_forensic_native.exe "path\to\song.wav" --fast

# Run the 10-check mathematical verification suite
.\test_dsp_native.exe
```

---

## 📜 License

MIT License
