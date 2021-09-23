#include <cstdio>
#include <string>
#include <memory>
#include <gflags/gflags.h>

#include "RtMidi.h"
#include "proto/midi_convert.pb.h"
#include "util/file.h"
#include "google/protobuf/text_format.h"

DEFINE_double(fps, 60.0, "Target frames per second");
DEFINE_bool(list, false, "List MIDI output ports and exit");
DEFINE_int32(port, -1, "MIDI output port to use");

namespace player {

void ListOutputPorts(RtMidiOut* midiout) {
    size_t n = midiout->getPortCount();
    for(size_t i=0; i<n; ++i) {
        std::string name = midiout->getPortName(i);
        printf("%lu: %s\n", i, name.c_str());
    }
}

class TrackPlayer {
  public:
    TrackPlayer(RtMidiOut* midiout, const proto::Track* track, int measure_frames)
      : midiout_(midiout),
        track_(track),
        measure_frames_(measure_frames)
      {}

    const proto::Frame* GetFrame() {
        auto& measure = track_->measures(track_->sequence(seqnum_));
        for(auto& frame: measure.frames()) {
            if (frame.frame() == frame_)
                return &frame;
        }
        return nullptr;
    }

    void SendOther(const proto::Frame* frame) {
        for (const auto& other : frame->other()) {
            switch(other.event_case()) {
                case proto::OtherEvent::EventCase::EVENT_NOT_SET:
                    break;
                case proto::OtherEvent::EventCase::kProgramChange: {
                    std::vector<uint8_t> change = {
                        uint8_t(track_->number() | 0xC0),
                        uint8_t(other.program_change()),
                    };
                    midiout_->sendMessage(&change);
                    }
                    break;
                default:
                    fprintf(stderr, "Unknown `other` event %s\n",
                            other.DebugString().c_str());
            }
        }
    }

    void SendMidi(const proto::Frame* frame) {
        if (frame == nullptr) return;
        // Send midi control messages first.
        SendOther(frame);

        // Send midi note message next.
        const auto& note = frame->note();
        auto kind = note.kind();
        if (kind == proto::Note_Kind_NONE) return;
        uint8_t op = track_->number();
        if (kind == proto::Note_Kind_ON) {
            if (last_note_) {
                // Send a NoteOff event if one hasn't been sent for the
                // previous note.
                std::vector<uint8_t> off = { uint8_t(op|0x80), last_note_, 0 };
                midiout_->sendMessage(&off);
            }
            op |= 0x90;
            last_note_ = note.note();
        } else {
            op |= 0x80;
            last_note_ = 0;
        }
        std::vector<uint8_t> message = {
            op, uint8_t(note.note()), uint8_t(note.velocity())
        };
        midiout_->sendMessage(&message);
    }

    void Tick() {
        if (frame_ == measure_frames_) {
            frame_ = 0;
            seqnum_ += 1;
        }
        int measure = track_->sequence(seqnum_);
        if (measure & 0x80) {
            // If the MSB is set, that means set seqnum to that value & 0x7f.
            seqnum_ = measure & 0x7f;
        }
        if (Done()) return;
        SendMidi(GetFrame());
        frame_ += 1;
    }

    bool Done() {
        return seqnum_ >= track_->sequence_size();
    }

  private:
    RtMidiOut* midiout_;
    const proto::Track *track_;
    int measure_frames_;
    int seqnum_ = 0;
    int frame_ = 0;
    uint8_t last_note_ = 0;
};

class SongPlayer {
  public:
    SongPlayer(RtMidiOut* midiout)
      : midiout_(midiout)
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

        player_.clear();
        for(const auto& track : song_.tracks()) {
            player_.emplace_back(midiout_, &track, MeasureFrames());
        }
        return true;
    }

    int MeasureFrames() {
        double bps = song_.bpm() / 60.0;
        double beat = 1.0 / bps;
        double measure = song_.time_signature().numerator() * beat;
        int frames = (measure * FLAGS_fps + 0.5);
        return frames;
    }

    void Tick() {
        for(auto& p : player_) {
            p.Tick();
        }
    }

    bool Done() {
        for(auto& p : player_) {
            if (!p.Done()) return false;
        }
        return true;
    }

    void Play() {
        int delay = 1000000.0 / FLAGS_fps;
        while(!Done()) {
            Tick();
            usleep(delay);
        }
    }

  private:
    RtMidiOut* midiout_;
    proto::Song song_;
    std::vector<TrackPlayer> player_;
};

}  // namespace converter

const char kUsage[] =
R"ZZZ(<optional flags> [midi-file]

Description:
  Proto-MIDI file player.

Flags:
)ZZZ";

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage(kUsage);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto midiout = std::make_unique<RtMidiOut>();

    if (FLAGS_list) {
        player::ListOutputPorts(midiout.get());
        return 0;
    }
    if (FLAGS_port == -1) {
        fprintf(stderr, "Set --port to something (hint: see ports with --list)\n");
        return 1;
    }

    midiout->openPort(FLAGS_port);
    player::SongPlayer sp(midiout.get());
    if (argc > 1) {
        sp.Load(argv[1]);
        sp.Play();
    }
    return 0;
}
