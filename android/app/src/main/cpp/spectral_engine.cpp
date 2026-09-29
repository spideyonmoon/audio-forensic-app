#include "spectral_engine.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>

namespace audio_forensic {

struct CodecWall {
    const char* codec;
    const char* profile;
    int wall_hz;
    int tol_hz;
};

static const CodecWall CODEC_WALLS[] = {
    {"MP3 (LAME)", "320 kbps", 20220, 150}, {"MP3 (LAME)", "320 kbps @48k", 20510, 150},
    {"MP3 (LAME)", "256 kbps", 19530, 150}, {"MP3 (LAME)", "256 kbps @48k", 19760, 150},
    {"MP3 (LAME)", "192 kbps", 18840, 150}, {"MP3 (LAME)", "192 kbps @48k", 19010, 150},
    {"MP3 (LAME)", "160 kbps", 17460, 150}, {"MP3 (LAME)", "128 kbps", 16770, 150},
    {"MP3 (LAME)", "96 kbps", 15410, 150},  {"MP3 (LAME)", "64 kbps", 11270, 250},
    {"AAC", "~192 kbps", 19350, 150},       {"AAC", "~192 kbps @48k", 19560, 150},
    {"AAC", "~128 kbps", 17280, 150},       {"AAC", "~96 kbps", 15860, 180},
    {"Vorbis", "q4 (~128 kbps)", 19000, 150}, {"Vorbis", "q4 @48k", 19180, 150},
    {"Vorbis", "q2 (~96 kbps)", 16575, 150},
    {"Opus", "CELT 20 kHz band limit (any bitrate)", 20460, 260}
};

static const int RESAMPLE_RATES[] = {44100, 48000, 88200, 96000};

// AAC scalefactor bands (long window 1024 coefficients)
static const int SWB_LONG_44_48[] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108,
    120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448,
    480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 1024
};

SpectralEngine::SpectralEngine(int sample_rate, int channels, double claimed_duration,
                               int claimed_bitrate_kbps, const std::string& codec)
    : sample_rate_(sample_rate), nyquist_(sample_rate / 2.0), channels_(channels),
      claimed_duration_(claimed_duration), claimed_bitrate_kbps_(claimed_bitrate_kbps),
      codec_(codec) {
    native_dsd_ = (codec.find("DSD") != std::string::npos);
}

std::vector<double> SpectralEngine::freqBins() const {
    size_t n_bins = WINDOW / 2 + 1;
    std::vector<double> bins(n_bins);
    double step = (sample_rate_ / 2.0) / static_cast<double>(n_bins - 1);
    for (size_t i = 0; i < n_bins; ++i) {
        bins[i] = i * step;
    }
    return bins;
}

void SpectralEngine::computeSTFT(const float* audio, size_t len, size_t hop,
                                std::vector<std::vector<double>>& mags,
                                std::vector<std::vector<double>>& phases_hi,
                                size_t& hi_start) const {
    if (len < WINDOW) return;
    size_t n_frames = (len - WINDOW + hop) / hop;
    auto win = hanningWindow(WINDOW);
    auto bins = freqBins();
    double bin_hz = bins[1] - bins[0];
    hi_start = std::min(bins.size() - 1, static_cast<size_t>(10000.0 / bin_hz));

    mags.resize(n_frames);
    phases_hi.resize(n_frames);

    std::vector<double> block(WINDOW);
    for (size_t i = 0; i < n_frames; ++i) {
        size_t off = i * hop;
        for (size_t j = 0; j < WINDOW; ++j) {
            block[j] = static_cast<double>(audio[off + j]) * win[j];
        }
        auto spec = rfft(block);
        mags[i].resize(spec.size());
        for (size_t k = 0; k < spec.size(); ++k) {
            mags[i][k] = std::abs(spec[k]);
        }
        phases_hi[i].resize(spec.size() - hi_start);
        for (size_t k = hi_start; k < spec.size(); ++k) {
            phases_hi[i][k - hi_start] = std::arg(spec[k]);
        }
    }
}

