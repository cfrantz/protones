#include <cstdio>
#include <string>
#include <memory>
#include <gflags/gflags.h>

#include "proto/midi_convert.pb.h"
#include "util/file.h"
#include "google/protobuf/text_format.h"
#include "absl/strings/str_cat.h"
#include "midi/fti.h"
#include "proto/fti.pb.h"
#include "proto/midi.pb.h"

DEFINE_double(fps, 60.0, "Target frames per second");
DEFINE_string(config, "", "MIDI configuration textproto file");

namespace converter {

std::string Symbol(const std::string& str) {
    std::string sym;
    for(char ch : str) {
        ch = tolower(ch);
        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'a' && ch <= 'z') ||
            ch == '_') {
            sym.push_back(ch);
        } else {
            sym.push_back('_');
        }
    }
    return sym;
}

class TrackPlayer {
  public:
    TrackPlayer(const proto::Track* track, int measure_frames)
      : track_(track),
        measure_frames_(measure_frames)
      {}

    void RenderDelay(std::vector<uint8_t>* m, int delay) {
        while(delay) {
            int d = std::min(delay, 128);
            m->push_back(-d);
            delay -= d;
        }
    }

    void RenderMeasures() {
        uint8_t last_volume = 0;
        for(const auto& measure : track_->measures()) {
            measures_.emplace_back();
            auto& m = measures_.back();
            int last_frame = 0;

            for(const auto& frame : measure.frames()) {
                const auto& note = frame.note();
                if (note.kind() == proto::Note_Kind_NONE) continue;

                RenderDelay(&m, frame.frame() - last_frame);
                last_frame = frame.frame();
                if (note.kind() == proto::Note_Kind_ON) {
                    int volume = note.velocity() / 8;
                    if (volume != last_volume) {
                        // 0x01 - 0x10: set volume to (val&0xF)
                        m.push_back(volume ? volume : 0x10);
                        last_volume = volume;
                    }
                    m.push_back(note.note() + note_offset_);
                } else if (note.kind() == proto::Note_Kind_OFF) {
                    m.push_back(0x17);
                } else {
                    //error
                }
            }

            RenderDelay(&m, measure_frames_ - last_frame);
            m.push_back(0);
        }
    }


    std::string MeasureName(const std::string& songname, size_t measurenum) {
        return Symbol(absl::StrCat(songname, "_", track_->name(), "_measure", measurenum));
    }
    std::string SequenceName(const std::string& songname) {
        return Symbol(absl::StrCat(songname, "_", track_->name(), "_sequence"));
    }
    std::string DataName(const std::string& songname) {
        return Symbol(absl::StrCat(songname, "_", track_->name(), "_data"));
    }

    void ToText(const std::string& songname) {
        size_t mn = 0;
        std::string name = DataName(songname);
        printf("%s:\n", name.c_str());
        printf("    .WORD %s\n", SequenceName(songname).c_str());
        for(mn=0; mn < measures_.size(); ++mn) {
            std::string name = MeasureName(songname, mn+1);
            printf("    .WORD %s\n", name.c_str());
        }

        name = SequenceName(songname);
        printf("%s:\n", name.c_str());
        printf("    .BYT ");
        size_t i = 0;
        for(auto val : track_->sequence()) {
            // In the protofile, sequences index into measures as zero-based,
            // but are 1-based in the assembly.
            if (val >=0 && val <= 127) {
                val += 1;
            }
            printf("%c$%02x", i == 0 ? ' ' : ',', uint8_t(val));
            ++i;
        }
        printf(",$00\n");

        mn = 0;
        for(const auto& measure : measures_) {
            ++mn;
            std::string name = MeasureName(songname, mn);
            printf("%s:\n", name.c_str());
            printf("    .BYT ");
            for(size_t i=0; i<measure.size(); ++i) {
                printf("%c$%02x", i == 0 ? ' ' : ',', measure[i]);
            }
            printf("\n");
        }
    }

    void Render(const std::string& songname) {
        RenderMeasures();
        ToText(songname);
    }

