#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <numbers>
#include <cstdint>

namespace audio_forensic {

constexpr double PI = 3.141592653589793238462643383279502884;

// ---------------------------------------------------------------------------
// Self-contained Fast Fourier Transform (Cooley-Tukey Radix-2 + Arbitrary N)
// ---------------------------------------------------------------------------
inline size_t nextPowerOfTwo(size_t n) {
    if (n <= 1) return 1;
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// In-place Radix-2 Complex FFT (n must be a power of 2)
inline void fftRadix2(std::vector<std::complex<double>>& a, bool invert) {
    size_t n = a.size();
    if (n <= 1) return;

    // Bit reversal permutation
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = 2.0 * PI / len * (invert ? -1.0 : 1.0);
        std::complex<double> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        double inv_n = 1.0 / static_cast<double>(n);
        for (auto& x : a) {
            x *= inv_n;
        }
    }
}

// Bluestein's Chirp-z Algorithm for arbitrary-size N FFT
inline void fftGeneral(std::vector<std::complex<double>>& a, bool invert) {
    size_t n = a.size();
    if (n <= 1) return;
    if ((n & (n - 1)) == 0) {
        fftRadix2(a, invert);
        return;
    }

    size_t m = nextPowerOfTwo(2 * n - 1);
    std::vector<std::complex<double>> an(m, 0.0);
    std::vector<std::complex<double>> bn(m, 0.0);

    double sign = invert ? 1.0 : -1.0;
    for (size_t i = 0; i < n; ++i) {
        double angle = sign * PI * (static_cast<double>(i * i % (2 * n))) / static_cast<double>(n);
        std::complex<double> w(std::cos(angle), std::sin(angle));
        an[i] = a[i] * w;
    }

    for (size_t i = 0; i < n; ++i) {
        double angle = -sign * PI * (static_cast<double>(i * i % (2 * n))) / static_cast<double>(n);
        bn[i] = std::complex<double>(std::cos(angle), std::sin(angle));
        if (i > 0) {
            bn[m - i] = bn[i];
        }
    }

    fftRadix2(an, false);
    fftRadix2(bn, false);
    for (size_t i = 0; i < m; ++i) {
        an[i] *= bn[i];
    }
    fftRadix2(an, true);

    double inv_scale = invert ? (1.0 / static_cast<double>(n)) : 1.0;
    for (size_t i = 0; i < n; ++i) {
        double angle = sign * PI * (static_cast<double>(i * i % (2 * n))) / static_cast<double>(n);
        std::complex<double> w(std::cos(angle), std::sin(angle));
        a[i] = an[i] * w * inv_scale;
    }
}

// Real FFT (rfft) - matches NumPy/SciPy np.fft.rfft
inline std::vector<std::complex<double>> rfft(const double* input, size_t n) {
    std::vector<std::complex<double>> cinput(n);
    for (size_t i = 0; i < n; ++i) {
        cinput[i] = std::complex<double>(input[i], 0.0);
    }
    fftGeneral(cinput, false);
    size_t out_len = n / 2 + 1;
    cinput.resize(out_len);
    return cinput;
}

inline std::vector<std::complex<double>> rfft(const std::vector<double>& input) {
    return rfft(input.data(), input.size());
}

inline std::vector<std::complex<double>> rfft(const std::vector<float>& input) {
    std::vector<double> d(input.begin(), input.end());
    return rfft(d.data(), d.size());
}

// Inverse Real FFT (irfft) - matches NumPy/SciPy np.fft.irfft(X, n)
inline std::vector<double> irfft(const std::vector<std::complex<double>>& X, size_t n) {
    std::vector<std::complex<double>> full(n);
    size_t half = n / 2 + 1;
    for (size_t i = 0; i < half && i < X.size(); ++i) {
        full[i] = X[i];
    }
    for (size_t i = 1; i < n - half + 1; ++i) {
        full[n - i] = std::conj(full[i]);
    }
    fftGeneral(full, true);
    std::vector<double> out(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = full[i].real();
    }
    return out;
}

// ---------------------------------------------------------------------------
// Discrete Cosine Transform Type IV (DCT-IV) for MDCT
// ---------------------------------------------------------------------------
inline std::vector<double> dctIV(const std::vector<double>& input) {
    size_t N = input.size();
    std::vector<double> out(N, 0.0);
    double factor = std::sqrt(2.0 / static_cast<double>(N));
    for (size_t k = 0; k < N; ++k) {
        double sum = 0.0;
        for (size_t n = 0; n < N; ++n) {
            sum += input[n] * std::cos(PI * (static_cast<double>(n) + 0.5) * (static_cast<double>(k) + 0.5) / static_cast<double>(N));
        }
        out[k] = factor * sum;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Second-Order Section (SOS) Biquad IIR Filter
// Direct Form II Transposed (matches scipy.signal.sosfilt)
// ---------------------------------------------------------------------------
struct BiquadSOS {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;
};

inline std::vector<double> sosfilt(const std::vector<BiquadSOS>& sos, const std::vector<double>& x) {
    std::vector<double> y = x;
    for (const auto& sec : sos) {
        double b0 = sec.b0 / sec.a0;
        double b1 = sec.b1 / sec.a0;
        double b2 = sec.b2 / sec.a0;
        double a1 = sec.a1 / sec.a0;
        double a2 = sec.a2 / sec.a0;

        double s1 = 0.0, s2 = 0.0;
        for (size_t i = 0; i < y.size(); ++i) {
            double xi = y[i];
            double yi = b0 * xi + s1;
            s1 = b1 * xi - a1 * yi + s2;
            s2 = b2 * xi - a2 * yi;
            y[i] = yi;
        }
    }
    return y;
}

// Butterworth Filter Design (matching scipy.signal.butter with output='sos')
inline std::vector<BiquadSOS> designButterworthHighpass(int order, double cutoff_hz, double fs) {
    std::vector<BiquadSOS> sos;
    double warped = 2.0 * fs * std::tan(PI * cutoff_hz / fs);
    int num_sections = (order + 1) / 2;

    for (int k = 1; k <= num_sections; ++k) {
        double theta = PI * (2.0 * k + order - 1.0) / (2.0 * order);
        double re = std::cos(theta);
        double im = std::sin(theta);

        // s-plane pole
        std::complex<double> sp(warped * re, warped * im);

        // Bilinear transform: s = 2*fs * (z - 1) / (z + 1) -> z = (2*fs + s) / (2*fs - s)
        double fs2 = 2.0 * fs;
        if (order % 2 == 1 && k == num_sections) {
            // First order section
            double sp_real = -warped;
            double a0 = fs2 - sp_real;
            double a1 = -(fs2 + sp_real);
            double b0 = fs2;
            double b1 = -fs2;
            sos.push_back({b0, b1, 0.0, a0, a1, 0.0});
        } else {
            // Second order conjugate pole pair
            std::complex<double> pole = (fs2 + sp) / (fs2 - sp);
            double p_re = pole.real();
            double p_mag2 = std::norm(pole);

            // Highpass zero at z = 1
            // H(z) = K * (1 - z^-1)^2 / (1 - 2*re(p)*z^-1 + |p|^2*z^-2)
            double a0 = 1.0;
            double a1 = -2.0 * p_re;
            double a2 = p_mag2;

            // Gain at z = -1 (Nyquist, highpass passband) should be 1
            double gain_denom = 1.0 - a1 + a2;
            double gain_num = 4.0; // (1 - (-1))^2 = 4
            double scale = gain_denom / gain_num;

            sos.push_back({scale, -2.0 * scale, scale, a0, a1, a2});
        }
    }
    return sos;
}

inline std::vector<BiquadSOS> designButterworthBandpass(int order, double lowcut, double highcut, double fs) {
    // A bandpass of order N has 2N poles. Designed via LP prototype + bandpass transform
    // For order=4 bandpass (2 lowcut poles + 2 highcut poles): cascade 4th-order HP and LP
    // Or standard 2nd-order biquad bandpass pairs:
    // Bilinear transform of analog bandpass biquads:
    std::vector<BiquadSOS> sos;
    double f1 = lowcut;
    double f2 = highcut;
    double w1 = 2.0 * fs * std::tan(PI * f1 / fs);
    double w2 = 2.0 * fs * std::tan(PI * f2 / fs);
    double w0 = std::sqrt(w1 * w2);
    double bw = w2 - w1;
    int n_sections = order;

    for (int k = 1; k <= order / 2; ++k) {
        double theta = PI * (2.0 * k + order - 1.0) / (2.0 * order);
        double c_re = std::cos(theta);
        double c_im = std::sin(theta);
        std::complex<double> pk(c_re, c_im);

        // s-plane quadratic: s^2 - bw * pk * s + w0^2 = 0
        std::complex<double> disc = std::sqrt(bw * bw * pk * pk - 4.0 * w0 * w0);
        std::complex<double> sp1 = 0.5 * (bw * pk + disc);
        std::complex<double> sp2 = 0.5 * (bw * pk - disc);

        double fs2 = 2.0 * fs;
        std::complex<double> z1 = (fs2 + sp1) / (fs2 - sp1);
        std::complex<double> z2 = (fs2 + sp2) / (fs2 - sp2);

        // Biquad 1
        double a1_1 = -2.0 * z1.real();
        double a2_1 = std::norm(z1);
        // Bandpass zeros at z = 1 and z = -1: (1 - z^-1)(1 + z^-1) = 1 - z^-2
        // Normalization at center freq
        double b0_1 = bw / fs2;
        sos.push_back({b0_1, 0.0, -b0_1, 1.0, a1_1, a2_1});

        // Biquad 2
        double a1_2 = -2.0 * z2.real();
        double a2_2 = std::norm(z2);
        double b0_2 = bw / fs2;
        sos.push_back({b0_2, 0.0, -b0_2, 1.0, a1_2, a2_2});
    }

    // Normalize passband gain to 1.0 at center frequency
    double w_center = 2.0 * PI * std::sqrt(lowcut * highcut) / fs;
    std::complex<double> ejw(std::cos(w_center), std::sin(w_center));
    std::complex<double> ejw2(std::cos(2.0 * w_center), std::sin(2.0 * w_center));
    std::complex<double> total_h(1.0, 0.0);
    for (const auto& s : sos) {
        std::complex<double> num = s.b0 + s.b1 * std::conj(ejw) + s.b2 * std::conj(ejw2);
        std::complex<double> den = s.a0 + s.a1 * std::conj(ejw) + s.a2 * std::conj(ejw2);
        total_h *= (num / den);
    }
    double gain = std::abs(total_h);
    if (gain > 1e-12) {
        sos[0].b0 /= gain;
        sos[0].b1 /= gain;
        sos[0].b2 /= gain;
    }
    return sos;
}

inline std::vector<double> bandpassFilter(const std::vector<double>& data, double lowcut, double highcut, double fs, int order = 4) {
    auto sos = designButterworthBandpass(order, lowcut, highcut, fs);
    return sosfilt(sos, data);
}

inline std::vector<double> highpassFilter(const std::vector<double>& data, double cutoff, double fs, int order = 4) {
    auto sos = designButterworthHighpass(order, cutoff, fs);
    return sosfilt(sos, data);
}

// ---------------------------------------------------------------------------
// Statistical Metrics (Autocorrelation, Variance, Uniform 1D)
// ---------------------------------------------------------------------------
inline double calculateAutocorrelation(const std::vector<double>& data, size_t lag = 50) {
    if (data.size() <= lag * 2) return 0.0;
    double sum = 0.0;
    for (double v : data) sum += v;
    double mean = sum / static_cast<double>(data.size());

    double var = 0.0;
    for (double v : data) {
        double diff = v - mean;
        var += diff * diff;
    }
    double std_dev = std::sqrt(var / static_cast<double>(data.size()));
    if (std_dev < 1e-10) return 0.0;

    double num = 0.0, denom_a = 0.0, denom_b = 0.0;
    size_t count = data.size() - lag;
    for (size_t i = 0; i < count; ++i) {
        double a = (data[i] - mean) / std_dev;
        double b = (data[i + lag] - mean) / std_dev;
        num += a * b;
        denom_a += a * a;
        denom_b += b * b;
    }
    double denom = std::sqrt(denom_a * denom_b);
    if (denom <= 1e-12) return 0.0;
    return std::abs(num / denom);
}

inline double calculateTemporalVariance(const std::vector<double>& data, int sample_rate, double segment_duration = 1.0) {
    size_t seg_samples = static_cast<size_t>(segment_duration * sample_rate);
    if (seg_samples == 0) return 0.0;
    size_t num_segs = data.size() / seg_samples;
    if (num_segs < 2) return 0.0;

    std::vector<double> energies_db(num_segs);
    double sum_e = 0.0;
    for (size_t i = 0; i < num_segs; ++i) {
        double sum_sq = 0.0;
        size_t start = i * seg_samples;
        for (size_t j = 0; j < seg_samples; ++j) {
            double v = data[start + j];
            sum_sq += v * v;
        }
        double rms = std::sqrt(sum_sq / static_cast<double>(seg_samples));
        double edb = 20.0 * std::log10(rms + 1e-12);
        energies_db[i] = edb;
        sum_e += edb;
    }

    double mean_e = sum_e / static_cast<double>(num_segs);
    double var_e = 0.0;
    for (double edb : energies_db) {
        double diff = edb - mean_e;
        var_e += diff * diff;
    }
    return std::sqrt(var_e / static_cast<double>(num_segs));
}

// 1D Uniform filter (moving average) with 'nearest' boundary condition
// Matches scipy.ndimage.uniform_filter1d(..., size=5, mode='nearest')
inline std::vector<double> uniformFilter1D(const std::vector<double>& in, size_t size) {
    size_t n = in.size();
    if (n == 0) return {};
    std::vector<double> out(n, 0.0);
    int half = static_cast<int>(size / 2);

    double window_sum = 0.0;
    for (int k = -half; k <= half; ++k) {
        int idx = std::clamp(k, 0, static_cast<int>(n - 1));
        window_sum += in[idx];
    }
    out[0] = window_sum / static_cast<double>(size);

    for (size_t i = 1; i < n; ++i) {
        int left_drop = std::clamp(static_cast<int>(i) - 1 - half, 0, static_cast<int>(n - 1));
        int right_add = std::clamp(static_cast<int>(i) + half, 0, static_cast<int>(n - 1));
        window_sum += in[right_add] - in[left_drop];
        out[i] = window_sum / static_cast<double>(size);
    }
    return out;
}

// Kaiser Window
inline double besselI0(double x) {
    double ax = std::abs(x);
    if (ax < 3.75) {
        double y = x / 3.75;
        y *= y;
        return 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492 + y * (0.2659732 + y * (0.0360768 + y * 0.0045813)))));
    } else {
        double y = 3.75 / ax;
        return (std::exp(ax) / std::sqrt(ax)) * (0.39894228 + y * (0.01328592 + y * (0.00225319 + y * (-0.00157565 + y * (0.00916281 + y * (-0.02057706 + y * (0.02635537 + y * (-0.01647633 + y * 0.00392377))))))));
    }
}

