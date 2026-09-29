#pragma once

#include "types.hpp"
#include "dsp_math.hpp"
#include "audio_decoder.hpp"
#include <functional>
#include <memory>

namespace audio_forensic {

struct ProgressCallback {
    std::function<void(const std::string& stage, float progress)> onProgress;
};

class SpectralEngine {
public:
    static constexpr size_t WINDOW = 4096;
    static constexpr size_t HOP = 2048;
    static constexpr double CUTOFF_DB = -65.0;
    static constexpr double NYQUIST_MARGIN = 0.85;

    static constexpr int SCORE_CUTOFF_WELL_BELOW_NYQUIST = 2;
    static constexpr int SCORE_SHARP_CLIFF_HARD = 3;
    static constexpr int SCORE_SHARP_CLIFF_SOFT = 1;
    static constexpr int SCORE_HF_NEAR_ZERO = 1;
    static constexpr int SCORE_VOID_ABOVE_CUTOFF = 3;
    static constexpr int SCORE_QUIET_ABOVE_CUTOFF = 1;
    static constexpr int SCORE_VERY_STABLE_CUTOFF = 1;
    static constexpr int SCORE_BANDING_STRONG = 1;
    static constexpr int SCORE_SIDE_ANOMALY = 2;
    static constexpr int MAX_LOSSY_SCORE = 14;

    static constexpr int NATURAL_GRADUAL_ROLLOFF = 1;
    static constexpr int NATURAL_HIGH_VARIANCE = 1;
    static constexpr int NATURAL_MODERATE_VARIANCE = 1;
    static constexpr int NATURAL_RICH_HF = 1;
    static constexpr int NATURAL_HF_NOISE = 1;
    static constexpr int NATURAL_HEALTHY_SIDE = 1;
    static constexpr int NATURAL_HIGH_ENTROPY = 1;

    static constexpr double CODEC_CEILING_HZ = 22500.0;
    static constexpr double TIME_DOMAIN_CAP_S = 180.0;

    SpectralEngine(int sample_rate, int channels = 2, double claimed_duration = 0.0,
                   int claimed_bitrate_kbps = 0, const std::string& codec = "");

    SpectralAnalysis analyse(const float* mid_data, size_t mid_len,
                             const float* side_data = nullptr, size_t side_len = 0,
                             double max_seconds = 0.0,
                             const ProgressCallback* cb = nullptr);

    // Individual metric routines for unit testing against test_dsp.py
    std::vector<double> cutoffPerFrame(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins) const;
    double sharpness(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double window_hz = 2500.0) const;
    double cliffDepth(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double span_hz = 400.0) const;
    double hfEnergyRatio(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double threshold_hz = 15000.0) const;
    double bandingScore(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double scan_hz = 1500.0) const;
    double noiseFloorAboveCutoff(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz) const;
    double sideChannelAnomaly(const std::vector<std::vector<double>>& mid_frames, const float* side, size_t side_len, const std::vector<double>& bins) const;

    void aucdtectFeatures(const std::vector<std::vector<double>>& frames,
                         const std::vector<std::vector<double>>& phase_hi,
                         const std::vector<double>& bins,
                         double& avg_bound, double& prob_bound, double& phase_entropy) const;

    double mdctQuantError(const float* audio, size_t len, const float* side = nullptr, size_t side_len = 0) const;
    void vorbisGrid(const float* mid, size_t len, const float* side, size_t side_len,
                    double& score, int& support, int& tested, std::string& channel) const;

    std::vector<double> freqBins() const;

private:
    int sample_rate_;
    double nyquist_;
    int channels_;
    double claimed_duration_;
    int claimed_bitrate_kbps_;
    std::string codec_;
    bool native_dsd_;

    void computeSTFT(const float* audio, size_t len, size_t hop,
                     std::vector<std::vector<double>>& mags,
                     std::vector<std::vector<double>>& phases_hi,
                     size_t& hi_start) const;
};

// Top-level forensic report builder
ForensicReport analyzeAudio(const std::string& filepath, double max_seconds = 0.0, const ProgressCallback* cb = nullptr);

} // namespace audio_forensic
