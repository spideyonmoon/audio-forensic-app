#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace audio_forensic {

struct DsdInfo {
    int channels = 0;
    int source_sample_rate = 0; // e.g. 2822400 (DSD64)
    int target_sample_rate = 88200;
    uint64_t total_pcm_frames = 0;
    std::vector<float> pcm_interleaved; // Decimated to 88.2 kHz
};

// Sinc decimation filter coefficients for 32:1 decimation
inline float dsdByteToPcm(uint8_t byte) {
    // Count set bits: 1s are +1.0, 0s are -1.0
    int ones = 0;
    for (int i = 0; i < 8; ++i) {
        if ((byte >> i) & 1) ones++;
    }
    return (2.0f * ones - 8.0f) / 8.0f;
}

inline bool readDsf(const std::string& path, DsdInfo& info, double max_seconds = 0.0) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    char header[28];
    if (!file.read(header, 28)) return false;
    if (std::memcmp(header, "DSD ", 4) != 0) return false;

    // fmt chunk
    char fmt_chunk[52];
    if (!file.read(fmt_chunk, 52)) return false;
    if (std::memcmp(fmt_chunk, "fmt ", 4) != 0) return false;

    uint32_t channel_num = *reinterpret_cast<uint32_t*>(fmt_chunk + 16);
    uint32_t sample_rate = *reinterpret_cast<uint32_t*>(fmt_chunk + 20);
    uint64_t sample_count = *reinterpret_cast<uint64_t*>(fmt_chunk + 32);
    uint32_t block_size_per_channel = *reinterpret_cast<uint32_t*>(fmt_chunk + 40);

    if (channel_num == 0 || sample_rate == 0 || block_size_per_channel == 0) return false;

    // data chunk
    char data_header[12];
    if (!file.read(data_header, 12)) return false;
    if (std::memcmp(data_header, "data", 4) != 0) return false;
    uint64_t data_size = *reinterpret_cast<uint64_t*>(data_header + 4) - 12;

    info.channels = channel_num;
    info.source_sample_rate = sample_rate;
    info.target_sample_rate = 88200;

    int decimation_ratio = sample_rate / info.target_sample_rate;
    if (decimation_ratio <= 0) decimation_ratio = 32;

    uint64_t total_pcm_frames = sample_count / decimation_ratio;
    if (max_seconds > 0.0) {
        uint64_t cap_frames = static_cast<uint64_t>(max_seconds * info.target_sample_rate);
        total_pcm_frames = std::min(total_pcm_frames, cap_frames);
    }
    info.total_pcm_frames = total_pcm_frames;

    // Streaming decimation: read block by block
    // DSF stores blocks of size `block_size_per_channel` per channel interleaved:
    // [Block Ch 0][Block Ch 1]...
    size_t block_size = block_size_per_channel;
    std::vector<uint8_t> block_buf(block_size * channel_num);

    info.pcm_interleaved.reserve(total_pcm_frames * channel_num);

    uint64_t frames_produced = 0;
    while (file && frames_produced < total_pcm_frames) {
        if (!file.read(reinterpret_cast<char*>(block_buf.data()), block_buf.size())) break;

        // Each channel has `block_size` bytes = `block_size * 8` 1-bit samples
        size_t pcm_frames_in_block = (block_size * 8) / decimation_ratio;
        size_t bytes_per_pcm = decimation_ratio / 8;

        for (size_t f = 0; f < pcm_frames_in_block && frames_produced < total_pcm_frames; ++f) {
            for (size_t ch = 0; ch < channel_num; ++ch) {
                const uint8_t* ch_ptr = block_buf.data() + ch * block_size + f * bytes_per_pcm;
                float sum = 0.0f;
                for (size_t b = 0; b < bytes_per_pcm; ++b) {
                    sum += dsdByteToPcm(ch_ptr[b]);
                }
                info.pcm_interleaved.push_back(sum / static_cast<float>(bytes_per_pcm));
            }
            frames_produced++;
        }
    }

    return true;
}

} // namespace audio_forensic
