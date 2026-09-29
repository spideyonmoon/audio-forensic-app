#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <cstdint>

namespace audio_forensic {

enum class VerdictTier {
    GENUINE,         // 0–10
    LIKELY_GENUINE,  // 11–30
    CAUTION,         // 31–54
    SUSPICIOUS,      // 55–85
    LIKELY_LOSSY,    // 86–100
    KNOWN_LOSSY      // Categorical (e.g. MQA, decoded MP3 header)
};

inline std::string verdictTierToString(VerdictTier tier) {
    switch (tier) {
        case VerdictTier::GENUINE: return "GENUINE";
        case VerdictTier::LIKELY_GENUINE: return "LIKELY_GENUINE";
        case VerdictTier::CAUTION: return "CAUTION";
        case VerdictTier::SUSPICIOUS: return "SUSPICIOUS";
        case VerdictTier::LIKELY_LOSSY: return "LIKELY_LOSSY";
        case VerdictTier::KNOWN_LOSSY: return "KNOWN_LOSSY";
    }
    return "UNKNOWN";
}

struct AudioTags {
    std::string title;
    std::string album;
    std::string date;
    std::string album_artist;
    std::string artist;
    std::string bpm;
    std::string comment_quality;
    std::string comments;
    std::string replaygain_track_gain;
    std::string replaygain_album_gain;
    std::map<std::string, std::string> other;
};

struct AudioTechnical {
    std::string bit_rate;
    int channels = 2;
    int precision = 16;
    int sample_rate = 44100;
    std::string sample_encoding;
    std::string duration;
    double duration_sec = 0.0;
    std::string writing_library;
    std::string format_profile;
    std::string compression_mode;
    std::string codec;
};

struct LoudnessProfile {
    std::string peak_db;
    std::string rms_db;
    std::string rms_peak_db;
    std::string rms_trough_db;
    std::string noise_floor_db;
    std::string dynamic_range_db;
    std::string crest_factor_db;
    std::string flat_factor;
    std::string peak_count;
    std::string entropy;
    std::string dc_offset;
    std::string zero_crossings_rate;
    std::string lufs_integrated;
    std::string lufs_range;
    std::string true_peak_dbtp;
    std::string lufs_momentary_max;
    std::string lufs_shortterm_max;
    std::string apple_music_delta;
    std::string spotify_delta;
};

struct SegmentResult {
    double offset_sec = 0.0;
    double cutoff_hz = 0.0;
    double cliff_db = 0.0;
    double void_rel_db = 0.0;
    double peak_db = 0.0;
};

struct SpectralAnalysis {
    double cutoff_hz = 0.0;
    std::string cutoff_hz_str;
    double cutoff_variance = 0.0;
    std::string cutoff_variance_interp;
    double cutoff_sharpness_db = 0.0;
    std::string cutoff_sharpness_interp;
    double cliff_depth_db = 0.0;
    double hf_energy_ratio = 0.0;
    std::string hf_energy_interp;
    double banding_score = 0.0;
    std::string banding_interp;
    double nf_above_cutoff_db = 0.0;
    std::string nf_interp;
    double side_anomaly_score = 0.0;
    std::string side_interp;
    double entropy = 0.0;
    std::string entropy_interp;
    bool lpf_detected = false;
    std::string lpf_cutoff_str;
    bool dsd_detected = false;
    int lossy_score = 0;
    int natural_score = 0;
    int net_score = 0;
    int max_score = 14;
    double raw_lossy_pct = 0.0;
    double net_confidence_pct = 0.0;
    int main_score = 0;
    std::string known_lossy_codec;
    std::string verdict_label;
    std::string primary_verdict;
    std::vector<std::string> evidence;
    std::vector<std::string> natural_evidence;
    std::vector<std::string> caveats;

    // Advanced DSP suite
    double spectral_sparsity = 0.0;
    std::string sparsity_interp;
    double hf_envelope_correlation = 0.0;
    std::string hf_env_corr_interp;
    double preecho_pct = 0.0;
    double aliasing_corr = 0.0;
    bool mp3_noise_pattern_detected = false;
    int cassette_score = 0;
    double silence_ratio = -1.0;
    bool vinyl_noise_detected = false;
    double vinyl_clicks_per_min = 0.0;
    bool header_duration_mismatch = false;
    bool header_bitrate_mismatch = false;
    int segment_walled = -1;
    int segment_total = 0;
    double segment_wall_hz = 16500.0;
    std::vector<SegmentResult> segments;
    std::string codec_fingerprint;
    std::string resample_detected;
    int resample_src_rate = 0;
    std::string fake_hires;
    double auc_avg_bound_freq = 0.0;
    std::string auc_bound_interp;
    double auc_prob_bound_freq = 0.0;
    double auc_phase_entropy = 0.0;
    std::string auc_phase_interp;
    double mdct_quant_score = -1.0;
    std::string mdct_quant_interp = "not evaluated";
    double vorbis_grid_score = -1.0;
    int vorbis_grid_support = 0;
    int vorbis_grid_tested = 0;
    std::string vorbis_grid_channel;
    std::string vorbis_grid_interp = "not evaluated";
};

struct MQADetection {
    bool detected = false;
    bool metadata_claimed = false;
    std::optional<bool> studio;
    int original_sample_rate = 0;
    int bit_plane = -1;
    int sync_sample = -1;
    std::string error;
};

struct BitDepthProfile {
    int effective_bits = 0;
    double floor_db = 0.0;
    double peak_db = 0.0;
    bool flat = false;
    double slope_db = 0.0;
    std::string verdict;
};

struct AuthenticityReport {
    std::optional<SpectralAnalysis> spectral;
    std::string spectral_cutoff_hz;
    std::string spectral_cutoff_verdict;
    bool lpf_detected = false;
    std::string lpf_cutoff_hz;
    std::string bit_depth_authentic;
    std::string phase_correlation;
    std::string phase_verdict;
    std::string clipped_samples;
    std::string clipping_verdict;
    std::string silence_total_pct;
    std::vector<std::string> silence_sections;
    std::string rg_stored;
    std::string rg_measured_lufs;
    std::string rg_delta;
    std::string rg_verdict;
    bool cassette_rip_detected = false;
    bool vinyl_rip_detected = false;
    MQADetection mqa;
    std::string side_channel_analysis;
    std::string header_integrity;
    std::string encoder_trace;
};

struct ForensicReport {
    std::string filepath;
    AudioTags tags;
    AudioTechnical technical;
    std::map<std::string, std::string> stats;
    LoudnessProfile loudness;
    AuthenticityReport authenticity;
    std::string dr_score = "N/A";
    std::string spectrogram_path;
    double analysis_seconds = 0.0;
    double file_size_mb = 0.0;
};

} // namespace audio_forensic
