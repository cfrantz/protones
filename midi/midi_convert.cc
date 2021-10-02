#include <cstdio>
#include <string>
#include <gflags/gflags.h>

#include "MidiFile.h"
#include "proto/midi_convert.pb.h"
#include "google/protobuf/util/message_differencer.h"

DEFINE_double(bpm, 0.0, "BPM (overrides MIDI file)");
DEFINE_string(time_signature, "", "Time Signature (overrides MIDI file");
DEFINE_double(fps, 60.0, "Target frames per second");

namespace converter {

using google::protobuf::util::MessageDifferencer;

int CalculateMeasureFrames(const smf::MidiFile& midi, const proto::TimeSignature& tsig, double bpm) {
    double bps = bpm / 60.0;
    double beat = 1.0 / bps;
    double measure = tsig.numerator() * beat;
    int frames = (measure * FLAGS_fps + 0.5);
    return frames;
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

void ConvertEvent(const smf::MidiEvent& event, proto::Frame *frame) {
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
        other->set_program_change(event.getP1());
    }
}

bool TimeSignatureEquals(const proto::TimeSignature& a, const proto::TimeSignature& b) {
    return a.numerator() == b.numerator()
        && a.denominator() == b.denominator();
}

void OptimizeTrack(proto::Track *track) {
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
}

void ConvertTrack(const smf::MidiFile& midi, proto::Song* song, int tnum) {
    proto::Track *track = song->add_tracks();
    track->set_number(tnum);
    proto::TimeSignature tsig = song->time_signature();
    double tempo = song->bpm();
    int measure_frames = CalculateMeasureFrames(midi, tsig, tempo);

    for(int evnum=0; evnum < midi[tnum].size(); ++evnum) {
        smf::MidiEvent event = midi[tnum][evnum];

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
            measure_frames = CalculateMeasureFrames(midi, tsig, tempo);
        } else if (event.isTempo()) {
            tempo = event.getTempoBPM();
            if (song->bpm() == 0.0) {
                song->set_bpm(tempo);
            } else if (song->bpm() != tempo) {
                fprintf(stderr, "warning: track %d has an unexpected tempo of %f\n",
                        tnum, tempo);
            }
            measure_frames = CalculateMeasureFrames(midi, tsig, tempo);
        } else {
            int mnum = 0, fnum = 0;
            int frame = (event.seconds * FLAGS_fps + 0.5);
            if (measure_frames != 0) {
                mnum = frame / measure_frames;
                fnum = frame % measure_frames;
            }
            //printf("track=%d tm=%f (%d %% %d) measure=%d frame=%d\n", tnum, event.seconds, frame, measure_frames, mnum, fnum);
            ConvertEvent(event, GetFrame(track, mnum, fnum));
        }
    }
    OptimizeTrack(track);
}

// Convert a midi file to proto-representation.
proto::Song ConvertFile(const std::string& filename) {
    smf::MidiFile midi;
    bool result = midi.read(filename);
    if (!result) {
        fprintf(stderr, "Could not read %s\n", filename.c_str());
        exit(1);
    }

    midi.doTimeAnalysis();
    int tracks = midi.getTrackCount();

    proto::Song song;
    for(int track = 0; track < tracks; ++track) {
        ConvertTrack(midi, &song, track);
    }
    return song;
}
}  // namespace converter

const char kUsage[] =
R"ZZZ(<optional flags> [midi-file]

Description:
  MIDI file converter.

Flags:
)ZZZ";

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage(kUsage);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc > 1) {
        proto::Song song = converter::ConvertFile(argv[1]);
        printf("%s\n", song.DebugString().c_str());
    }
    return 0;
}
