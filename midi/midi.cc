#include <stdint.h>
#include "midi/midi.h"

#include "nes/mem.h"
#include "util/file.h"
#include "midi/fti.h"

namespace protones {

int MidiConnector::notes_[128];

void MidiConnector::InitNotes(double a440) {
    double a0 = a440 / 16.0;
    for(int i=0; i<128; i++) {
        double n = i-21;
        if (n < 0.0) {
            notes_[i] = 0;
        } else {
            notes_[i] = a0 * std::pow(TRT, n);
        }
    }
}

void MidiConnector::InitEnables() {
    // Enable all channels in the APU.
    nes_->mem()->Write(0x4015, 0x1F);
    // Enable the pulse channels in MMC5.
    nes_->mem()->Write(0x5015, 0x03);
}

void MidiConnector::Emulate() {
    ProcessMessages();
    for(auto& channel : channel_) {
        channel.second->Step();
    }
}

void MidiConnector::ProcessMessages() {
    std::vector<uint8_t> message;

    for(;;) {
        midi_->getMessage(&message);
        if (message.size() == 0) {
            break;
        }
        ProcessMessage(message);
    }
}

void MidiConnector::ProcessMessage(const std::vector<uint8_t>& message) {
    int32_t ch = (message[0] & 0x0F) + 1;
    bool dispatched = false;
    for(const auto& channel : config_.channel()) {
        if (ch == channel.channel()) {
            Channel* chan = channel_[channel.name()].get();
            if (chan) {
                chan->ProcessMessage(message);
                dispatched = true;
            }
        }
    }
    if (!dispatched) {
        printf("No channel configured for message");
        for(const auto& data : message) {
            printf(" %02x", data);
        }
        printf("\n");
    }
}

void MidiConnector::LoadConfig(const std::string& filename) {
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
        auto fti = LoadFTI(inst.second);
        if (fti.ok()) {
            instrument_.emplace(inst.first, fti.ValueOrDie());
        } else {
            fprintf(stderr, "Error loading '%s': %s\n",
                    inst.first.c_str(),
                    fti.status().ToString().c_str());
        }
    }

    channel_.clear();
    for(const auto& channel : config_.channel()) {
        channel_.emplace(channel.name(),
                std::make_unique<Channel>(nes_,
                                          channel,
                                          instrument(channel.instrument())));
    }
}

proto::FTInstrument* MidiConnector::instrument(const std::string& name) {
    auto item = instrument_.find(name);
    if (item != instrument_.end()) {
        return &item->second;
    } else {
        return nullptr;
    }
}

void Channel::ProcessMessage(const std::vector<uint8_t>& message) {
    uint8_t op = message[0] & 0xF0;

#if 0
    printf("midi message:");
    for(const auto& data : message) {
        printf(" %02x", data);
    }
    printf("\n");
#endif

    switch(op) {
        case 0x90: // Note On
            NoteOn(message[1] + config_.note_offset(), message[2]);
            break;
        case 0x80: // Note Off
            NoteOff(message[1] + config_.note_offset());
            break;
        default:
            printf("Unhandled midi message:");
            for(const auto& data : message) {
                printf(" %02x", data);
            }
            printf("\n");

    }
}

void Channel::NoteOn(uint8_t note, uint8_t velocity) {
    if (config_.drumkit().empty()) {
        player_.emplace_back(instrument_);
        player_.back().NoteOn(note, velocity);
    } else {
        auto drum = config_.drumkit().find(note);
        if (drum != config_.drumkit().end()) {
            player_.emplace_back(nes_->midi()->instrument(drum->second.patch()));
            player_.back().NoteOn(drum->second.period(), velocity);
        } else {
            fprintf(stderr, "Unknown drum patch for midi note %d\n", note);
        }
    }
    printf("player_ size = %d\n", (int)player_.size());
}

