#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>

#include "dr_flac.h"
#include "dr_mp3.h"

#include "wav_decoder.hpp"
#include "dsd_decoder.hpp"

namespace audio_forensic {

struct DecodedAudio {
    int channels = 0;
    int sample_rate = 0;
    int bit_depth = 16;
    double duration_sec = 0.0;
    std::string codec_name;
    std::vector<float> mid;
    std::vector<float> side; // empty if mono
    std::vector<int32_t> raw_i32_stereo; // for bit depth / MQA analysis
};

inline bool decodeAudioFile(const std::string& path, DecodedAudio& out, double max_seconds = 0.0) {
    namespace fs = std::filesystem;
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::vector<float> interleaved;
    int ch = 0;
    int sr = 0;
    int bits = 16;
    uint64_t total_frames = 0;

    if (ext == ".flac") {
        drflac* pFlac = drflac_open_file(path.c_str(), nullptr);
        if (!pFlac) return false;

        ch = pFlac->channels;
        sr = pFlac->sampleRate;
        bits = pFlac->bitsPerSample;
        total_frames = pFlac->totalPCMFrameCount;
        out.codec_name = "FLAC";

        if (max_seconds > 0.0) {
            uint64_t cap = static_cast<uint64_t>(max_seconds * sr);
            total_frames = std::min(total_frames, cap);
        }

        interleaved.resize(total_frames * ch);
        drflac_read_pcm_frames_f32(pFlac, total_frames, interleaved.data());
        drflac_close(pFlac);

    } else if (ext == ".mp3") {
        drmp3 mp3;
        if (!drmp3_init_file(&mp3, path.c_str(), nullptr)) return false;

        ch = mp3.channels;
        sr = mp3.sampleRate;
        bits = 16;
        total_frames = drmp3_get_pcm_frame_count(&mp3);
        out.codec_name = "MP3";

        if (max_seconds > 0.0) {
            uint64_t cap = static_cast<uint64_t>(max_seconds * sr);
            total_frames = std::min(total_frames, cap);
        }

        interleaved.resize(total_frames * ch);
        drmp3_read_pcm_frames_f32(&mp3, total_frames, interleaved.data());
        drmp3_uninit(&mp3);

    } else if (ext == ".wav" || ext == ".wave") {
        WavInfo winfo;
        if (!readWav(path, winfo, max_seconds)) return false;

        ch = winfo.channels;
        sr = winfo.sample_rate;
        bits = winfo.bits_per_sample;
        total_frames = winfo.total_samples;
        interleaved = std::move(winfo.pcm_interleaved);
        out.codec_name = "PCM WAV";

    } else if (ext == ".dsf" || ext == ".dff") {
        DsdInfo dinfo;
        if (!readDsf(path, dinfo, max_seconds)) return false;

        ch = dinfo.channels;
        sr = dinfo.target_sample_rate; // 88.2 kHz
        bits = 24;
        total_frames = dinfo.total_pcm_frames;
        interleaved = std::move(dinfo.pcm_interleaved);
        out.codec_name = "DSD";
    } else {
        // Unknown / unsupported container
        return false;
    }

    if (ch == 0 || sr == 0 || interleaved.empty()) return false;

    out.channels = ch;
    out.sample_rate = sr;
    out.bit_depth = bits;
    out.duration_sec = static_cast<double>(total_frames) / static_cast<double>(sr);

    out.mid.resize(total_frames);
    if (ch >= 2) {
        out.side.resize(total_frames);
        for (size_t i = 0; i < total_frames; ++i) {
            float l = interleaved[i * ch + 0];
            float r = interleaved[i * ch + 1];
            out.mid[i] = (l + r) * 0.5f;
            out.side[i] = (l - r) * 0.5f;
        }
    } else {
        for (size_t i = 0; i < total_frames; ++i) {
            out.mid[i] = interleaved[i];
        }
    }

    // Populate raw_i32_stereo for bit-depth & MQA analysis (up to 30s)
    size_t probe_frames = std::min(total_frames, static_cast<uint64_t>(30 * sr));
    out.raw_i32_stereo.resize(probe_frames * 2);
    for (size_t i = 0; i < probe_frames; ++i) {
        float l = interleaved[i * ch + 0];
        float r = (ch >= 2) ? interleaved[i * ch + 1] : l;
        out.raw_i32_stereo[i * 2 + 0] = static_cast<int32_t>(std::clamp(l, -1.0f, 1.0f) * 2147483647.0f);
        out.raw_i32_stereo[i * 2 + 1] = static_cast<int32_t>(std::clamp(r, -1.0f, 1.0f) * 2147483647.0f);
    }

    return true;
}

} // namespace audio_forensic
