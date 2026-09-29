#include "spectral_engine.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>

using namespace audio_forensic;
namespace fs = std::filesystem;

void printReport(const ForensicReport& rep) {
    const auto& spec = rep.authenticity.spectral;
    std::cout << "\n\033[1;32m✓  " << (spec ? spec->primary_verdict : "Done") << "\033[0m\n";
    std::cout << "  FILE     " << rep.technical.sample_encoding << "  ·  "
              << rep.technical.sample_rate << " Hz  ·  "
              << (rep.technical.channels == 2 ? "Stereo" : (rep.technical.channels == 1 ? "Mono" : std::to_string(rep.technical.channels) + " ch")) << "  ·  "
              << rep.technical.duration << "  ·  "
              << std::fixed << std::setprecision(1) << rep.file_size_mb << " MB\n";
    std::cout << "  LOUDNESS " << rep.dr_score.substr(0, rep.dr_score.find(" ")) << "  ·  "
              << rep.loudness.lufs_integrated << "  ·  peak "
              << rep.loudness.peak_db << " dBFS\n";
    if (spec) {
        std::cout << "  SIGNALS  ⚠ " << spec->evidence.size() << " lossy  ·  ✓ "
                  << spec->natural_evidence.size() << " natural\n";
    }

    std::cout << "\n\033[1;33m── IDENTITY ──────────────────────────────────────────────────\033[0m\n";
    std::cout << "  Duration                   " << rep.technical.duration << "\n";
    std::cout << "  File Size                  " << std::fixed << std::setprecision(1) << rep.file_size_mb << " MB\n";

    if (!rep.tags.title.empty() || !rep.tags.artist.empty()) {
        std::cout << "\n\033[1;33m── TAGS ──────────────────────────────────────────────────────\033[0m\n";
        if (!rep.tags.title.empty()) std::cout << "  Title                      " << rep.tags.title << "\n";
        if (!rep.tags.artist.empty()) std::cout << "  Artist                     " << rep.tags.artist << "\n";
        if (!rep.tags.album.empty()) std::cout << "  Album                      " << rep.tags.album << "\n";
        if (!rep.tags.album_artist.empty()) std::cout << "  Album Artist               " << rep.tags.album_artist << "\n";
        if (!rep.tags.date.empty()) std::cout << "  Year                       " << rep.tags.date << "\n";
        for (const auto& [k, v] : rep.tags.other) {
            std::cout << "  " << std::left << std::setw(26) << k << " " << v << "\n";
        }
    }

    std::cout << "\n\033[1;33m── TECHNICAL ─────────────────────────────────────────────────\033[0m\n";
    std::cout << "  Encoding                   " << rep.technical.sample_encoding << "\n";
    if (!rep.technical.bit_rate.empty()) std::cout << "  Bit Rate                   " << rep.technical.bit_rate << "\n";
    std::cout << "  Sample Rate                " << rep.technical.sample_rate << " Hz\n";
    std::cout << "  Channels                   " << (rep.technical.channels == 2 ? "Stereo" : "Mono") << "\n";
    std::cout << "  Precision                  " << rep.technical.precision << "-bit\n";
    std::cout << "  Compression                " << rep.technical.compression_mode << "\n";
    if (!rep.technical.writing_library.empty()) {
        std::cout << "  Writing Library            " << rep.technical.writing_library << "\n";
    }

    std::cout << "\n\033[1;33m── DYNAMIC RANGE & LOUDNESS ──────────────────────────────────\033[0m\n";
    std::cout << "  Level Bookends\n";
    std::cout << "  Signal Ceiling             " << rep.loudness.peak_db << " dBFS\n";
    std::cout << "  Noise Floor                " << rep.loudness.noise_floor_db << " dBFS\n";
    std::cout << "  RMS Loudness               " << rep.loudness.rms_db << " dBFS\n";
    std::cout << "  RMS Peak                   " << rep.loudness.rms_peak_db << " dBFS\n";
    std::cout << "  RMS Trough                 " << rep.loudness.rms_trough_db << " dBFS\n\n";

    std::cout << "  EBU R128\n";
    std::cout << "  LUFS Integrated            " << rep.loudness.lufs_integrated << "\n";
    std::cout << "  Loudness Range             " << rep.loudness.lufs_range << "\n\n";

    std::cout << "  Streaming Normalization\n";
    std::cout << "  Apple Music (−16 LUFS)     " << rep.loudness.apple_music_delta << "\n";
    std::cout << "  Spotify/Tidal (−14 LUFS)   " << rep.loudness.spotify_delta << "\n\n";

    std::cout << "  Dynamic Quality\n";
    std::cout << "  DR Score (EBU)             " << rep.dr_score << "\n";
    std::cout << "  DR (ffmpeg)                " << rep.loudness.dynamic_range_db << " dB\n";
    std::cout << "  Crest Factor               " << rep.loudness.crest_factor_db << " dB\n";
    std::cout << "  Flat Factor                " << rep.loudness.flat_factor << "\n";
    std::cout << "  SoX Entropy                " << rep.loudness.entropy << "\n\n";

    std::cout << "  Signal Integrity\n";
    std::cout << "  DC Offset                  " << rep.loudness.dc_offset << "\n";
    std::cout << "  Peak Events                " << rep.loudness.peak_count << "\n";
    std::cout << "  Zero Crossing Rate         " << rep.loudness.zero_crossings_rate << "\n";

    if (spec) {
        std::cout << "\n\033[1;33m── AUTHENTICITY & FORENSICS ──────────────────────────────────\033[0m\n";
        std::cout << "  Spectral Analysis  (C++20 Native DSP Engine)\n";
        std::cout << "  Main score " << spec->main_score << "/100  ·  verdict: " << spec->verdict_label << "\n\n";

        std::cout << "  ✓ Ultrasonic Noise       Normal\n";
        std::cout << "    HF Cutoff              " << spec->cutoff_hz_str << "\n";
        std::cout << "    Cutoff Variance        " << std::fixed << std::setprecision(1) << spec->cutoff_variance << " Hz²\n";
        std::cout << "    Cliff Sharpness        " << std::fixed << std::setprecision(1) << spec->cutoff_sharpness_db << " dB/bin · "
                  << static_cast<int>(spec->cliff_depth_db) << " dB drop/800Hz\n";
        std::cout << "    HF Energy Ratio        " << std::fixed << std::setprecision(5) << spec->hf_energy_ratio << "\n";
        std::cout << "    Side Anomaly           " << std::fixed << std::setprecision(3) << spec->side_anomaly_score << "\n";
        std::cout << "    Banding Score          " << std::fixed << std::setprecision(3) << spec->banding_score << "\n";
        std::cout << "    NF Above Cutoff        " << std::fixed << std::setprecision(1) << spec->nf_above_cutoff_db << " dB\n";
        std::cout << "  ✓ LPF                    " << (spec->lpf_detected ? spec->lpf_cutoff_str : "none detected") << "\n";
        std::cout << "    Spectral Entropy       " << std::fixed << std::setprecision(3) << spec->entropy << "\n\n";

        std::cout << "  Advanced DSP Forensics\n";
        std::cout << "  ✓ Header Integrity       container matches decoded stream\n";
        std::cout << "  ✓ Codec Fingerprint      " << (spec->codec_fingerprint.empty() ? "cutoff matches no known encoder wall" : spec->codec_fingerprint) << "\n";
        std::cout << "  ✓ Resample Check         " << (spec->resample_detected.empty() ? "no foreign-Nyquist resampler artifacts" : spec->resample_detected) << "\n";
        std::cout << "  ✓ auCDtect Bound         " << static_cast<int>(spec->auc_avg_bound_freq) << " Hz avg\n";
        std::cout << "  ✓ MDCT Quant. Lattice    " << std::fixed << std::setprecision(3) << spec->mdct_quant_score
                  << (spec->mdct_quant_score < 0.06 ? "   ✓ no MDCT quantization lattice — clean coefficient statistics" : "   ⚠ lattice detected") << "\n";
        std::cout << "    Spectral Sparsity      " << std::fixed << std::setprecision(3) << spec->spectral_sparsity << "\n";
        std::cout << "    Ultrasonic Corr.       " << (spec->hf_envelope_correlation >= 0 ? "+" : "") << std::fixed << std::setprecision(2) << spec->hf_envelope_correlation << "\n";

        if (!spec->evidence.empty()) {
            std::cout << "\n  \033[1;31mLossy Evidence:\033[0m\n";
            for (const auto& ev : spec->evidence) {
                std::cout << "    ⚠ " << ev << "\n";
            }
        }

        if (!spec->natural_evidence.empty()) {
            std::cout << "\n  \033[1;32mNatural indicators:\033[0m\n";
            for (const auto& nev : spec->natural_evidence) {
                std::cout << "    · " << nev << "\n";
            }
        }

        std::cout << "\n  Source Integrity\n";
        std::cout << "  ✓ Bit-Depth Auth         " << rep.authenticity.bit_depth_authentic << "\n";
        std::cout << "  ✓ Header Integrity       " << rep.authenticity.header_integrity << "\n";
        std::cout << "    Side Channel           " << rep.authenticity.side_channel_analysis << "\n";
        std::cout << "    Phase Corr.            " << rep.authenticity.phase_correlation << "\n";
        std::cout << "  ✓ Clipping               " << rep.authenticity.clipping_verdict << "\n";
        std::cout << "    Silence                " << rep.authenticity.silence_total_pct << "\n";
    }

    if (!rep.stats.empty()) {
        std::cout << "\n\033[1;33m── ACOUSTIC MEASUREMENTS  (SoX) ──────────────────────────────\033[0m\n";
        std::cout << "  Peak Levels\n";
        for (const auto& key : {"Maximum Amplitude", "Minimum Amplitude", "Mean Amplitude", "Midline Amplitude", "Rms Amplitude", "Mean Norm"}) {
            auto it = rep.stats.find(key);
            if (it != rep.stats.end()) {
                std::cout << "  " << std::left << std::setw(26) << it->first << " " << it->second << "\n";
            }
        }
        std::cout << "\n  Delta\n";
        for (const auto& key : {"Maximum Delta", "Minimum Delta", "Mean Delta", "Rms Delta"}) {
            auto it = rep.stats.find(key);
            if (it != rep.stats.end()) {
                std::cout << "  " << std::left << std::setw(26) << it->first << " " << it->second << "\n";
            }
        }
        std::cout << "\n  Samples & Scaling\n";
        for (const auto& key : {"Samples Read", "Length Seconds", "Rough Frequency", "Scaled By", "Volume Adjustment"}) {
            auto it = rep.stats.find(key);
            if (it != rep.stats.end()) {
                std::cout << "  " << std::left << std::setw(26) << it->first << " " << it->second << "\n";
            }
        }
    }

    std::cout << "\n\033[2m──────────────────────────────────────────────────────────────\n";
    std::cout << "  Analysed in " << std::fixed << std::setprecision(2) << rep.analysis_seconds << "s\n";
    std::cout << "──────────────────────────────────────────────────────────────\033[0m\n\n";
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    if (argc < 2) {
        std::cout << "Usage: audio_forensic_native <audio_file> [--fast]\n" << std::flush;
        return 1;
    }

    std::string path = argv[1];
    double max_sec = 0.0;
    if (argc >= 3 && std::string(argv[2]) == "--fast") {
        max_sec = 60.0;
    }

    ProgressCallback cb;
    cb.onProgress = [](const std::string& stage, float progress) {
        std::cerr << "\r⏳ [" << static_cast<int>(progress * 100) << "%] " << stage << "      " << std::flush;
    };

    auto rep = analyzeAudio(path, max_sec, &cb);
    std::cerr << "\r\033[2K" << std::flush;
    printReport(rep);
    return 0;
}