void Channel::NoteOff(uint8_t note) {
    if (!config_.drumkit().empty()) {
        auto drum = config_.drumkit().find(note);
        if (drum != config_.drumkit().end()) {
            note = drum->second.period();
        }
    }
    for(auto& p : player_) {
        if (p.note() == note && !p.released()) {
            p.Release();
            break;
        }
    }

}

uint16_t Channel::OscBaseAddress(proto::MidiChannel::Oscillator oscillator) {
    switch(oscillator) {
        case proto::MidiChannel_Oscillator_MMC5_PULSE1:
            return 0x5000;
        case proto::MidiChannel_Oscillator_MMC5_PULSE2:
            return 0x5004;
        case proto::MidiChannel_Oscillator_PULSE1:
            return 0x4000;
        case proto::MidiChannel_Oscillator_PULSE2:
            return 0x4004;
        case proto::MidiChannel_Oscillator_TRIANGLE:
            return 0x4008;
        case proto::MidiChannel_Oscillator_NOISE:
            return 0x400c;
        default:
            fprintf(stderr, "Unknown oscillator type %d\n",
                    static_cast<int>(oscillator));
            return 0x4000;
    }
}

void Channel::Step() {
    int osc = 0;
    size_t nplayer = 0;
    // Program oscillators according to playing notes.
    for(auto& p : player_) {
        nplayer++;
        p.Step();
        proto::MidiChannel::Oscillator oscillator = config_.oscillator(osc++);
        if (nplayer < player_.size() && osc == config_.oscillator_size()) {
            // Pseudo-polyphony: If there are more players that oscillators,
            // skip until the last one.
            osc = config_.oscillator_size() - 1;
            continue;
        }

        uint16_t base = OscBaseAddress(oscillator);
        uint8_t vol = p.volume();
        uint8_t duty = p.duty();
        uint16_t timer = p.timer();
        uint8_t timer_hi = timer >> 8;
        switch(oscillator) {
            case proto::MidiChannel_Oscillator_MMC5_PULSE1:
            case proto::MidiChannel_Oscillator_MMC5_PULSE2:
            case proto::MidiChannel_Oscillator_PULSE1:
            case proto::MidiChannel_Oscillator_PULSE2:
                nes_->mem()->Write(base + 0, vol | 0x30 | (duty<<6));
                nes_->mem()->Write(base + 2, timer & 0xFF);
                if (timer_hi != last_timer_hi_[base+3]) {
                    nes_->mem()->Write(base + 3, timer_hi);
                    last_timer_hi_[base+3] = timer_hi;
                }
                break;
            case proto::MidiChannel_Oscillator_TRIANGLE:
                nes_->mem()->Write(base + 0, vol ? 0x9f : 0x80);
                nes_->mem()->Write(base + 2, timer & 0xFF);
                if (timer_hi != last_timer_hi_[base+3]) {
                    nes_->mem()->Write(base + 3, timer_hi);
                    last_timer_hi_[base+3] = timer_hi;
                }
                break;
            case proto::MidiChannel_Oscillator_NOISE:
                nes_->mem()->Write(base + 0, vol | 0x30);
                if (p.note() != last_timer_hi_[base+2]) {
                    nes_->mem()->Write(base + 2, p.note() % 16);
                    nes_->mem()->Write(base + 3, 0xF8);
                    last_timer_hi_[base+2] = p.note();
                }
                break;
            default:
                fprintf(stderr, "Don't know how to program oscillator type %d\n",
                        static_cast<int>(oscillator));
        }
    }

    // Silence any unprogrammed oscillator
    for(; osc < config_.oscillator_size(); osc++) {
        proto::MidiChannel::Oscillator oscillator = config_.oscillator(osc);
        uint16_t base = OscBaseAddress(oscillator);
        switch(oscillator) {
            case proto::MidiChannel_Oscillator_MMC5_PULSE1:
            case proto::MidiChannel_Oscillator_MMC5_PULSE2:
            case proto::MidiChannel_Oscillator_PULSE1:
            case proto::MidiChannel_Oscillator_PULSE2:
                nes_->mem()->Write(base + 0, 0x30);
                last_timer_hi_[base+3] = 0xFF;
                break;
            case proto::MidiChannel_Oscillator_TRIANGLE:
                nes_->mem()->Write(base + 0, 0x80);
                last_timer_hi_[base+3] = 0xFF;
                break;
            case proto::MidiChannel_Oscillator_NOISE:
                nes_->mem()->Write(base + 0, 0x30);
                last_timer_hi_[base+2] = 0xFF;
                break;
            default:
                ;
        }
    }

    // Delete notes which are no longer playing
    for(auto it = player_.begin(); it != player_.end(); ) {
        if (it->done()) {
            it = player_.erase(it);
        } else {
            ++it;
        }
    }
}

