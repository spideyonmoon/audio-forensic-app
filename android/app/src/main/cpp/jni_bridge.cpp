#include <jni.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "spectral_engine.hpp"

using namespace audio_forensic;

// Helper to escape JSON strings
static std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (static_cast<unsigned char>(c) <= 0x1f) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            o << c;
        }
    }
    return o.str();
}

static std::string reportToJson(const ForensicReport& rep) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"filepath\": \"" << escapeJson(rep.filepath) << "\",\n";
    ss << "  \"analysis_seconds\": " << rep.analysis_seconds << ",\n";
    ss << "  \"file_size_mb\": " << rep.file_size_mb << ",\n";
    ss << "  \"dr_score\": \"" << escapeJson(rep.dr_score) << "\",\n";

    // 1. Technical
    ss << "  \"technical\": {\n";
    ss << "    \"codec\": \"" << escapeJson(rep.technical.codec) << "\",\n";
    ss << "    \"sample_encoding\": \"" << escapeJson(rep.technical.sample_encoding) << "\",\n";
    ss << "    \"channels\": " << rep.technical.channels << ",\n";
    ss << "    \"sample_rate\": " << rep.technical.sample_rate << ",\n";
    ss << "    \"precision\": " << rep.technical.precision << ",\n";
    ss << "    \"duration\": \"" << escapeJson(rep.technical.duration) << "\",\n";
    ss << "    \"duration_sec\": " << rep.technical.duration_sec << ",\n";
    ss << "    \"bit_rate\": \"" << escapeJson(rep.technical.bit_rate) << "\",\n";
    ss << "    \"compression_mode\": \"" << escapeJson(rep.technical.compression_mode) << "\",\n";
    ss << "    \"writing_library\": \"" << escapeJson(rep.technical.writing_library) << "\"\n";
    ss << "  },\n";

    // 2. Tags
    ss << "  \"tags\": {\n";
    ss << "    \"title\": \"" << escapeJson(rep.tags.title) << "\",\n";
    ss << "    \"artist\": \"" << escapeJson(rep.tags.artist) << "\",\n";
    ss << "    \"album\": \"" << escapeJson(rep.tags.album) << "\",\n";
    ss << "    \"album_artist\": \"" << escapeJson(rep.tags.album_artist) << "\",\n";
    ss << "    \"date\": \"" << escapeJson(rep.tags.date) << "\",\n";
    ss << "    \"replaygain_track_gain\": \"" << escapeJson(rep.tags.replaygain_track_gain) << "\",\n";
    ss << "    \"replaygain_album_gain\": \"" << escapeJson(rep.tags.replaygain_album_gain) << "\",\n";
    ss << "    \"comments\": \"" << escapeJson(rep.tags.comments) << "\",\n";
    ss << "    \"other\": {\n";
    size_t tag_idx = 0;
    for (const auto& [k, v] : rep.tags.other) {
        ss << "      \"" << escapeJson(k) << "\": \"" << escapeJson(v) << "\""
           << (++tag_idx < rep.tags.other.size() ? ",\n" : "\n");
    }
    ss << "    }\n";
    ss << "  },\n";

    // 3. Loudness
    ss << "  \"loudness\": {\n";
    ss << "    \"peak_db\": \"" << escapeJson(rep.loudness.peak_db) << "\",\n";
    ss << "    \"rms_db\": \"" << escapeJson(rep.loudness.rms_db) << "\",\n";
    ss << "    \"rms_peak_db\": \"" << escapeJson(rep.loudness.rms_peak_db) << "\",\n";
    ss << "    \"rms_trough_db\": \"" << escapeJson(rep.loudness.rms_trough_db) << "\",\n";
    ss << "    \"noise_floor_db\": \"" << escapeJson(rep.loudness.noise_floor_db) << "\",\n";
    ss << "    \"dynamic_range_db\": \"" << escapeJson(rep.loudness.dynamic_range_db) << "\",\n";
    ss << "    \"crest_factor_db\": \"" << escapeJson(rep.loudness.crest_factor_db) << "\",\n";
    ss << "    \"flat_factor\": \"" << escapeJson(rep.loudness.flat_factor) << "\",\n";
    ss << "    \"peak_count\": \"" << escapeJson(rep.loudness.peak_count) << "\",\n";
    ss << "    \"entropy\": \"" << escapeJson(rep.loudness.entropy) << "\",\n";
    ss << "    \"dc_offset\": \"" << escapeJson(rep.loudness.dc_offset) << "\",\n";
    ss << "    \"zero_crossings_rate\": \"" << escapeJson(rep.loudness.zero_crossings_rate) << "\",\n";
    ss << "    \"lufs_integrated\": \"" << escapeJson(rep.loudness.lufs_integrated) << "\",\n";
    ss << "    \"lufs_range\": \"" << escapeJson(rep.loudness.lufs_range) << "\",\n";
    ss << "    \"true_peak_dbtp\": \"" << escapeJson(rep.loudness.true_peak_dbtp) << "\",\n";
    ss << "    \"apple_music_delta\": \"" << escapeJson(rep.loudness.apple_music_delta) << "\",\n";
    ss << "    \"spotify_delta\": \"" << escapeJson(rep.loudness.spotify_delta) << "\"\n";
    ss << "  },\n";

    // 4. SoX Stats
    ss << "  \"stats\": {\n";
    size_t stat_idx = 0;
    for (const auto& [k, v] : rep.stats) {
        ss << "    \"" << escapeJson(k) << "\": \"" << escapeJson(v) << "\""
           << (++stat_idx < rep.stats.size() ? ",\n" : "\n");
    }
    ss << "  },\n";

    // 5. Authenticity & Spectral
    ss << "  \"authenticity\": {\n";
    ss << "    \"bit_depth_authentic\": \"" << escapeJson(rep.authenticity.bit_depth_authentic) << "\",\n";
    ss << "    \"phase_correlation\": \"" << escapeJson(rep.authenticity.phase_correlation) << "\",\n";
    ss << "    \"side_channel_analysis\": \"" << escapeJson(rep.authenticity.side_channel_analysis) << "\",\n";
    ss << "    \"clipping_verdict\": \"" << escapeJson(rep.authenticity.clipping_verdict) << "\",\n";
    ss << "    \"clipped_samples\": \"" << escapeJson(rep.authenticity.clipped_samples) << "\",\n";
    ss << "    \"silence_total_pct\": \"" << escapeJson(rep.authenticity.silence_total_pct) << "\",\n";
    ss << "    \"header_integrity\": \"" << escapeJson(rep.authenticity.header_integrity) << "\",\n";
    ss << "    \"silence_sections\": [";
    for (size_t i = 0; i < rep.authenticity.silence_sections.size(); ++i) {
        ss << "\"" << escapeJson(rep.authenticity.silence_sections[i]) << "\""
           << (i + 1 < rep.authenticity.silence_sections.size() ? ", " : "");
    }
    ss << "]";

    if (rep.authenticity.spectral) {
        const auto& s = *rep.authenticity.spectral;
        ss << ",\n    \"spectral\": {\n";
        ss << "      \"main_score\": " << s.main_score << ",\n";
        ss << "      \"verdict_label\": \"" << escapeJson(s.verdict_label) << "\",\n";
        ss << "      \"primary_verdict\": \"" << escapeJson(s.primary_verdict) << "\",\n";
        ss << "      \"cutoff_hz\": " << s.cutoff_hz << ",\n";
        ss << "      \"cutoff_hz_str\": \"" << escapeJson(s.cutoff_hz_str) << "\",\n";
        ss << "      \"cutoff_variance\": " << s.cutoff_variance << ",\n";
        ss << "      \"cutoff_sharpness_db\": " << s.cutoff_sharpness_db << ",\n";
        ss << "      \"cliff_depth_db\": " << s.cliff_depth_db << ",\n";
        ss << "      \"hf_energy_ratio\": " << s.hf_energy_ratio << ",\n";
        ss << "      \"banding_score\": " << s.banding_score << ",\n";
        ss << "      \"side_anomaly_score\": " << s.side_anomaly_score << ",\n";
        ss << "      \"nf_above_cutoff_db\": " << s.nf_above_cutoff_db << ",\n";
        ss << "      \"entropy\": " << s.entropy << ",\n";
        ss << "      \"lpf_detected\": " << (s.lpf_detected ? "true" : "false") << ",\n";
        ss << "      \"lpf_cutoff_str\": \"" << escapeJson(s.lpf_cutoff_str) << "\",\n";
        ss << "      \"codec_fingerprint\": \"" << escapeJson(s.codec_fingerprint) << "\",\n";
        ss << "      \"resample_detected\": \"" << escapeJson(s.resample_detected) << "\",\n";
        ss << "      \"auc_avg_bound_freq\": " << s.auc_avg_bound_freq << ",\n";
        ss << "      \"auc_prob_bound_freq\": " << s.auc_prob_bound_freq << ",\n";
        ss << "      \"auc_phase_entropy\": " << s.auc_phase_entropy << ",\n";
        ss << "      \"mdct_quant_score\": " << s.mdct_quant_score << ",\n";
        ss << "      \"spectral_sparsity\": " << s.spectral_sparsity << ",\n";
        ss << "      \"hf_envelope_correlation\": " << s.hf_envelope_correlation << ",\n";

        ss << "      \"evidence\": [";
        for (size_t i = 0; i < s.evidence.size(); ++i) {
            ss << "\"" << escapeJson(s.evidence[i]) << "\"" << (i + 1 < s.evidence.size() ? ", " : "");
        }
        ss << "],\n";

        ss << "      \"natural_evidence\": [";
        for (size_t i = 0; i < s.natural_evidence.size(); ++i) {
            ss << "\"" << escapeJson(s.natural_evidence[i]) << "\"" << (i + 1 < s.natural_evidence.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "    }\n";
    } else {
        ss << "\n";
    }
    ss << "  }\n";
    ss << "}";
    return ss.str();
}

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_audioforensic_app_jni_NativeBridge_analyzeFileNative(
    JNIEnv* env,
    jobject /* thiz */,
    jstring jFilePath,
    jdouble maxSeconds,
    jobject jCallback) {

    const char* cPath = env->GetStringUTFChars(jFilePath, nullptr);
    std::string path(cPath);
    env->ReleaseStringUTFChars(jFilePath, cPath);

    ProgressCallback cb;
    jclass cbClass = nullptr;
    jmethodID onProgressMid = nullptr;

    if (jCallback != nullptr) {
        cbClass = env->GetObjectClass(jCallback);
        if (cbClass != nullptr) {
            onProgressMid = env->GetMethodID(cbClass, "onProgress", "(Ljava/lang/String;F)V");
        }
        cb.onProgress = [&](const std::string& stage, float progress) {
            if (cbClass != nullptr && onProgressMid != nullptr) {
                jstring jStage = env->NewStringUTF(stage.c_str());
                env->CallVoidMethod(jCallback, onProgressMid, jStage, static_cast<jfloat>(progress));
                env->DeleteLocalRef(jStage);
            }
        };
    }

    auto report = analyzeAudio(path, maxSeconds, (jCallback != nullptr) ? &cb : nullptr);
    std::string jsonStr = reportToJson(report);

    return env->NewStringUTF(jsonStr.c_str());
}

JNIEXPORT jintArray JNICALL
Java_com_audioforensic_app_jni_NativeBridge_generateSpectrogramNative(
    JNIEnv* env,
    jobject /* thiz */,
    jstring jFilePath,
    jint targetWidth,
    jint targetHeight) {

    const char* cPath = env->GetStringUTFChars(jFilePath, nullptr);
    std::string path(cPath);
    env->ReleaseStringUTFChars(jFilePath, cPath);

    DecodedAudio audio;
    if (!decodeAudioFile(path, audio, 60.0)) {
        return nullptr;
    }

    size_t width = static_cast<size_t>(std::max(64, targetWidth));
    size_t height = static_cast<size_t>(std::max(64, targetHeight));
    std::vector<int32_t> pixels(width * height, 0xFF000000);

    auto colormap = [](float val) -> uint32_t {
        val = std::clamp(val, 0.0f, 1.0f);
        uint8_t r = static_cast<uint8_t>(std::clamp(std::sin(val * 3.14159f * 0.8f) * 255.0f, 0.0f, 255.0f));
        uint8_t g = static_cast<uint8_t>(std::clamp(std::sin(val * 3.14159f) * 220.0f, 0.0f, 255.0f));
        uint8_t b = static_cast<uint8_t>(std::clamp(std::cos(val * 3.14159f * 0.5f) * 255.0f, 0.0f, 255.0f));
        return (0xFF000000) | (r << 16) | (g << 8) | b;
    };

    size_t step = audio.mid.size() / width;
    if (step >= 512) {
        for (size_t x = 0; x < width; ++x) {
            size_t off = x * step;
            std::vector<double> block(512);
            for (size_t i = 0; i < 512 && off + i < audio.mid.size(); ++i) {
                block[i] = audio.mid[off + i];
            }
            auto spec = rfft(block);
            for (size_t y = 0; y < height; ++y) {
                size_t bin = (height - 1 - y) * (spec.size() - 1) / height;
                float mag = static_cast<float>(std::abs(spec[bin]));
                float db = 20.0f * std::log10(mag + 1e-6f);
                float norm = (db + 80.0f) / 80.0f;
                pixels[y * width + x] = colormap(norm);
            }
        }
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(pixels.size()));
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(pixels.size()), reinterpret_cast<const jint*>(pixels.data()));
    return result;
}

} // extern "C"
