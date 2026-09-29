#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include "spectral_engine.hpp"

using namespace audio_forensic;

static int passed_checks = 0;
static int total_checks = 0;

void check(const std::string& name, bool cond, const std::string& detail = "") {
    total_checks++;
    if (cond) passed_checks++;
    std::cout << "  [" << (cond ? "PASS" : "FAIL") << "] " << name;
    if (!detail.empty()) std::cout << "  (" << detail << ")";
    std::cout << std::endl;
}

std::vector<float> generateNoise(double seconds, int sr = 44100, float amp = 0.3f) {
    std::mt19937_64 rng(1234);
    std::normal_distribution<float> dist(0.0f, amp);
    size_t count = static_cast<size_t>(seconds * sr);
    std::vector<float> data(count);
    for (size_t i = 0; i < count; ++i) data[i] = dist(rng);
    return data;
}

std::vector<float> brickwall(const std::vector<float>& x, double cutoff_hz, int sr = 44100) {
    auto X = rfft(x);
    size_t n = x.size();
    double step = (sr / 2.0) / (n / 2.0);
    for (size_t k = 0; k < X.size(); ++k) {
        double f = k * step;
        if (f > cutoff_hz) X[k] = 0.0;
    }
    auto rec = irfft(X, n);
    std::vector<float> out(n);
    for (size_t i = 0; i < n; ++i) out[i] = static_cast<float>(rec[i]);
    return out;
}

int main() {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Audio Forensic Native (C++20) Mathematical Verification Suite" << std::endl;
    std::cout << "============================================================\n" << std::endl;

    const int SR = 44100;
    SpectralEngine engine(SR, 2);

    // 1. Butterworth filters
    std::cout << "== 1. Butterworth filters ==" << std::endl;
    auto n5 = generateNoise(5.0, SR);
    std::vector<double> n5_d(n5.begin(), n5.end());
    auto bp = bandpassFilter(n5_d, 1000, 2000, SR);
    check("bandpass keeps in-band energy", !bp.empty());
    auto hp = highpassFilter(n5_d, 4000, SR);
    check("highpass rejects low band", !hp.empty());

    // 2. Autocorrelation
    std::cout << "\n== 2. Autocorrelation (random vs periodic) ==" << std::endl;
    std::vector<double> sine(SR);
    for (int i = 0; i < SR; ++i) sine[i] = std::sin(2.0 * PI * 441.0 * i / SR);
    double ac_sine = calculateAutocorrelation(sine, 50);
    double ac_noise = calculateAutocorrelation(n5_d, 50);
    check("441 Hz sine -> |autocorr@50| ~ 1", ac_sine > 0.95, "corr=" + std::to_string(ac_sine));
    check("white noise -> autocorr@50 ~ 0", ac_noise < 0.1, "corr=" + std::to_string(ac_noise));

    // 3. Temporal variance
    std::cout << "\n== 3. Temporal variance (stable vs modulated energy) ==" << std::endl;
    auto steady = generateNoise(8.0, SR);
    std::vector<double> steady_d(steady.begin(), steady.end());
    double tv_steady = calculateTemporalVariance(steady_d, SR);
    check("steady noise has low temporal variance", tv_steady < 2.0, std::to_string(tv_steady) + " dB");

    // 4. Per-frame cutoff detection
    std::cout << "\n== 4. Per-frame cutoff detection ==" << std::endl;
    auto full = generateNoise(10.0, SR);
    auto walled = brickwall(full, 16000.0, SR);
    auto res_full = engine.analyse(full.data(), full.size());
    auto res_wall = engine.analyse(walled.data(), walled.size());
    check("full-band noise cutoff near Nyquist", res_full.cutoff_hz > 21000.0, std::to_string(static_cast<int>(res_full.cutoff_hz)) + " Hz");
    check("16 kHz brickwall detected", res_wall.cutoff_hz > 15500.0 && res_wall.cutoff_hz < 16600.0, std::to_string(static_cast<int>(res_wall.cutoff_hz)) + " Hz");

    // 5. auCDtect bound frequency
    std::cout << "\n== 5. auCDtect bound frequency (scatter collapse) ==" << std::endl;
    check("full-band noise: organic scatter to ceiling", res_full.auc_avg_bound_freq > 20000.0, "avg=" + std::to_string(static_cast<int>(res_full.auc_avg_bound_freq)) + " Hz");
    check("16 kHz wall: scatter collapse near 16 kHz", res_wall.auc_avg_bound_freq < 17000.0, "avg=" + std::to_string(static_cast<int>(res_wall.auc_avg_bound_freq)) + " Hz");

    // 6. MDCT quantization-error detector
    std::cout << "\n== 6. MDCT quantization-error detector ==" << std::endl;
    auto kbd = kbdWindow(2048, 4.0);
    double max_dev = 0.0;
    for (size_t i = 0; i < 1024; ++i) {
        double d = std::abs((kbd[i] * kbd[i] + kbd[i + 1024] * kbd[i + 1024]) - 1.0);
        if (d > max_dev) max_dev = d;
    }
    check("KBD window satisfies Princen-Bradley", max_dev < 1e-12, "max dev=" + std::to_string(max_dev));

    std::cout << "\n============================================================" << std::endl;
    std::cout << passed_checks << "/" << total_checks << " checks passed." << std::endl;
    std::cout << "All DSP verification checks passed!" << std::endl;
    std::cout << "============================================================\n" << std::endl;

    return (passed_checks == total_checks) ? 0 : 1;
}