std::vector<double> SpectralEngine::cutoffPerFrame(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins) const {
    if (frames.empty()) return {};
    size_t n_frames = frames.size();
    size_t n_bins = bins.size();
    std::vector<double> out(n_frames, 0.0);

    for (size_t s = 0; s < n_frames; ++s) {
        const auto& row = frames[s];
        double max_val = 0.0;
        for (double v : row) if (v > max_val) max_val = v;
        double ref = max_val + 1e-12;

        int last_idx = -1;
        for (int k = static_cast<int>(n_bins) - 1; k >= 0; --k) {
            double db = 20.0 * std::log10((row[k] / ref) + 1e-12);
            if (db > CUTOFF_DB) {
                last_idx = k;
                break;
            }
        }
        out[s] = (last_idx >= 0) ? bins[last_idx] : 0.0;
    }
    return out;
}

double SpectralEngine::sharpness(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double window_hz) const {
    if (frames.empty()) return 0.0;
    double bin_hz = bins[1] - bins[0];
    int lo = std::max(0, static_cast<int>((cutoff_hz - window_hz) / bin_hz));
    int hi = std::min(static_cast<int>(bins.size()), static_cast<int>((cutoff_hz + window_hz * 0.25) / bin_hz));
    if (hi <= lo + 1) return 0.0;

    std::vector<double> avg(bins.size(), 0.0);
    for (const auto& row : frames) {
        for (size_t k = 0; k < bins.size(); ++k) avg[k] += row[k];
    }
    double max_avg = 0.0;
    for (double v : avg) if (v > max_avg) max_avg = v;
    double ref = (max_avg / frames.size()) + 1e-12;

    std::vector<double> db(hi - lo);
    for (int k = lo; k < hi; ++k) {
        double val = (avg[k] / frames.size()) / ref;
        db[k - lo] = 20.0 * std::log10(val + 1e-12);
    }

    double max_diff = 0.0;
    for (size_t i = 1; i < db.size(); ++i) {
        double diff = std::abs(db[i] - db[i - 1]);
        if (diff > max_diff) max_diff = diff;
    }
    return max_diff;
}

double SpectralEngine::cliffDepth(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double span_hz) const {
    if (frames.empty()) return 0.0;
    double bin_hz = bins[1] - bins[0];
    int lo = std::max(0, static_cast<int>((cutoff_hz - span_hz) / bin_hz));
    int hi = std::min(static_cast<int>(bins.size() - 1), static_cast<int>((cutoff_hz + span_hz) / bin_hz));
    if (hi <= lo) return 0.0;

    std::vector<double> avg(bins.size(), 0.0);
    for (const auto& row : frames) {
        for (size_t k = 0; k < bins.size(); ++k) avg[k] += row[k];
    }
    double max_avg = 0.0;
    for (double v : avg) if (v > max_avg) max_avg = v;
    double ref = max_avg + 1e-12;

    double db_lo = 20.0 * std::log10((avg[lo] / ref) + 1e-12);
    double db_hi = 20.0 * std::log10((avg[hi] / ref) + 1e-12);
    return db_lo - db_hi;
}

double SpectralEngine::hfEnergyRatio(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double threshold_hz) const {
    if (frames.empty()) return 0.0;
    double bin_hz = bins[1] - bins[0];
    size_t start_bin = static_cast<size_t>(threshold_hz / bin_hz);
    double hf_sum = 0.0, total_sum = 0.0;

    for (const auto& row : frames) {
        for (size_t k = 0; k < row.size(); ++k) {
            total_sum += row[k];
            if (k >= start_bin) hf_sum += row[k];
        }
    }
    return hf_sum / (total_sum + 1e-12);
}

double SpectralEngine::bandingScore(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz, double scan_hz) const {
    if (frames.empty()) return 0.0;
    double bin_hz = bins[1] - bins[0];
    int hi = static_cast<int>(cutoff_hz / bin_hz);
    int lo = std::max(0, hi - static_cast<int>(scan_hz / bin_hz));
    if (hi - lo < 4) return 0.0;

    size_t count_bins = hi - lo;
    size_t n_frames = frames.size();
    std::vector<double> temporal_stds(count_bins, 0.0);

    for (size_t b = 0; b < count_bins; ++b) {
        int bin_idx = lo + b;
        double sum = 0.0;
        for (size_t f = 0; f < n_frames; ++f) {
            sum += frames[f][bin_idx];
        }
        double mean = sum / n_frames;
        double var = 0.0;
        for (size_t f = 0; f < n_frames; ++f) {
            double diff = frames[f][bin_idx] - mean;
            var += diff * diff;
        }
        temporal_stds[b] = std::sqrt(var / n_frames);
    }

    double avg_std = 0.0;
    for (double s : temporal_stds) avg_std += s;
    avg_std /= count_bins;

    return std::clamp(1.0 - (avg_std / 15.0), 0.0, 1.0);
}

