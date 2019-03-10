#include <cmath>
#include <vector>
#include "nes/midi.h"

namespace protones {

const char* MidiConnector::notes_[128] = {
                          "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1",
    "A-1", "A#-1", "B-1", "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0",
    "A0", "A#0", "B0", "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1",
    "A1", "A#1", "B1", "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2",
    "A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3",
    "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4",
    "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5",
    "A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6",
    "A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7",
    "A7", "A#7", "B7", "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8",
    "A8", "A#8", "B8", "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9",
};


void MidiConnector::InitFreqTable(double a4) {
    // am1 is the frequency of midi note #9 AKA "A-1"
    double am1 = a4 / 32.0;
    for(int i=0; i<9; ++i) {
        freq_[i] = 0;
    }
    for(int i=9; i<128; ++i) {
        freq_[i] = am1 * std::pow(TRT, double(i-9));
    }
}

int MidiConnector::FindFreqIndex(double f) {
    for(int i=0; i<127; ++i) {
        if (f >= freq_[i] && f < freq_[i+1]) {
            // Which frequency is F closer to.
            if (f - freq_[i] < freq_[i+1] - f) {
                return i;
            } else {
                return i+1;
            }
        }
    }
    return 0;
}

void MidiConnector::NoteOnFreq(uint8_t chan, double f, uint8_t velocity) {
    if (chan >= NCHAN)
        return;
    NoteOn(chan, FindFreqIndex(f), velocity);
}

void MidiConnector::NoteOn(uint8_t chan, uint8_t note, uint8_t velocity) {
    if (chan >= NCHAN)
        return;

    float adjusted = chan_volume_[chan] * velocity;
    velocity = (adjusted < 128.0) ? uint8_t(adjusted) : 127;
    Note& n = channel_[chan];
    if (note == n.note && velocity == n.velocity) {
        // we just played this same note.
        return;
    }

    NoteOff(chan);
    n.note = note;
    n.velocity = velocity;
    if (enabled_ && chan_volume_[chan] > 0.0f && n.note) {
        std::vector<uint8_t> msg = { uint8_t(0x90|chan), n.note, n.velocity };
        midi_->sendMessage(&msg);
        //printf("NoteOn:  %d %02x %02x\n", chan, n.note, n.velocity);
    }
}

void MidiConnector::NoteOff(uint8_t chan) {
    if (chan >= NCHAN)
        return;

    Note& n = channel_[chan];
    if (enabled_ && n.note) {
        std::vector<uint8_t> msg = { uint8_t(0x80|chan), n.note, n.velocity };
        midi_->sendMessage(&msg);
        //printf("NoteOff: %d %02x %02x\n", chan, n.note, n.velocity);
    }
    n.note = 0;
    n.velocity = 0;
}

}  // namespace protones
