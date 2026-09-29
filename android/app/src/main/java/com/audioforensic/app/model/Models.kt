package com.audioforensic.app.model

import kotlinx.serialization.Serializable

@Serializable
data class AudioTechnical(
    val codec: String = "",
    val channels: Int = 2,
    val sample_rate: Int = 44100,
    val precision: Int = 16,
    val duration_sec: Double = 0.0
)

@Serializable
data class AuthenticityReport(
    val main_score: Int = 0,
    val verdict_label: String = "UNKNOWN",
    val primary_verdict: String = "",
    val cutoff_hz: Double = 0.0,
    val cutoff_hz_str: String = "",
    val cliff_depth_db: Double = 0.0,
    val hf_energy_ratio: Double = 0.0,
    val banding_score: Double = 0.0,
    val side_anomaly_score: Double = 0.0,
    val codec_fingerprint: String = "",
    val resample_detected: String = "",
    val auc_avg_bound_freq: Double = 0.0,
    val auc_phase_entropy: Double = 0.0,
    val mdct_quant_score: Double = -1.0,
    val evidence: List<String> = emptyList(),
    val natural_evidence: List<String> = emptyList()
)

@Serializable
data class ForensicReport(
    val filepath: String = "",
    val analysis_seconds: Double = 0.0,
    val technical: AudioTechnical = AudioTechnical(),
    val authenticity: AuthenticityReport? = null
) {
    val fileName: String
        get() = filepath.substringAfterLast('/').substringAfterLast('\\')

    val isLossy: Boolean
        get() = (authenticity?.main_score ?: 0) >= 55
}
