package com.audioforensic.app.model

import kotlinx.serialization.Serializable

@Serializable
data class AudioTechnical(
    val codec: String = "",
    val sample_encoding: String = "",
    val channels: Int = 2,
    val sample_rate: Int = 44100,
    val precision: Int = 16,
    val duration: String = "",
    val duration_sec: Double = 0.0,
    val bit_rate: String = "",
    val compression_mode: String = "Lossless",
    val writing_library: String = ""
)

@Serializable
data class AudioTags(
    val title: String = "",
    val artist: String = "",
    val album: String = "",
    val album_artist: String = "",
    val date: String = "",
    val replaygain_track_gain: String = "",
    val replaygain_album_gain: String = "",
    val comments: String = "",
    val other: Map<String, String> = emptyMap()
)

@Serializable
data class LoudnessProfile(
    val peak_db: String = "",
    val rms_db: String = "",
    val rms_peak_db: String = "",
    val rms_trough_db: String = "",
    val noise_floor_db: String = "",
    val dynamic_range_db: String = "",
    val crest_factor_db: String = "",
    val flat_factor: String = "",
    val peak_count: String = "",
    val entropy: String = "",
    val dc_offset: String = "",
    val zero_crossings_rate: String = "",
    val lufs_integrated: String = "",
    val lufs_range: String = "",
    val true_peak_dbtp: String = "",
    val apple_music_delta: String = "",
    val spotify_delta: String = ""
)

@Serializable
data class SpectralAnalysis(
    val main_score: Int = 0,
    val verdict_label: String = "UNKNOWN",
    val primary_verdict: String = "",
    val cutoff_hz: Double = 0.0,
    val cutoff_hz_str: String = "",
    val cutoff_variance: Double = 0.0,
    val cutoff_sharpness_db: Double = 0.0,
    val cliff_depth_db: Double = 0.0,
    val hf_energy_ratio: Double = 0.0,
    val banding_score: Double = 0.0,
    val side_anomaly_score: Double = 0.0,
    val nf_above_cutoff_db: Double = 0.0,
    val entropy: Double = 0.0,
    val lpf_detected: Boolean = false,
    val lpf_cutoff_str: String = "",
    val codec_fingerprint: String = "",
    val resample_detected: String = "",
    val auc_avg_bound_freq: Double = 0.0,
    val auc_prob_bound_freq: Double = 0.0,
    val auc_phase_entropy: Double = 0.0,
    val mdct_quant_score: Double = -1.0,
    val spectral_sparsity: Double = 0.0,
    val hf_envelope_correlation: Double = 0.0,
    val evidence: List<String> = emptyList(),
    val natural_evidence: List<String> = emptyList()
)

@Serializable
data class AuthenticityReport(
    val bit_depth_authentic: String = "",
    val phase_correlation: String = "",
    val side_channel_analysis: String = "",
    val clipping_verdict: String = "",
    val clipped_samples: String = "",
    val silence_total_pct: String = "",
    val header_integrity: String = "",
    val silence_sections: List<String> = emptyList(),
    val spectral: SpectralAnalysis? = null
)

@Serializable
data class ForensicReport(
    val filepath: String = "",
    val analysis_seconds: Double = 0.0,
    val file_size_mb: Double = 0.0,
    val dr_score: String = "N/A",
    val technical: AudioTechnical = AudioTechnical(),
    val tags: AudioTags = AudioTags(),
    val loudness: LoudnessProfile = LoudnessProfile(),
    val stats: Map<String, String> = emptyMap(),
    val authenticity: AuthenticityReport? = null
) {
    val fileName: String
        get() = filepath.substringAfterLast('/').substringAfterLast('\\')

    val isLossy: Boolean
        get() = (authenticity?.spectral?.main_score ?: 0) >= 55

    val mainScore: Int
        get() = authenticity?.spectral?.main_score ?: 0

    val verdictLabel: String
        get() = authenticity?.spectral?.verdict_label ?: "UNKNOWN"

    val primaryVerdict: String
        get() = authenticity?.spectral?.primary_verdict ?: "No spectral analysis available"
}
