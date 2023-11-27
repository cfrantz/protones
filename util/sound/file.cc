#include "util/sound/file.h"

#include <sndfile.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

namespace sound {

constexpr float PI = 3.14159265358979323846f;
constexpr double EPSILON = 0.000001;

absl::StatusOr<std::unique_ptr<File>> File::Load(const std::string& filename) {
    SF_INFO info;
    SNDFILE* fp = sf_open(filename.c_str(), SFM_READ, &info);
    if (fp == nullptr) {
        LOG(ERROR) << "File::Load could not open " << filename;
        return absl::InvalidArgumentError(sf_strerror(fp));
    }
    LOG(INFO) << "soundfile:"
              << " frames:" << info.frames << " rate:" << info.samplerate
              << " ch:" << info.channels << " format:" << info.format;

    std::vector<float> buf(size_t(info.samplerate * info.channels));
    size_t count;
    proto::File* data = new proto::File;
    data->set_name(filename);
    data->set_rate(double(info.samplerate));
    for (int ch = 0; ch < info.channels; ++ch) {
        data->mutable_channel()->Add();
    }
    do {
        count = sf_readf_float(fp, buf.data(), info.samplerate);
        for (size_t i = 0, n = 0; n < count; ++n) {
            for (int ch = 0; ch < info.channels; ++ch) {
                data->mutable_channel(ch)->mutable_data()->Add(buf[i++]);
            }
        }
    } while (count);
    sf_close(fp);
    return absl::make_unique<File>(data, true);
}

absl::Status File::SaveProto(const std::string& filename,
                             const proto::File& file) {
    std::fstream out(filename, std::ios_base::out | std::ios_base::trunc |
                                   std::ios_base::binary);
    if (!file.SerializeToOstream(&out)) {
        return absl::InternalError("serialization failed");
    }
    return absl::OkStatus();
}

absl::Status File::SaveText(const std::string& filename,
                            const proto::File& file) {
    FILE* fp = fopen(filename.c_str(), "wt");
    if (fp == nullptr) {
        return absl::InternalError("failed to open file");
    }
    google::protobuf::io::FileOutputStream out(fileno(fp));
    if (!google::protobuf::TextFormat::Print(file, &out)) {
        return absl::InternalError("serialization failed");
    }
    return absl::OkStatus();
}

absl::Status File::SaveWav(const std::string& filename,
                           const proto::File& file) {
    SF_INFO info{(sf_count_t)file.channel(0).data_size(), (int)file.rate(),
                 (int)file.channel_size(), SF_FORMAT_WAV | SF_FORMAT_FLOAT};
    for (const auto& ch : file.channel()) {
        if (ch.data_size() != info.frames) {
            return absl::FailedPreconditionError(
                "Not all channels the same length");
        }
    }
    LOG(INFO) << "soundfile:"
              << " frames:" << info.frames << " rate:" << info.samplerate
              << " ch:" << info.channels << " format:" << info.format;
    LOG(INFO) << "format_check " << sf_format_check(&info);

    // Take the dumb way out: allocate a buffer big enough to interleave
    // all of the sample data.
    std::vector<float> data;
    for (int i = 0; i < info.frames; ++i) {
        for (int ch = 0; ch < info.channels; ++ch) {
            data.emplace_back(file.channel(ch).data(i));
        }
    }

    SNDFILE* fp = sf_open(filename.c_str(), SFM_WRITE, &info);
    if (fp == nullptr) {
        return absl::InternalError(sf_strerror(fp));
    }
    sf_write_float(fp, data.data(), data.size());
    sf_close(fp);
    return absl::OkStatus();
}

absl::Status File::Save(const std::string& filename, const proto::File& file) {
    if (absl::EndsWith(filename, ".wav")) {
        return SaveWav(filename, file);
    } else if (absl::EndsWith(filename, ".pb")) {
        return SaveProto(filename, file);
    } else if (absl::EndsWith(filename, ".textpb")) {
        return SaveText(filename, file);
    } else {
        return absl::InvalidArgumentError("bad file extension");
    }
}

absl::Status File::Save(const std::string& filename) const {
    return Save(filename, *file_);
}

void File::ConvertToMono() {
    if (file_->channel_size() == 1) {
        // File is already mono.
        return;
    }
    proto::File* file = new proto::File;
    file->set_name(file_->name());
    file->set_rate(file_->rate());
    auto* data = file->mutable_channel()->Add();
    float n = file_->channel_size();
    for (int i = 0; i < file_->channel(0).data_size(); ++i) {
        float sample = 0;
        for (int ch = 0; ch < file_->channel_size(); ++ch) {
            sample += file_->channel(ch).data(i);
        }
        data->mutable_data()->Add(sample / n);
    }
    replace(file);
}

void File::Resample(double rate) {
    for (int ch = 0; ch < file_->channel_size(); ++ch) {
        int len = file_->channel(ch).data_size();
        int n = len * rate / file_->rate();
        double ratio = double(len - 1) / double(n - 1);
        proto::Samples samples;
        for (int i = 0; i < n; ++i) {
            double index = double(i) * ratio;
            double val;
            if (index < 1.0) {
                val = InterpolateCosine(ch, index);
            } else if (index >= len - 1) {
                val = file_->channel(ch).data(int(index));
            } else if (index >= len - 3) {
                val = InterpolateCosine(ch, index);
            } else {
                val = InterpolateCubic(ch, index);
            }
            samples.add_data(val);
        }
        *file_->mutable_channel(ch) = samples;
    }
    file_->set_rate(rate);
}

void File::BitCrush(int bits) {
    float factor = (1 << (bits - 1)) - 1;
    for (int ch = 0; ch < file_->channel_size(); ++ch) {
        for (int i = 0; i < file_->channel(ch).data_size(); ++i) {
            float s = file_->channel(ch).data(i);
            s = truncf(s * factor) / factor;
            file_->mutable_channel(ch)->set_data(i, s);
        }
    }
}

std::vector<uint8_t> File::DpcmEncode(int bits, float delta) {
    float factor = (1 << (bits)) - 1;
    float value = 0.0;
    uint8_t sample;
    std::vector<uint8_t> dpcm;
    for (int i = 0; i < file_->channel(0).data_size(); ++i) {
        float s = file_->channel(0).data(i);
        s = (0.5f + 0.5f * s) * factor;
        if (value < s) {
            value += delta;
            sample |= 1 << (i % 8);
        } else {
            value -= delta;
        }
        if (i && i % 8 == 0) {
            dpcm.push_back(sample);
            sample = 0;
        }
    }
    dpcm.push_back(sample);
    return dpcm;
}

std::unique_ptr<File> File::DpcmDecode(const uint8_t* data, size_t len,
                                       double rate, int bits, float delta) {
    float factor = (1 << (bits)) - 1;
    float value = 0.0;
    proto::File* file = new proto::File;
    file->set_rate(rate);
    proto::Samples* samples = file->add_channel();
    for (size_t k = 0; k < len; ++k) {
        uint8_t s = data[k];
        for (int i = 0; i < 8; ++i, s >>= 1) {
            if (s & 1) {
                value += delta;
                if (value > factor) value = factor;
            } else {
                value -= delta;
                if (value < 0.0) value = 0.0;
            }
            samples->add_data(2.0f * value / factor - 1.0f);
        }
    }
    return absl::make_unique<File>(file, true);
}

float File::InterpolateCosine(int ch, double index) {
    double i;
    double f = modf(index, &i);
    int x = i;
    float y1 = file_->channel(ch).data(x);
    float y2 = file_->channel(ch).data(x + 1);
    float mu = (1.0f - cosf(f * PI)) / 2.0f;
    return y1 * (1.0 - mu) + y2 * mu;
}

float File::InterpolateCubic(int ch, double index) {
    double i;
    double f = modf(index, &i);
    int x = i;
    float y0 = file_->channel(ch).data(x - 1);
    float y1 = file_->channel(ch).data(x);
    float y2 = file_->channel(ch).data(x + 1);
    float y3 = file_->channel(ch).data(x + 2);
    return y1 + 0.5 * f *
                    (y2 - y0 +
                     f * (2.0 * y0 - 5.0 * y1 + 4.0 * y2 - y3 +
                          f * (3.0 * (y1 - y2) + y3 - y0)));
}

}  // namespace sound