int8_t Envelope::value() {
    if (envelope_ == nullptr
        || envelope_->sequence_size() == 0
        || state_ == STATE_OFF) {
        return value_;
    }
    return envelope_->sequence(frame_);
}

void Envelope::Step() {
    switch(state_) {
        case STATE_OFF:
            break;

        case STATE_ON:
            if (envelope_ != nullptr) {
                if (frame_ == envelope_->release()) {
                    // We've reached the release point, but aren't released
                    // yet, so go to the loop point.
                    frame_ = envelope_->loop();
                    break;
                } else if (frame_ == envelope_->sequence_size()) {
                    // We've reached the end, so go to the loop point or
                    // last element of the envelope until we get released.
                    frame_ = envelope_->loop() >= 0 && envelope_->release() < 0
                        ? envelope_->loop()
                        : envelope_->sequence_size() - 1;
                    break;
                }
            }
            frame_ += 1;
            break;

        case STATE_RELEASE:
            frame_ += 1;
            break;
    }
    if (envelope_ != nullptr
        && envelope_->kind() != proto::Envelope_Kind_UNKNOWN
        && frame_ >= envelope_->sequence_size()) {
        Reset(STATE_OFF);
    }
}

void Envelope::Reset(int state) {
    state_ = state;
    frame_ = 0;
    value_ = state == STATE_OFF ? 0 : default_;
}

void Envelope::NoteOn() {
    Reset(STATE_ON);
}

void Envelope::Release() {
    if (envelope_ != nullptr
        && envelope_->kind() != proto::Envelope_Kind_UNKNOWN) {
        if (envelope_->release() >= 0) {
            frame_ = envelope_->release();
        }
        state_ = STATE_RELEASE;
    } else {
        state_ = STATE_OFF;
    }
}

void InstrumentPlayer::NoteOn(uint8_t note, uint8_t velocity) {
    note_ = note;
    velocity_ = velocity;
    volume_.NoteOn();
    arpeggio_.NoteOn();
    pitch_.NoteOn();
    duty_.NoteOn();
}

void InstrumentPlayer::Release() {
    released_ = true;
    volume_.Release();
    arpeggio_.Release();
    pitch_.Release();
    duty_.Release();
}

void InstrumentPlayer::Step() {
    volume_.Step();
    arpeggio_.Step();
    pitch_.Step();
    duty_.Step();
}

bool InstrumentPlayer::done() {
    return volume_.done() &&
           arpeggio_.done() &&
           pitch_.done() &&
           duty_.done();
}

uint8_t InstrumentPlayer::volume() {
    if (done()) return 0;

    double v = double(velocity_) / 127.0;
    double env = double(volume_.value()) / 15.0;
    double value = v * env * 15.0;
    if (value < 1.0 && v > 0.0 && env > 0.0) {
        // If there is even a small amount of volume
        // clamp to a minimum value of 1.
        value = 1.0;
    }
    return uint8_t(value);
}

uint8_t InstrumentPlayer::duty() {
    return duty_.value();
}

uint16_t InstrumentPlayer::timer() {
    int freq = MidiConnector::notes_[note_] + pitch_.value();
    if (freq == 0) return 0;
    int t = (NES::frequency / (16 * freq)) - 1;
    if (t > 2047) t = 0;
    return t;
}

}  // namespace protones