double SpectralEngine::noiseFloorAboveCutoff(const std::vector<std::vector<double>>& frames, const std::vector<double>& bins, double cutoff_hz) const {
    if (frames.empty()) return -120.0;
    double bin_hz = bins[1] - bins[0];
    size_t start_bin = static_cast<size_t>(cutoff_hz / bin_hz);
    if (start_bin >= bins.size()) return -120.0;

    double sum_sq = 0.0;
    size_t count = 0;
    for (const auto& row : frames) {
        for (size_t k = start_bin; k < row.size(); ++k) {
            sum_sq += row[k] * row[k];
            count++;
        }
    }
    if (count == 0) return -120.0;
    double rms = std::sqrt(sum_sq / count);
    return 20.0 * std::log10(rms + 1e-12);
}

double SpectralEngine::sideChannelAnomaly(const std::vector<std::vector<double>>& mid_frames, const float* side, size_t side_len, const std::vector<double>& bins) const {
    if (!side || side_len < WINDOW * 2 || mid_frames.empty()) return 0.0;
    double bin_hz = bins[1] - bins[0];
    size_t idx_10k = static_cast<size_t>(10000.0 / bin_hz);
    if (idx_10k >= bins.size()) return 0.0;

    std::vector<std::vector<double>> side_mags;
    std::vector<std::vector<double>> side_phase;
    size_t dummy;
    computeSTFT(side, side_len, HOP * 4, side_mags, side_phase, dummy);
    if (side_mags.empty()) return 0.0;

    double mid_hf_sum = 0.0;
    size_t mid_count = 0;
    for (size_t f = 0; f < mid_frames.size(); f += 4) {
        for (size_t k = idx_10k; k < mid_frames[f].size(); ++k) {
            mid_hf_sum += mid_frames[f][k];
            mid_count++;
        }
    }

    double side_hf_sum = 0.0;
    size_t side_count = 0;
    for (const auto& row : side_mags) {
        for (size_t k = idx_10k; k < row.size(); ++k) {
            side_hf_sum += row[k];
            side_count++;
        }
    }

    double mid_mean = mid_hf_sum / (mid_count + 1e-12);
    double side_mean = side_hf_sum / (side_count + 1e-12);
    double ratio = side_mean / (mid_mean + 1e-12);

    if (ratio < 0.02) return 1.0;
    if (ratio < 0.08) return 0.6;
    return 0.0;
}

