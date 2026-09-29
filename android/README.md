# Audio Forensic Android App

A high-performance native Android application for detecting fake lossless files (lossy transcodes hiding in FLAC/WAV/ALAC/DSD containers), resampled fake hi-res, and lossy codec fingerprints on Android phones and Digital Audio Players (DAPs).

---

## Architecture

The app is built with a **split native architecture**:
- **Native Core (C++20 NDK)**: Zero-dependency DSP engine (`spectral_engine.cpp`) with built-in FFT, Butterworth IIR filtering, auCDtect scatter analysis, MDCT quantization lattice detection, and streaming audio decoders (`dr_flac`, `dr_mp3`, WAV, and streaming DSD decimation).
- **Android Frontend (Kotlin + Jetpack Compose)**: Material 3 audiophile dark UI, Storage Access Framework (SAF) integration, live progress ring, and real-time FFT spectrogram rendering.

```
┌────────────────────────────────────────────────────────┐
│           Android UI (Kotlin + Jetpack Compose)        │
│  - Storage Access Framework (SAF) File/Folder Picker   │
│  - Coroutines / WorkManager (Background processing)    │
│  - Interactive Spectrogram Canvas & Score Badges       │
└───────────────────────────┬────────────────────────────┘
                            │ JNI (jni_bridge.cpp)
┌───────────────────────────▼────────────────────────────┐
│                  Native Core (C++20)                   │
│  - dr_flac (zero-dependency bit-exact FLAC decoding)   │
│  - dr_mp3 (zero-dependency MP3 decoding)               │
│  - Streaming DSD decimator (1-bit -> 88.2 kHz PCM)     │
│  - High-precision FFT & IIR Butterworth filters        │
│  - auCDtect bound collapse & phase entropy engine      │
│  - Derrien MDCT quantization-error lattice detector    │
│  - 0–100 Unified Main Score verdict                    │
└────────────────────────────────────────────────────────┘
```

---

## Features

- **Transcode Detection**: Identifies whether a FLAC or WAV file was previously an MP3, AAC, Vorbis, or Opus transcode.
- **DSD Safety**: Multi-gigabyte DSD (DSF/DFF) files are streamed and decimated on the fly to 88.2 kHz in small chunks. Never allocates full tracks in RAM, preventing Out-Of-Memory (OOM) crashes or phone freezes.
- **Fast Mode (60s probe)**: Analyzes the first 60 seconds of a track for instant verification and battery conservation.
- **Batch Album Scanning**: Scan an entire folder via SAF, with tracks ranked and color-coded by suspicion score.
- **Pinch-to-Zoom Spectrogram**: Real-time FFT spectrogram thumbnail with high-resolution visualizer.

---

## Opening and Building in Android Studio

1. Open **Android Studio** (Hedgehog / Iguana / Jellyfish / Koala or newer).
2. Select **Open** and choose the `android/` directory.
3. Gradle will automatically sync, download the NDK (if not already installed), and compile `libaudioforensic.so` via CMake.
4. Run on an Android device or emulator running **Android 8.0 (API 26) or higher** (supports arm64-v8a, armeabi-v7a, and x86_64).
