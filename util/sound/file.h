#ifndef WVLX_UTIL_SOUND_FILE_H
#define WVLX_UTIL_SOUND_FILE_H
#include <sndfile.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "proto/sound.pb.h"

namespace sound {

enum class Interpolation {
    None = 0,
    Linear,
    Cosine,
    Cubic,
};

class File {
  public:
    File() {}
    File(proto::File* file, bool take_ownership = false)
        : file_(file), owning_(take_ownership) {}
    virtual ~File() {
        if (owning_) delete file_;
    }
    static absl::StatusOr<std::unique_ptr<File>> Load(
        const std::string& filename);
    static absl::Status Save(const std::string& filename,
                             const proto::File& file);
    static absl::Status SaveWav(const std::string& filename,
                                const proto::File& file);
    static absl::Status SaveProto(const std::string& filename,
                                  const proto::File& file);
    static absl::Status SaveText(const std::string& filename,
                                 const proto::File& file);
    absl::Status Save(const std::string& filename) const;
    void ConvertToMono();

    int channels() const { return file_->channel_size(); }
    int size() const { return file_->channel(0).data_size(); }
    double rate() const { return file_->rate(); }
    double length() const { return size() / rate(); }

    float Sample(int ch, int i) const {
        if (i < 0) {
            return -Sample(ch, -i);
        } else if (i >= file_->channel(ch).data_size()) {
            i = (file_->channel(ch).data_size() - 1) -
                (i - file_->channel(ch).data_size());
            return -Sample(ch, i);
        } else {
            return file_->channel(ch).data(i);
        }
    }
    float Sample(int ch, double i) const { return Sample(ch, int(rate() * i)); }
    void Resample(double rate);
    void BitCrush(int bits);
    std::vector<uint8_t> DpcmEncode(int bits = 7, float delta = 2.0f);
    static std::unique_ptr<File> DpcmDecode(const uint8_t* data, size_t len,
                                            double rate, int bits = 7,
                                            float delta = 2.0f);

    const proto::File& data() const { return *file_; }
    proto::File* release() {
        owning_ = false;
        return file_;
    };
    void replace(proto::File* f) {
        if (owning_) delete file_;
        file_ = f;
    }

  private:
    float InterpolateCosine(int ch, double index);
    float InterpolateCubic(int ch, double index);
    proto::File* file_ = nullptr;
    bool owning_ = false;
};

}  // namespace sound
#endif  // WVLX_UTIL_SOUND_FILE_H