void SpectralEngine::aucdtectFeatures(const std::vector<std::vector<double>>& frames,
                                     const std::vector<std::vector<double>>& phase_hi,
                                     const std::vector<double>& bins,
                                     double& avg_bound, double& prob_bound, double& phase_entropy) const {
    avg_bound = 0.0;
    prob_bound = 0.0;
    phase_entropy = 0.0;
    if (frames.size() < 4 || bins.size() < 10) return;

    size_t stride = (frames.size() > 2500) ? (frames.size() / 2500 + 1) : 1;
    std::vector<double> bound_freqs;

    for (size_t f = 0; f < frames.size(); f += stride) {
        const auto& row = frames[f];
        double max_val = 0.0;
        for (double v : row) if (v > max_val) max_val = v;
        double ref = max_val + 1e-12;

        std::vector<double> log_p(row.size());
        for (size_t k = 0; k < row.size(); ++k) {
            double db = 20.0 * std::log10((row[k] / ref) + 1e-12);
            log_p[k] = std::max(db, -110.0);
        }

        auto m1 = uniformFilter1D(log_p, 5);
        std::vector<double> log_p2(row.size());
        for (size_t k = 0; k < row.size(); ++k) log_p2[k] = log_p[k] * log_p[k];
        auto m2 = uniformFilter1D(log_p2, 5);

        std::vector<double> scatter(row.size());
        double max_sc = 0.0;
        for (size_t k = 0; k < row.size(); ++k) {
            double var = std::max(0.0, m2[k] - m1[k] * m1[k]);
            scatter[k] = std::sqrt(var);
        }
        scatter = uniformFilter1D(scatter, 5);
        for (double s : scatter) if (s > max_sc) max_sc = s;

        double thresh = std::min(0.6, max_sc * 0.25);
        int last_organic = 0;
        for (int k = static_cast<int>(row.size()) - 1; k >= 0; --k) {
            if (scatter[k] >= thresh) {
                last_organic = k;
                break;
            }
        }
        bound_freqs.push_back(bins[std::min(static_cast<size_t>(last_organic), bins.size() - 1)]);
    }

    if (!bound_freqs.empty()) {
        double sum_b = 0.0;
        for (double bf : bound_freqs) sum_b += bf;
        avg_bound = sum_b / bound_freqs.size();

        // 20-bin histogram for most probable bound
        int n_hist_bins = std::min(20, static_cast<int>(bins.size()));
        double min_f = bins.front(), max_f = bins.back();
        std::vector<int> hist(n_hist_bins, 0);
        double bin_w = (max_f - min_f) / n_hist_bins;
        for (double bf : bound_freqs) {
            int bidx = std::clamp(static_cast<int>((bf - min_f) / bin_w), 0, n_hist_bins - 1);
            hist[bidx]++;
        }
        int max_bidx = 0;
        for (int i = 1; i < n_hist_bins; ++i) {
            if (hist[i] > hist[max_bidx]) max_bidx = i;
        }
        prob_bound = min_f + (max_bidx + 0.5) * bin_w;
    }

    // High band phase entropy (bins >= 10 kHz)
    if (phase_hi.size() >= 3 && !phase_hi[0].empty()) {
        std::vector<int> hist_p(36, 0);
        for (size_t f = 1; f < phase_hi.size(); ++f) {
            for (size_t k = 0; k < phase_hi[f].size(); ++k) {
                double diff = phase_hi[f][k] - phase_hi[f - 1][k];
                // wrap into [-pi, pi]
                double wrapped = std::atan2(std::sin(diff), std::cos(diff));
                int b = static_cast<int>((wrapped + PI) / (2.0 * PI) * 36.0);
                b = std::clamp(b, 0, 35);
                hist_p[b]++;
            }
        }
        int64_t total = 0;
        for (int c : hist_p) total += c;
        if (total > 0) {
            double ent = 0.0;
            for (int c : hist_p) {
                if (c > 0) {
                    double p = static_cast<double>(c) / total;
                    ent -= p * (std::log(p) / std::log(2.0));
                }
            }
            phase_entropy = ent;
        }
    }
}

double SpectralEngine::mdctQuantError(const float* audio, size_t len, const float* side, size_t side_len) const {
    if (sample_rate_ != 44100 && sample_rate_ != 48000) return -1.0;
    const size_t N2 = 2048;
    const size_t N = 1024;
    if (len < N2 * 4) return -1.0;

    auto win = kbdWindow(N2, 4.0);
    // Find active anchor windows with high energy
    size_t hop = 512;
    size_t n_pos = (len - N2) / hop;
    std::vector<size_t> anchors;
    for (size_t i = 0; i < n_pos; ++i) {
        size_t pos = i * hop;
        double sum_sq = 0.0;
        for (size_t j = 0; j < N2; ++j) {
            double v = audio[pos + j] * 32768.0;
            sum_sq += v * v;
        }
        if (sum_sq > N2 * 100.0) {
            anchors.push_back(pos);
            if (anchors.size() >= 12) break;
        }
    }
    if (anchors.size() < 4) return -1.0;

    double max_score = 0.0;
    // Test long window AAC scalefactor bands
    for (size_t pos : anchors) {
        std::vector<double> block(N2);
        for (size_t j = 0; j < N2; ++j) block[j] = audio[pos + j] * 32768.0 * win[j];

        // Princen-Bradley time-domain aliasing fold
        std::vector<double> folded(N);
        for (size_t i = 0; i < N / 2; ++i) {
            folded[i] = -block[N + N / 2 - 1 - i] - block[N + N / 2 + i];
            folded[N / 2 + i] = block[i] - block[N / 2 - 1 - i];
        }
        auto X = dctIV(folded);

        // Check power-law integer rounding
        int bands_supported = 0;
        for (int b = 0; b < 49; ++b) {
            int start = SWB_LONG_44_48[b];
            int end = SWB_LONG_44_48[b + 1];
            double max_b = 0.0;
            for (int k = start; k < end; ++k) max_b = std::max(max_b, std::abs(X[k]));
            if (max_b < 1.0) continue;

            double sdz = 16.0 + (4.0 / 3.0) * (std::log(max_b) / std::log(2.0));
            double s_band = 0.5 * sdz;
            double scale = std::pow(2.0, -3.0 * s_band / 16.0);

            double err_sum = 0.0;
            for (int k = start; k < end; ++k) {
                double xp = std::pow(std::abs(X[k]), 0.75) * scale;
                double eps = std::round(xp) - xp;
                err_sum += eps * eps;
            }
            if (err_sum / (end - start) < 0.1) bands_supported++;
        }
        double frac = static_cast<double>(bands_supported) / 49.0;
        max_score = std::max(max_score, frac);
    }
    return max_score;
}

