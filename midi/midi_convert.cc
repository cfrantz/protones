#include <cstdio>
#include <cstdlib>
#include <string>
#include <tuple>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "MidiFile.h"
#include "proto/midi_convert.pb.h"
#include "google/protobuf/util/message_differencer.h"

ABSL_FLAG(double, bpm, 0.0, "BPM (overrides MIDI file)");
ABSL_FLAG(std::string, time_signature, "", "Time Signature (overrides MIDI file");
ABSL_FLAG(double, fps, 60.0, "Target frames per second");

namespace converter {

using google::protobuf::util::MessageDifferencer;

std::tuple<int, int> CalculateMeasureFrames(const smf::MidiFile& midi, const proto::TimeSignature& tsig, double bpm) {
    double bps = bpm / 60.0;
    double beat = 1.0 / bps;
    double measure = tsig.numerator() * beat;
    int frames = (measure * absl::GetFlag(FLAGS_fps) + 0.5);
    int denominator = tsig.denominator();
    if (denominator == 0) denominator = 4;
    int ticks = midi.getTicksPerQuarterNote() * tsig.numerator() / (denominator / 4);
    return std::make_tuple(frames, ticks);
}

proto::Frame* GetFrame(proto::Track* track, int mnum, int fnum) {
    while(track->measures_size() <= mnum) {
        track->add_sequence(track->measures_size());
        track->add_measures();
    }
    proto::Measure* measure = track->mutable_measures(mnum);

    for(auto& frame : *measure->mutable_frames()) {
        if (frame.frame() == fnum) {
            return &frame;
        }
    }
    proto::Frame* frame = measure->add_frames();
    frame->set_frame(fnum);
    return frame;
}

bool Has(const std::map<std::string, std::string>& params, const std::string& key) {
    const auto it = params.find(key);
    return it != params.end();
}

int Get(const std::map<std::string, std::string>& params, const std::string& key) {
    const auto it = params.find(key);
    if (it != params.end()) {
        int val = strtol(it->second.c_str(), 0, 0);
        return val;
    }
    return 0;
}
int Get(const std::map<std::string, std::string>& params, const std::string& key, int keyn) {
    return Get(params, absl::StrCat(key, keyn));
}

void ConvertEvent(const smf::MidiEvent& event,
                  proto::Track* track, int mnum, int fnum,
                  const std::map<std::string, std::string>& params) {
    proto::Frame* frame = GetFrame(track, mnum, fnum);
    if (event.isNoteOn()) {
        auto* note = frame->mutable_note();
        note->set_kind(event.getVelocity() ? proto::Note_Kind_ON
                                           : proto::Note_Kind_OFF);
        note->set_note(event.getKeyNumber());
        note->set_velocity(event.getVelocity());
    } else if (event.isNoteOff()) {
        auto* note = frame->mutable_note();
        note->set_kind(proto::Note_Kind_OFF);
        note->set_note(event.getKeyNumber());
        note->set_velocity(event.getVelocity());
    } else if (event.isPatchChange()) {
        proto::OtherEvent* other = frame->add_other();
        int pgm = Get(params, "program", track->number()) - 1;
        if (pgm == -1) pgm = event.getP1();
        other->set_program_change(pgm);
    }
}

bool TimeSignatureEquals(const proto::TimeSignature& a, const proto::TimeSignature& b) {
    return a.numerator() == b.numerator()
        && a.denominator() == b.denominator();
}

void CheckMiscParams(proto::Track* track, int mnum, int fnum,
                   const std::map<std::string, std::string>& params) {
    if (track->number() != 0)
        return;
    if (Has(params, "next_song")) {
        proto::Frame* frame = GetFrame(track, mnum, fnum);
        proto::OtherEvent* other = frame->add_other();
        other->set_next_song(Get(params, "next_song"));
    }
    if (Has(params, "restart_last_song")) {
        proto::Frame* frame = GetFrame(track, mnum, fnum);
        proto::OtherEvent* other = frame->add_other();
        other->set_restart_last_song(Get(params, "restart_last_song"));
    }
    if (Has(params, "song_end")) {
        proto::Frame* frame = GetFrame(track, mnum, fnum);
        proto::OtherEvent* other = frame->add_other();
        other->set_song_end(Get(params, "song_end"));
    }
    const auto it = params.find("title_screen_hack");
    if (it != params.end()) {
        for (const auto& kv : absl::StrSplit(it->second, ':')) {
            const std::pair<std::string&, std::string&>& pair =
                            absl::StrSplit(kv, absl::MaxSplits('=', 1));
            int measure = strtol(pair.first.c_str(), 0, 0);
            int value = strtol(pair.second.c_str(), 0, 0);
            proto::Frame* frame = GetFrame(track, measure, 0);
            proto::OtherEvent* other = frame->add_other();
            other->set_title_screen_hack(value);
        }
    }
}

void OptimizeTrack(proto::Track *track,
                   const std::map<std::string, std::string>& params) {
    // We assume this function will be called immediately after constructing
    // the measures list, and thus the sequence will be ordered [0, 1, 2, ...].
    // We'll rebuild the sequence as we eliminate duplicates.
    track->clear_sequence();
    track->add_sequence(0);
    for(int i=1,j=0; i < track->measures_size();) {
        for(j=0; j < i; ++j) {
            if (MessageDifferencer::Equals(track->measures(i), track->measures(j))) {
                // Found a duplicate, remove it;
                track->mutable_measures()->DeleteSubrange(i, 1);
                break;
            }
        }
        track->add_sequence(j);
        if (j == i) {
            // If we didn't find a duplicate, advance to the next measure.
            ++i;
        }
    }
    if (Has(params, "loop")) {
        track->add_sequence(0x80 | Get(params, "loop"));
    }
}

void ConvertTrack(const smf::MidiFile& midi, proto::Song* song, int tnum,
                  const std::map<std::string, std::string>& params) {
    proto::Track *track = song->add_tracks();
    track->set_number(tnum);
    proto::TimeSignature tsig = song->time_signature();
    double tempo = song->bpm();
    int measure_frames = 0; // Number of NES frames per measure.
    int midi_tpm = 0; // Number of midi ticks per measure.
    int mnum = 0, fnum = 0;
    std::tie(measure_frames, midi_tpm) = CalculateMeasureFrames(midi, tsig, tempo);
    song->set_frames_per_measure(measure_frames);

    for(int evnum=0; evnum < midi[tnum].size(); ++evnum) {
        smf::MidiEvent event = midi[tnum][evnum];
        //printf("track=%d evnum=%d tm=%d(%f) (%02x %d %d)\n", tnum, evnum, event.tick, event.seconds, event.getP0(), event.getP1(), event.getP2());

        if (event.isTrackName()) {
            std::string name = event.getMetaContent();
            if (name.back() == '\0') {
                name.resize(name.size() - 1);
            }
            track->set_name(name);
        } else if (event.isTimeSignature()) {
            proto::TimeSignature zero;
            tsig.set_numerator(event[3]);
            tsig.set_denominator(1 << event[4]);
            if (TimeSignatureEquals(song->time_signature(), zero)) {
                *song->mutable_time_signature() = tsig;
            } else if (!TimeSignatureEquals(song->time_signature(), tsig)) {
                fprintf(stderr, "warning: track %d has an unexpected time signature of %s\n",
                        tnum, tsig.DebugString().c_str());
            }
            std::tie(measure_frames, midi_tpm) = CalculateMeasureFrames(midi, tsig, tempo);
            song->set_frames_per_measure(measure_frames);
        } else if (event.isTempo()) {
            tempo = event.getTempoBPM();
            //printf("Got tempo=%f\n", tempo);
            if (song->bpm() == 0.0) {
                song->set_bpm(tempo);
            } else if (song->bpm() != tempo) {
                fprintf(stderr, "warning: track %d has an unexpected tempo of %f\n",
                        tnum, tempo);
            }
            std::tie(measure_frames, midi_tpm) = CalculateMeasureFrames(midi, tsig, tempo);
            song->set_frames_per_measure(measure_frames);
        } else if (!event.isMetaMessage()) {
            mnum = event.tick / midi_tpm;
            fnum = (event.tick % midi_tpm) * measure_frames / midi_tpm;
            //printf("track=%d evnum=%d tm=%d(%f) measure=%d frame=%d\n", tnum, evnum, event.tick, event.seconds, mnum, fnum);
            ConvertEvent(event, track, mnum, fnum, params);
        }
    }
    CheckMiscParams(track, mnum, measure_frames-1, params);
    OptimizeTrack(track, params);
}

void AdjustTempo(smf::MidiFile& midi, double tempo) {
    int tracks = midi.getTrackCount();
    for(int track = 0; track < tracks; ++track) {
        for(int evnum=0; evnum < midi[track].size(); ++evnum) {
            smf::MidiEvent event = midi[track][evnum];
            if (event.isTempo()) {
                event.setTempo(tempo);
            }
        }
    }
}

// Convert a midi file to proto-representation.
proto::Song ConvertFile(const std::string& spec) {
    std::string filename;
    std::map<std::string, std::string> params;
    size_t comma = spec.find(',');
    if (comma != std::string::npos) {
        filename = spec.substr(0, comma);
        for (const auto& kv : absl::StrSplit(spec.substr(comma + 1), ',')) {
            params.insert(absl::StrSplit(kv, absl::MaxSplits('=', 1)));
        }
    } else {
        filename = spec;
    }

    smf::MidiFile midi;
    bool result = midi.read(filename);
    if (!result) {
        fprintf(stderr, "Could not read %s\n", filename.c_str());
        exit(1);
    }

    if (absl::GetFlag(FLAGS_bpm) > 0.0) {
        AdjustTempo(midi, absl::GetFlag(FLAGS_bpm));
    }
    midi.doTimeAnalysis();
    int tracks = midi.getTrackCount();

    proto::Song song;
    for(int track = 0; track < tracks; ++track) {
        ConvertTrack(midi, &song, track, params);
    }
    return song;
}
}  // namespace converter

const char kUsage[] =
R"ZZZ(<optional flags> [midi-file-and-spec ...]

Description:
  MIDI file converter.

Args:
  midi-file-and-spec can be:
    - A path to a midi filename
    - A path to a midi filename,key=value,...

  Supported key-value pairs:
    - program<n>=<p>: Override midi program change events on track <n> to use
                      midi program number <p>.

    - loop=<n>: At the end of the sequence loop to sequence number <n>

    - next_song=<n>: Start playing song <n> on the next frame.

    - song_end=1: Signal to the game that the song has ended.

    - restart_last_song=1: Restart the prior song on the next frame.

    - title_screen_hack=m=v:...: Set the title screen hack byte to the
                                 value <v> on frame 0 of measure <m>.
)ZZZ";

int main(int argc, char *argv[]) {
    absl::SetProgramUsageMessage(kUsage);
    auto args = absl::ParseCommandLine(argc, argv);

    for(size_t i=1; i<args.size(); ++i) {
        proto::Song song = converter::ConvertFile(args[i]);
        printf("%s\n", song.DebugString().c_str());
    }
    return 0;
}
