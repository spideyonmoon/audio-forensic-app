#include "spectral_engine.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>

using namespace audio_forensic;
namespace fs = std::filesystem;

void printReport(const ForensicReport& rep) {
    const auto& spec = rep.authenticity.spectral;
    std::cout << "\n\033[1;33m── Audio Forensic Report ──────────────────────────────────\033[0m\n";
    std::cout << "\033[1;36mFile:\033[0m        " << rep.filepath << "\n";
    std::cout << "\033[1;36mContainer:\033[0m   " << rep.technical.codec << ", "
              << rep.technical.sample_rate << " Hz, " << rep.technical.precision << "-bit, "
              << rep.technical.channels << " ch\n";
    std::cout << "\033[1;36mDuration:\033[0m    " << std::fixed << std::setprecision(2) << rep.technical.duration_sec << " s\n";

    if (spec) {
        std::cout << "\n\033[1;33m── Authenticity Analysis ──────────────────────────────────\033[0m\n";
        std::cout << "\033[1mMain Score:\033[0m  " << spec->main_score << " / 100 (" << spec->verdict_label << ")\n";
        std::cout << "\033[1mVerdict:\033[0m     " << spec->primary_verdict << "\n";
        std::cout << "\033[1mCutoff:\033[0m      " << spec->cutoff_hz_str << "\n";

        if (!spec->codec_fingerprint.empty()) {
            std::cout << "\033[1;31mFingerprint:\033[0m " << spec->codec_fingerprint << "\n";
        }
        if (!spec->resample_detected.empty()) {
            std::cout << "\033[1;31mResample:\033[0m    " << spec->resample_detected << "\n";
        }

        if (!spec->evidence.empty()) {
            std::cout << "\n\033[1;31mEvidence of Transcoding / Modification:\033[0m\n";
            for (const auto& ev : spec->evidence) {
                std::cout << "  ✗ " << ev << "\n";
            }
        }

        if (!spec->natural_evidence.empty()) {
            std::cout << "\n\033[1;32mNatural Audio Characteristics:\033[0m\n";
            for (const auto& nev : spec->natural_evidence) {
                std::cout << "  ✓ " << nev << "\n";
            }
        }
    }
    std::cout << "\n\033[2mAnalysis completed in " << std::fixed << std::setprecision(3)
              << rep.analysis_seconds << "s\033[0m\n\n";
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