void SpectralEngine::vorbisGrid(const float* mid, size_t len, const float* side, size_t side_len,
                               double& score, int& support, int& tested, std::string& channel) const {
    score = -1.0;
    support = 0;
    tested = 0;
    channel = "";
    if (sample_rate_ != 44100 && sample_rate_ != 48000) return;
    const size_t size = 2048;
    if (len < size * 8) return;

    // Vorbis sine-of-sine window
    std::vector<double> window(size);
    for (size_t i = 0; i < size; ++i) {
        double v = std::sin(PI * (i + 0.5) / size);
        window[i] = std::sin(0.5 * PI * v * v);
    }

    score = 0.0;
    tested = 12;
    channel = (side && side_len > 0) ? "L/R" : "M";
}

SpectralAnalysis SpectralEngine::analyse(const float* mid_data, size_t mid_len,
                                        const float* side_data, size_t side_len,
                                        double max_seconds,
                                        const ProgressCallback* cb) {
    SpectralAnalysis res;
    if (!mid_data || mid_len < WINDOW) return res;

    auto report_stage = [&](const std::string& st, float p) {
        if (cb && cb->onProgress) cb->onProgress(st, p);
    };

    report_stage("STFT", 0.14f);
    std::vector<std::vector<double>> mags;
    std::vector<std::vector<double>> phases_hi;
    size_t hi_start = 0;
    computeSTFT(mid_data, mid_len, HOP, mags, phases_hi, hi_start);

    report_stage("spectral metrics", 0.24f);
    auto bins = freqBins();
    auto cutoffs = cutoffPerFrame(mags, bins);

    // 95th percentile cutoff
    std::vector<double> sorted_c = cutoffs;
    std::sort(sorted_c.begin(), sorted_c.end());
    double cutoff_hz = sorted_c.empty() ? 0.0 : sorted_c[static_cast<size_t>(sorted_c.size() * 0.95)];

    double cutoff_var = 0.0;
    if (!cutoffs.empty()) {
        double mean_c = std::accumulate(cutoffs.begin(), cutoffs.end(), 0.0) / cutoffs.size();
        for (double c : cutoffs) cutoff_var += (c - mean_c) * (c - mean_c);
        cutoff_var /= cutoffs.size();
    }

    double sh = sharpness(mags, bins, cutoff_hz);
    double cd = cliffDepth(mags, bins, cutoff_hz);
    double hf = hfEnergyRatio(mags, bins);
    double bd = bandingScore(mags, bins, cutoff_hz);
    double nf = noiseFloorAboveCutoff(mags, bins, cutoff_hz);
    double sa = sideChannelAnomaly(mags, side_data, side_len, bins);

    res.cutoff_hz = cutoff_hz;
    res.cutoff_hz_str = std::to_string(static_cast<int>(cutoff_hz)) + " Hz";
    res.cutoff_sharpness_db = sh;
    res.cliff_depth_db = cd;
    res.hf_energy_ratio = hf;
    res.banding_score = bd;
    res.nf_above_cutoff_db = nf;
    res.side_anomaly_score = sa;

    report_stage("resample check", 0.28f);
    for (int rate : RESAMPLE_RATES) {
        double fn = rate / 2.0;
        if (fn + 1200.0 < nyquist_ - 200.0 && std::abs(cutoff_hz - fn) < 300.0 && cd > 30.0) {
            res.resample_src_rate = rate;
            res.resample_detected = "upsampled from " + std::to_string(rate / 1000) + " kHz";
            res.evidence.push_back("Sample-Rate Upscale: hard cliff at " + std::to_string(static_cast<int>(fn)) + " Hz");
            break;
        }
    }

    report_stage("auCDtect statistics", 0.63f);
    double avg_b = 0.0, prob_b = 0.0, phase_ent = 0.0;
    aucdtectFeatures(mags, phases_hi, bins, avg_b, prob_b, phase_ent);
    res.auc_avg_bound_freq = avg_b;
    res.auc_prob_bound_freq = prob_b;
    res.auc_phase_entropy = phase_ent;

    report_stage("mdct quantization", 0.70f);
    double mdct_sc = mdctQuantError(mid_data, mid_len, side_data, side_len);
    res.mdct_quant_score = mdct_sc;

    report_stage("finalizing", 0.95f);
    // Unified 0-100 Score calculation
    int score = 0;
    if (cutoff_hz < nyquist_ * 0.85) {
        score += 25;
        res.evidence.push_back("Depressed Cutoff: Frequency ceiling ends at " + std::to_string(static_cast<int>(cutoff_hz)) + " Hz");
    }
    if (cd > 25.0) {
        score += 30;
        res.evidence.push_back("Brickwall Filter: " + std::to_string(static_cast<int>(cd)) + " dB cliff drop at cutoff");
    }
    if (res.resample_src_rate > 0) {
        score += 45;
    }
    if (avg_b > 0 && avg_b < 16500.0) {
        score += 20;
        res.evidence.push_back("auCDtect Bound Collapse: scatter collapses at " + std::to_string(static_cast<int>(avg_b)) + " Hz");
    }
    if (mdct_sc >= 0.10) {
        score += 55;
        res.evidence.push_back("MDCT Quantization Lattice: AAC transcode fingerprint detected");
    }

    // Codec wall fingerprinting
    for (const auto& w : CODEC_WALLS) {
        if (std::abs(cutoff_hz - w.wall_hz) <= w.tol_hz && cd > 20.0) {
            res.codec_fingerprint = std::string(w.codec) + " " + w.profile;
            res.evidence.push_back("Codec Fingerprint: Cutoff matches " + res.codec_fingerprint);
            score = std::max(score, 88);
            break;
        }
    }

    // Natural credits
    if (cutoff_hz > nyquist_ * 0.90 && cd < 10.0 && hf > 0.02) {
        score = std::max(0, score - 30);
        res.natural_evidence.push_back("Rich Full-Band Extension: Clean organic harmonics to Nyquist");
    }

    res.main_score = std::clamp(score, 0, 100);

    // Primary verdict
    if (res.main_score <= 10) {
        res.verdict_label = "GENUINE";
        res.primary_verdict = "✓ Genuine Lossless — No lossy transcode indicators found";
    } else if (res.main_score <= 30) {
        res.verdict_label = "LIKELY_GENUINE";
        res.primary_verdict = "✓ Likely Genuine — Consistent with uncompressed master";
    } else if (res.main_score <= 54) {
        res.verdict_label = "CAUTION";
        res.primary_verdict = "~ Caution — Minor spectral quirks detected";
    } else if (res.main_score <= 85) {
        res.verdict_label = "SUSPICIOUS";
        res.primary_verdict = "⚠ Suspicious — Probable transcode or fake hi-res";
    } else {
        res.verdict_label = "LIKELY_LOSSY";
        res.primary_verdict = "✗ Fake Lossless — Lossy codec signature confirmed";
    }

    return res;
}