    void set_note_offset(int offset) { note_offset_ = offset; }

  private:
    const proto::Track *track_;
    int measure_frames_;
    int seqnum_ = 0;
    int frame_ = 0;
    int note_offset_ = 0;
    std::vector<std::vector<uint8_t>> measures_;
};

class SongPlayer {
  public:
    SongPlayer()
      {}

    bool Load(const std::string& filename) {
        std::string buffer;
        if (!File::GetContents(filename, &buffer)) {
            fprintf(stderr, "Unable to read file '%s'\n", filename.c_str());
            return false;
        }
        if (!google::protobuf::TextFormat::ParseFromString(buffer, &song_)) {
            fprintf(stderr, "Unable to parse file '%s'\n", filename.c_str());
            return false;
        }

        if (song_.title().empty()) {
            song_.set_title(File::Basename(filename));
        }
        player_.clear();
        for(const auto& track : song_.tracks()) {
            player_.emplace(track.number(), TrackPlayer(&track, MeasureFrames()));
        }
        return true;
    }

    void LoadConfig(const std::string& filename) {
        std::string data;
        if (!File::GetContents(filename, &data)) {
            fprintf(stderr, "Could not load %s\n", filename.c_str());
            return;
        }

        proto::MidiConfig config;
        if (!google::protobuf::TextFormat::ParseFromString(data, &config)) {
            fprintf(stderr, "Could not parse %s\n", filename.c_str());
            return;
        }

        config_ = config;
        instrument_.clear();
        for(const auto& inst : config_.instruments()) {
            auto fti = protones::LoadFTI(inst.second);
            if (fti.ok()) {
                instrument_.emplace(inst.first, fti.ValueOrDie());
            } else {
                fprintf(stderr, "Error loading '%s': %s\n",
                        inst.first.c_str(),
                        fti.status().ToString().c_str());
            }
        }
    }

    int MeasureFrames() {
        double bps = song_.bpm() / 60.0;
        double beat = 1.0 / bps;
        double measure = song_.time_signature().numerator() * beat;
        int frames = (measure * FLAGS_fps + 0.5);
        return frames;
    }

    void NaiveOrder(const std::string& title) {
        printf("    .BYT %d, 0\n", song_.tracks_size());
        for(auto& pair : player_) {
            printf("    .WORD %s\n", pair.second.DataName(title).c_str());
        }
    }

    void ConfigOrder(const std::string& title) {
        printf("    .BYT %d, 0\n", config_.channel_size());
        for(const auto& channel : config_.channel()) {
            // config uses 1-based MIDI channel numbers, while the player
            // uses zero-based channel numbers.
            auto it = player_.find(channel.channel() - 1);
            if (it != player_.end()) {
                printf("    .WORD %s\n", it->second.DataName(title).c_str());

                // Kindof hacky: we set config parameters in the player here
                // because we haven't "rendered" the data in the player yet.
                it->second.set_note_offset(channel.note_offset());
            } else {
                printf("    .WORD 0\n");
            }
        }
    }

    void Render() {
        std::string title = Symbol(song_.title());
        printf("%s:\n", title.c_str());
        if (config_.channel().empty()) {
            NaiveOrder(title);
        } else {
            ConfigOrder(title);
        }
        for(auto& pair : player_) {
            pair.second.Render(title);
        }
    }

  private:
    proto::Song song_;
    proto::MidiConfig config_;
    std::map<std::string, proto::FTInstrument> instrument_;
    std::map<int32_t, TrackPlayer> player_;
};

}  // namespace converter

const char kUsage[] =
R"ZZZ(<optional flags> [proto-file]

Description:
  Proto-MIDI file player.

Flags:
)ZZZ";

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage(kUsage);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    converter::SongPlayer sp;
    if (!FLAGS_config.empty()) {
        sp.LoadConfig(FLAGS_config);
    }
    if (argc > 1) {
        sp.Load(argv[1]);
        sp.Render();
    }
    return 0;
}
