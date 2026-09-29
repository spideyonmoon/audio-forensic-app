#pragma once

#include "types.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace audio_forensic {

class MetadataExtractor {
public:
    static void extractFlacMetadata(const std::string& filepath, AudioTags& tags, AudioTechnical& tech, std::string& cover_info) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return;

        char magic[4];
        if (!file.read(magic, 4) || std::memcmp(magic, "fLaC", 4) != 0) return;

        bool is_last = false;
        while (!is_last && file.good()) {
            uint8_t hdr[4];
            if (!file.read(reinterpret_cast<char*>(hdr), 4)) break;

            is_last = (hdr[0] & 0x80) != 0;
            int type = hdr[0] & 0x7F;
            uint32_t len = (static_cast<uint32_t>(hdr[1]) << 16) |
                           (static_cast<uint32_t>(hdr[2]) << 8) |
                           static_cast<uint32_t>(hdr[3]);

            if (type == 0 && len >= 34) { // STREAMINFO
                std::vector<uint8_t> buf(len);
                if (file.read(reinterpret_cast<char*>(buf.data()), len)) {
                    // Min/Max block size (0..3), Min/Max frame size (4..9)
                    // Sample rate (20 bits), Channels (3 bits), BPS (5 bits), Total samples (36 bits)
                    uint32_t sr = (static_cast<uint32_t>(buf[10]) << 12) |
                                  (static_cast<uint32_t>(buf[11]) << 4) |
                                  (static_cast<uint32_t>(buf[12]) >> 4);
                    int ch = ((buf[12] >> 1) & 0x07) + 1;
                    int bps = (((buf[12] & 0x01) << 4) | ((buf[13] >> 4) & 0x0F)) + 1;
                    uint64_t total_samples = ((static_cast<uint64_t>(buf[13] & 0x0F)) << 32) |
                                             (static_cast<uint64_t>(buf[14]) << 24) |
                                             (static_cast<uint64_t>(buf[15]) << 16) |
                                             (static_cast<uint64_t>(buf[16]) << 8) |
                                             static_cast<uint64_t>(buf[17]);
                    tech.sample_rate = sr;
                    tech.channels = ch;
                    tech.precision = bps;
                    tech.codec = "FLAC";
                    tech.sample_encoding = std::to_string(bps) + "-bit FLAC";
                    tech.compression_mode = "Lossless";
                    if (sr > 0 && total_samples > 0) {
                        tech.duration_sec = static_cast<double>(total_samples) / sr;
                        int min = static_cast<int>(tech.duration_sec) / 60;
                        int sec = static_cast<int>(tech.duration_sec) % 60;
                        std::ostringstream ss;
                        ss << std::setfill('0') << std::setw(2) << min << ":" << std::setw(2) << sec;
                        tech.duration = ss.str();
                    }
                }
            } else if (type == 4) { // VORBIS_COMMENT
                std::vector<uint8_t> buf(len);
                if (file.read(reinterpret_cast<char*>(buf.data()), len)) {
                    std::string vendor;
                    parseVorbisComments(buf.data(), len, tags, vendor);
                    if (!vendor.empty()) tech.writing_library = vendor;
                }
            } else if (type == 6) { // PICTURE
                std::vector<uint8_t> buf(len);
                if (file.read(reinterpret_cast<char*>(buf.data()), len)) {
                    tags.other["Cover"] = "Yes";
                    if (len >= 8) {
                        uint32_t mime_len = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
                        if (8 + mime_len <= len) {
                            std::string mime(reinterpret_cast<char*>(buf.data() + 8), mime_len);
                            tags.other["Cover Mime"] = mime;
                            tags.other["Cover Type"] = "Cover (front)";
                        }
                    }
                }
            } else {
                file.seekg(len, std::ios::cur);
            }
        }
    }

    static void parseVorbisComments(const uint8_t* data, size_t size, AudioTags& tags, std::string& vendor) {
        if (size < 8) return;
        size_t offset = 0;

        uint32_t vendor_len = 0;
        std::memcpy(&vendor_len, data + offset, 4);
        offset += 4;
        if (offset + vendor_len > size) return;
        vendor = std::string(reinterpret_cast<const char*>(data + offset), vendor_len);
        offset += vendor_len;

        if (offset + 4 > size) return;
        uint32_t count = 0;
        std::memcpy(&count, data + offset, 4);
        offset += 4;

        for (uint32_t i = 0; i < count && offset + 4 <= size; ++i) {
            uint32_t len = 0;
            std::memcpy(&len, data + offset, 4);
            offset += 4;
            if (offset + len > size) break;
            std::string comment(reinterpret_cast<const char*>(data + offset), len);
            offset += len;

            auto eq = comment.find('=');
            if (eq == std::string::npos) continue;
            std::string key = comment.substr(0, eq);
            std::string val = comment.substr(eq + 1);
            std::string lk = key;
            std::transform(lk.begin(), lk.end(), lk.begin(), ::tolower);

            if (lk == "title") tags.title = val;
            else if (lk == "artist") tags.artist = val;
            else if (lk == "album") tags.album = val;
            else if (lk == "albumartist" || lk == "album_artist") tags.album_artist = val;
            else if (lk == "date" || lk == "year") tags.date = val;
            else if (lk == "tracknumber" || lk == "track") tags.other["Track Position"] = val;
            else if (lk == "totaltracks") tags.other["Track Position Total"] = val;
            else if (lk == "discnumber") tags.other["Part"] = val;
            else if (lk == "totaldiscs") tags.other["Part Position Total"] = val;
            else if (lk == "organization" || lk == "label") tags.other["Producer"] = val;
            else if (lk == "catalognumber" || lk == "catalog_number") tags.other["Catalognumber"] = val;
            else if (lk == "replaygain_track_gain") tags.replaygain_track_gain = val;
            else if (lk == "replaygain_album_gain") tags.replaygain_album_gain = val;
            else if (lk == "comment" || lk == "description") tags.comments = val;
            else tags.other[key] = val;
        }
    }
};

} // namespace audio_forensic