ForensicReport analyzeAudio(const std::string& filepath, double max_seconds, const ProgressCallback* cb) {
    ForensicReport report;
    report.filepath = filepath;

    auto t_start = std::chrono::high_resolution_clock::now();

    DecodedAudio audio;
    if (!decodeAudioFile(filepath, audio, max_seconds)) {
        report.authenticity.spectral_cutoff_verdict = "Decode failed or unsupported format";
        return report;
    }

    report.technical.channels = audio.channels;
    report.technical.sample_rate = audio.sample_rate;
    report.technical.precision = audio.bit_depth;
    report.technical.duration_sec = audio.duration_sec;
    report.technical.codec = audio.codec_name;

    SpectralEngine engine(audio.sample_rate, audio.channels, audio.duration_sec, 0, audio.codec_name);
    auto spectral = engine.analyse(audio.mid.data(), audio.mid.size(),
                                  audio.side.empty() ? nullptr : audio.side.data(), audio.side.size(),
                                  max_seconds, cb);

    report.authenticity.spectral = spectral;
    report.authenticity.spectral_cutoff_hz = spectral.cutoff_hz_str;
    report.authenticity.spectral_cutoff_verdict = spectral.primary_verdict;

    auto t_end = std::chrono::high_resolution_clock::now();
    report.analysis_seconds = std::chrono::duration<double>(t_end - t_start).count();

    return report;
}

} // namespace audio_forensic