// Kaiser-Bessel Derived (KBD) analysis window of length n2 (matches _kbd_window)
inline std::vector<double> kbdWindow(size_t n2, double alpha = 4.0) {
    size_t m = n2 / 2;
    std::vector<double> k(m + 1);
    double i0_beta = besselI0(PI * alpha);
    for (size_t i = 0; i <= m; ++i) {
        double x = 2.0 * static_cast<double>(i) / static_cast<double>(m) - 1.0;
        k[i] = besselI0(PI * alpha * std::sqrt(std::max(0.0, 1.0 - x * x))) / i0_beta;
    }
    std::vector<double> cs(m + 1, 0.0);
    cs[0] = k[0];
    for (size_t i = 1; i <= m; ++i) cs[i] = cs[i - 1] + k[i];
    double total = cs[m];

    std::vector<double> win(n2);
    for (size_t i = 0; i < m; ++i) {
        double r = std::sqrt(cs[i] / total);
        win[i] = r;
        win[n2 - 1 - i] = r;
    }
    return win;
}

// Hanning Window
inline std::vector<double> hanningWindow(size_t n) {
    std::vector<double> win(n);
    for (size_t i = 0; i < n; ++i) {
        win[i] = 0.5 * (1.0 - std::cos(2.0 * PI * static_cast<double>(i) / static_cast<double>(n)));
    }
    return win;
}

} // namespace audio_forensic
