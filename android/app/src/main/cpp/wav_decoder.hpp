#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace audio_forensic {

struct WavInfo {
    int channels = 0;
    int sample_rate = 0;
    int bits_per_sample = 0;
    uint64_t total_samples = 0;
    std::vector<float> pcm_interleaved; // -1.0f to 1.0f
};

inline bool readWav(const std::string& path, WavInfo& info, double max_seconds = 0.0) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    char header[12];
    if (!file.read(header, 12)) return false;
    if (std::memcmp(header, "RIFF", 4) != 0 || std::memcmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }

    uint16_t audio_format = 0;
    uint16_t num_channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    uint32_t data_size = 0;
    std::streampos data_pos = 0;

    while (file) {
        char chunk_id[4];
        uint32_t chunk_size = 0;
        if (!file.read(chunk_id, 4)) break;
        if (!file.read(reinterpret_cast<char*>(&chunk_size), 4)) break;

        if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&audio_format), 2);
            file.read(reinterpret_cast<char*>(&num_channels), 2);
            file.read(reinterpret_cast<char*>(&sample_rate), 4);
            uint32_t byte_rate = 0;
            uint16_t block_align = 0;
            file.read(reinterpret_cast<char*>(&byte_rate), 4);
            file.read(reinterpret_cast<char*>(&block_align), 2);
            file.read(reinterpret_cast<char*>(&bits_per_sample), 2);
            if (chunk_size > 16) {
                file.seekg(chunk_size - 16, std::ios::cur);
            }
        } else if (std::memcmp(chunk_id, "data", 4) == 0) {
            data_size = chunk_size;
            data_pos = file.tellg();
            break;
        } else {
            file.seekg(chunk_size, std::ios::cur);
        }
    }

    if (num_channels == 0 || sample_rate == 0 || data_size == 0) return false;

    info.channels = num_channels;
    info.sample_rate = sample_rate;
    info.bits_per_sample = bits_per_sample;

    file.seekg(data_pos);
    size_t bytes_per_sample = bits_per_sample / 8;
    if (bytes_per_sample == 0) return false;
    size_t total_frames = data_size / (num_channels * bytes_per_sample);
    if (max_seconds > 0.0) {
        size_t cap_frames = static_cast<size_t>(max_seconds * sample_rate);
        total_frames = std::min(total_frames, cap_frames);
    }
    info.total_samples = total_frames;

    size_t total_floats = total_frames * num_channels;
    info.pcm_interleaved.resize(total_floats);

    std::vector<uint8_t> buffer(total_frames * num_channels * bytes_per_sample);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    if (audio_format == 1) { // PCM Integer
        if (bits_per_sample == 16) {
            const int16_t* ptr = reinterpret_cast<const int16_t*>(buffer.data());
            for (size_t i = 0; i < total_floats; ++i) {
                info.pcm_interleaved[i] = static_cast<float>(ptr[i]) / 32768.0f;
            }
        } else if (bits_per_sample == 24) {
            const uint8_t* ptr = buffer.data();
            for (size_t i = 0; i < total_floats; ++i) {
                int32_t val = (static_cast<int32_t>(ptr[i * 3 + 0]) << 8) |
                              (static_cast<int32_t>(ptr[i * 3 + 1]) << 16) |
                              (static_cast<int32_t>(static_cast<int8_t>(ptr[i * 3 + 2])) << 24);
                info.pcm_interleaved[i] = static_cast<float>(val) / 2147483648.0f;
            }
        } else if (bits_per_sample == 32) {
            const int32_t* ptr = reinterpret_cast<const int32_t*>(buffer.data());
            for (size_t i = 0; i < total_floats; ++i) {
                info.pcm_interleaved[i] = static_cast<float>(ptr[i]) / 2147483648.0f;
            }
        }
    } else if (audio_format == 3) { // IEEE Float
        if (bits_per_sample == 32) {
            const float* ptr = reinterpret_cast<const float*>(buffer.data());
            for (size_t i = 0; i < total_floats; ++i) {
                info.pcm_interleaved[i] = ptr[i];
            }
        }
    }

    return true;
}

} // namespace audio_forensic
