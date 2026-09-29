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
    ss << "  \"technical\": {\n";
    ss << "    \"codec\": \"" << escapeJson(rep.technical.codec) << "\",\n";
    ss << "    \"channels\": " << rep.technical.channels << ",\n";
    ss << "    \"sample_rate\": " << rep.technical.sample_rate << ",\n";
    ss << "    \"precision\": " << rep.technical.precision << ",\n";
    ss << "    \"duration_sec\": " << rep.technical.duration_sec << "\n";
    ss << "  }";

    if (rep.authenticity.spectral) {
        const auto& s = *rep.authenticity.spectral;
        ss << ",\n  \"authenticity\": {\n";
        ss << "    \"main_score\": " << s.main_score << ",\n";
        ss << "    \"verdict_label\": \"" << escapeJson(s.verdict_label) << "\",\n";
        ss << "    \"primary_verdict\": \"" << escapeJson(s.primary_verdict) << "\",\n";
        ss << "    \"cutoff_hz\": " << s.cutoff_hz << ",\n";
        ss << "    \"cutoff_hz_str\": \"" << escapeJson(s.cutoff_hz_str) << "\",\n";
        ss << "    \"cliff_depth_db\": " << s.cliff_depth_db << ",\n";
        ss << "    \"hf_energy_ratio\": " << s.hf_energy_ratio << ",\n";
        ss << "    \"banding_score\": " << s.banding_score << ",\n";
        ss << "    \"side_anomaly_score\": " << s.side_anomaly_score << ",\n";
        ss << "    \"codec_fingerprint\": \"" << escapeJson(s.codec_fingerprint) << "\",\n";
        ss << "    \"resample_detected\": \"" << escapeJson(s.resample_detected) << "\",\n";
        ss << "    \"auc_avg_bound_freq\": " << s.auc_avg_bound_freq << ",\n";
        ss << "    \"auc_phase_entropy\": " << s.auc_phase_entropy << ",\n";
        ss << "    \"mdct_quant_score\": " << s.mdct_quant_score << ",\n";

        ss << "    \"evidence\": [";
        for (size_t i = 0; i < s.evidence.size(); ++i) {
            ss << "\"" << escapeJson(s.evidence[i]) << "\"" << (i + 1 < s.evidence.size() ? ", " : "");
        }
        ss << "],\n";

        ss << "    \"natural_evidence\": [";
        for (size_t i = 0; i < s.natural_evidence.size(); ++i) {
            ss << "\"" << escapeJson(s.natural_evidence[i]) << "\"" << (i + 1 < s.natural_evidence.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "  }\n";
    }
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
    }

    if (onProgressMid != nullptr) {
        cb.onProgress = [env, jCallback, onProgressMid](const std::string& stage, float progress) {
            jstring jStage = env->NewStringUTF(stage.c_str());
            env->CallVoidMethod(jCallback, onProgressMid, jStage, static_cast<jfloat>(progress));
            env->DeleteLocalRef(jStage);
        };
    }

    ForensicReport report = analyzeAudio(path, maxSeconds, onProgressMid ? &cb : nullptr);
    std::string json = reportToJson(report);
    return env->NewStringUTF(json.c_str());
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
    if (!decodeAudioFile(path, audio, 60.0)) { // 60s probe for spectrogram thumbnail
        return nullptr;
    }

    size_t width = static_cast<size_t>(std::max(64, targetWidth));
    size_t height = static_cast<size_t>(std::max(64, targetHeight));
    std::vector<int32_t> pixels(width * height, 0xFF000000); // ARGB

    // Turbo-style colormap mapping
    auto colormap = [](float val) -> uint32_t {
        val = std::clamp(val, 0.0f, 1.0f);
        uint8_t r = static_cast<uint8_t>(std::clamp(std::sin(val * 3.14159f * 0.8f) * 255.0f, 0.0f, 255.0f));
        uint8_t g = static_cast<uint8_t>(std::clamp(std::sin(val * 3.14159f) * 220.0f, 0.0f, 255.0f));
        uint8_t b = static_cast<uint8_t>(std::clamp(std::cos(val * 3.14159f * 0.5f) * 255.0f, 0.0f, 255.0f));
        return (0xFF000000) | (r << 16) | (g << 8) | b;
    };

    // Render FFT magnitudes across dimensions
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
                float norm = (db + 80.0f) / 80.0f; // -80 dBFS to 0 dBFS
                pixels[y * width + x] = colormap(norm);
            }
        }
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(pixels.size()));
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(pixels.size()), reinterpret_cast<const jint*>(pixels.data()));
    return result;
}

} // extern "C"
