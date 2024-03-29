#include <cstdio>
#include <string>
#include <memory>
#include <map>

#include "proto/midi_convert.pb.h"
#include "util/file.h"
#include "google/protobuf/text_format.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "midi/fti.h"
#include "proto/fti.pb.h"
#include "proto/midi.pb.h"

ABSL_FLAG(double, fps, 60.0, "Target frames per second");
ABSL_FLAG(std::string, config, "", "MIDI configuration textproto file");
ABSL_FLAG(size_t, songtable_size, 128, "Length of the song pointer table");
ABSL_FLAG(int, drumtable_size, -1, "Length of the drum period/patch table");
ABSL_FLAG(double, volume, 1.0, "MIDI volume scaling");

namespace converter {

// asm-level "measure" opcodes.
enum CFplayer: uint8_t {
    // Delay events: 0x80 - 0xFF
    END = 0x00,
    // Volume events: 0x01 - 0x10 (set vol to value & 0x0F).
    NOTE_OFF = 0x11,
    SONG_END = 0x12,
    NEXT_SONG = 0x13,
    RESTART_LAST_SONG = 0x14,
    TITLE_SCREEN_HACK = 0x15,
    // Undefined: 0x14 - 0x1F
    PROGRAM_CHANGE = 0x20,
    // Note On events: 0x21 - 0x77 (midi note A0 through B7)
    // Undefined: 0x78 - 0x7F.
};

// Prints a string using only valid symbol characters ([0-9A-Fa-F_]).
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

// Remember which instruments get used by converted songs.
// Map midi program (instrument) numbers to a local (asm) instrument number.
class InstrumentManager {
  public:
    void LoadConfig(proto::MidiConfig* config) {
        config_ = config;
        instrument_.clear();
        for(const auto& inst : config_->instruments()) {
            auto fti = protones::LoadFTI(inst.second);
            if (fti.ok()) {
                instrument_.emplace(inst.first, fti.value());
            } else {
                fprintf(stderr, "Error loading '%s': %s\n",
                        inst.first.c_str(),
                        fti.status().ToString().c_str());
            }
        }
    }
    
    int32_t UseInstrument(int32_t program) {
        // Midi messages zero-index program numbers, but all user
        // docs/interfaces one-index them.
        program += 1;
        std::string instname = (*config_->mutable_midi_program())[program];
        if (instname.empty()) {
            fprintf(stderr, "Undefined midi program number %d\n", program);
            return -1;
        }
        return UseInstrumentByName(instname);
    }

    int32_t UseInstrumentByName(const std::string& instname) {
        size_t i;
        for(i=0; i < used_.size(); ++i) {
            if (used_[i] == instname)
                return int32_t(i);
        }
        used_.push_back(instname);
        return int32_t(i);
    }

    bool IsDrumChannel(int32_t chan) {
        // In midi files, channel numbers are zero based, but in all user
        // docs/interfaces, they're one-based.
        chan += 1;
        for(const auto& c : config_->channel()) {
            if (c.channel() == chan) {
                if (c.drumkit_size() > 0) {
                    return true;
                }
            }
        }
        return false;
    }

    const proto::MidiChannel* DrumChannel(int32_t chan) {
        // In midi files, channel numbers are zero based, but in all user
        // docs/interfaces, they're one-based.
        chan += 1;
        for(const auto& c : config_->channel()) {
            if (c.channel() == chan) {
                if (c.drumkit_size() > 0) {
                    return &c;
                } else {
                    fprintf(stderr, "Found drum channel, but no drumkit!\n");
                }
            }
        }
        return nullptr;
    }

    int32_t UseDrum(size_t chan, uint8_t midi_note) {
        // Already know the mapping for this drum sound?
        auto note = drum_notes_.find(midi_note);
        if (note != drum_notes_.end()) {
            return note->second;
        }

        const proto::MidiChannel* c = DrumChannel(chan);
        auto drum = c->drumkit().find(midi_note);
        if (drum == c->drumkit().end()) {
            fprintf(stderr, "No drum sound defined for midi note %d\n", midi_note);
            return -1;
        }

        // Remap MIDI drum note numbers to consecutive integers starting at
        // 0x21.  This lets us build a compact table describing how which
        // patch (instrument/envelope) and period should be used with the
        // noise channel.
        drum_patch_.push_back(UseInstrumentByName(drum->second.patch()));
        drum_period_.push_back(drum->second.period());
        int32_t n = drum_notes_.size() + 0x21;
        drum_notes_[midi_note] = n;
        return n;
    }

    std::string EnvName(const std::string& name, proto::Envelope_Kind kind) {
        const char *type;
        switch(kind) {
            case proto::Envelope_Kind_VOLUME: type = "volume"; break;
            case proto::Envelope_Kind_ARPEGGIO: type = "arpeggio"; break;
            case proto::Envelope_Kind_PITCH: type = "pitch"; break;
            case proto::Envelope_Kind_HIPITCH: type = "hipitch"; break;
            case proto::Envelope_Kind_DUTY: type = "duty"; break;
            default:
                fprintf(stderr, "Don't know how to deal with envelope kind %d\n", kind);
                type = "unknown";
        }
        return Symbol(absl::StrCat("env_", name, "_", type));
    }

    bool EnvEmpty(const proto::Envelope* env) {
        return env->sequence_size() == 0;
    }

    std::string RenderEnvelope(const std::string& name, const proto::Envelope* env) {
        std::string r;
        if (EnvEmpty(env))
            return r;

        // The layout of the envelope data is:
        // LABEL:
        //     .BYT <length>, <loop-index>, <release-index>, <data ...>
        // Because the data is offset 3-bytes from the label, the length
        // and loop/release indices are offset by 3 byte.

        uint8_t size = env->sequence_size() + 3;
        // If no loop point, then the loop point is the last element in
        // the sequence.
        int32_t loop = env->loop();
        if (loop < 0) loop = env->sequence_size() - 1;
        loop += 3;
        // If no release point, then relase is 0, which is detected as an
        // invalid index in the asm player.
        int32_t release = env->release();
        release = (release < 0) ? 0 : release + 3;

        absl::StrAppendFormat(&r, "%s:\n", EnvName(name, env->kind()));
        absl::StrAppendFormat(&r, "    .BYT $%02x,$%02x,$%02x", size, loop, release);
        for(int32_t val : env->sequence()) {
            if (env->kind() == proto::Envelope_Kind_VOLUME) {
                // Adjust the volume to the upper nybble so we don't have
                // to do it in the player.
                val <<= 4;
            }
            absl::StrAppendFormat(&r, ",$%02x", val & 0xFF);
        }
        absl::StrAppendFormat(&r, "\n");
        return r;
    }

    std::string VRC7RegsName(const std::string& name) {
        return Symbol(absl::StrCat("vrc7regs_", name));
    }

    std::string RenderVRC7Regs(const std::string& name, const proto::VRC7Patch& patch) {
        std::string r;

        absl::StrAppendFormat(&r, "%s:\n", VRC7RegsName(name));
        size_t n = 0;
        for(int32_t val : patch.regs()) {
            if (n == 0) {
                absl::StrAppendFormat(&r, "    .BYT $%02x", val);
            } else {
                absl::StrAppendFormat(&r, ",$%02x", val);
            }
            n += 1;
        }
        absl::StrAppendFormat(&r, "\n");
        return r;
    }

    std::string RenderInstruments() {
        std::string r;

        absl::StrAppendFormat(&r, ".export _instruments_table\n");
        absl::StrAppendFormat(&r, "_instruments_table:\n");
        const std::string zero("0");
        for (const auto& name : used_) {
            const auto& instrument = instrument_[name];
            const auto& v = &instrument.volume();
            const auto& a = &instrument.arpeggio();
            const auto& p = &instrument.pitch();
            const auto& d = &instrument.duty();

            if (instrument.kind() == proto::FTInstrument_Kind_VRC7) {
                int32_t patch = instrument.vrc7().patch();
                std::string pn;
                if (patch == 0) {
                    pn = VRC7RegsName(name);
                } else {
                    absl::StrAppendFormat(&pn, "$%04x", patch);
                }
                absl::StrAppendFormat(&r, "    ; Instrument name: '%s', type: VRC7\n    .WORD %s,%s,%s,%s\n",
                        name,
                        EnvEmpty(v) ? zero : EnvName(name, v->kind()).c_str(),
                        EnvEmpty(a) ? zero : EnvName(name, a->kind()).c_str(),
                        EnvEmpty(p) ? zero : EnvName(name, p->kind()).c_str(),
                        pn);
            } else {
                absl::StrAppendFormat(&r, "    ; Instrument name: '%s', type: 2A03\n    .WORD %s,%s,%s,%s\n",
                        name,
                        EnvEmpty(v) ? zero : EnvName(name, v->kind()).c_str(),
                        EnvEmpty(a) ? zero : EnvName(name, a->kind()).c_str(),
                        EnvEmpty(p) ? zero : EnvName(name, p->kind()).c_str(),
                        EnvEmpty(d) ? zero : EnvName(name, d->kind()).c_str());
            }
        }

        absl::StrAppendFormat(&r, "\n");
        for (const auto& name : used_) {
            const auto& instrument = instrument_[name];
            std::string v = RenderEnvelope(name, &instrument.volume());
            std::string a = RenderEnvelope(name, &instrument.arpeggio());
            std::string p = RenderEnvelope(name, &instrument.pitch());
            std::string d;
            if (instrument.kind() == proto::FTInstrument_Kind_VRC7) {
                if (instrument.vrc7().patch() == 0) {
                    d = RenderVRC7Regs(name, instrument.vrc7());
                }
            } else {
                d = RenderEnvelope(name, &instrument.duty());
            }
            absl::StrAppend(&r, v, a, p, d);
        }
        return r;
    }

    std::string RenderDrums(int len) {
        std::string r;

        absl::StrAppendFormat(&r, ".export _drum_period\n");
        absl::StrAppendFormat(&r, "_drum_period:\n");
        int sz = int(drum_period_.size());
        int n = len == -1 ? sz : len;
        for(int i=0; i<n; i++) {
            uint8_t period = i < sz ? drum_period_[i] : 0x00;
            absl::StrAppendFormat(&r, "    .BYTE $%02x\n", period);
        }

        absl::StrAppendFormat(&r, ".export _drum_patch\n");
        absl::StrAppendFormat(&r, "_drum_patch:\n");
        sz = int(drum_patch_.size());
        n = len == -1 ? sz : len;
        for(int i=0; i<n; i++) {
            uint8_t patch = i < sz ? drum_patch_[i] : 0x00;
            absl::StrAppendFormat(&r, "    .BYTE $%02x\n", patch << 3);
        }
        return r;
    }

  private:
    proto::MidiConfig* config_;
    std::map<std::string, proto::FTInstrument> instrument_;
    // List of insrtuments used.
    std::vector<std::string> used_;

    std::map<uint8_t, uint8_t> drum_notes_;
    std::vector<uint8_t> drum_period_;
    std::vector<uint8_t> drum_patch_;
};

// Convert a Track to it's assembly level representation.
class TrackPlayer {
  public:
    TrackPlayer(const proto::Track* track, InstrumentManager* manager, int measure_frames)
      : track_(track),
        manager_(manager),
        measure_frames_(measure_frames)
    {
        drum_channel_ = manager_->IsDrumChannel(track_->number());
    }

    // Add a delay opcode to the asm representation.
    void RenderDelay(std::vector<uint8_t>* m, int delay) {
        while(delay) {
            int d = std::min(delay, 128);
            m->push_back(-d);
            delay -= d;
        }
    }

    // Add non-note opcodes to the asm-representation.
    // This includes:
    // - Program Change
    void RenderOther(const proto::Frame* frame, std::vector<uint8_t> *m) {
        for (const auto& other : frame->other()) {
            switch(other.event_case()) {
                case proto::OtherEvent::EventCase::EVENT_NOT_SET:
                    break;
                case proto::OtherEvent::EventCase::kProgramChange:
                {
                    int32_t p = manager_->UseInstrument(other.program_change());
                    if (p >= 0) {
                        m->push_back(CFplayer::PROGRAM_CHANGE);
                        // pre-multiply the instrument number by 8 so the
                        // asm player doesn't have to.
                        m->push_back(uint8_t(p << 3));
                    }
                }
                    break;
                case proto::OtherEvent::EventCase::kSongEnd:
                    if (other.song_end()) {
                        m->push_back(CFplayer::SONG_END);
                    }
                    break;
                case proto::OtherEvent::EventCase::kRestartLastSong:
                    if (other.restart_last_song()) {
                        m->push_back(CFplayer::RESTART_LAST_SONG);
                    }
                    break;
                case proto::OtherEvent::EventCase::kNextSong:
                    m->push_back(CFplayer::NEXT_SONG);
                    m->push_back(uint8_t(other.next_song()));
                    break;
                case proto::OtherEvent::EventCase::kTitleScreenHack:
                {
                    m->push_back(CFplayer::TITLE_SCREEN_HACK);
                    m->push_back(uint8_t(other.title_screen_hack()));
                }
                    break;
                default:
                    fprintf(stderr, "Unknown `other` event %s\n",
                            other.DebugString().c_str());
            }
        }
    }

    // Convert a measure into asm-representation.
    void RenderMeasures() {
        uint8_t last_volume = 0;
        for(const auto& measure : track_->measures()) {
            measures_.emplace_back();
            auto& m = measures_.back();
            int last_frame = 0;

            for(const auto& frame : measure.frames()) {
                RenderDelay(&m, frame.frame() - last_frame);
                last_frame = frame.frame();
                RenderOther(&frame, &m);
                const auto& note = frame.note();
                if (note.kind() == proto::Note_Kind_NONE) continue;
                if (note.kind() == proto::Note_Kind_ON) {
                    double vscale = absl::GetFlag(FLAGS_volume);
                    int volume = std::min(double(note.velocity()) * vscale / 8.0, 15.0);
                    if (volume != last_volume) {
                        // 0x01 - 0x10: set volume to (val&0xF)
                        m.push_back(volume ? volume : 0x10);
                        last_volume = volume;
                    }
                    if (drum_channel_) {
                        m.push_back(manager_->UseDrum(track_->number(), note.note()));
                    } else {
                        m.push_back(note.note() + note_offset_);
                    }
                } else if (note.kind() == proto::Note_Kind_OFF) {
                    m.push_back(CFplayer::NOTE_OFF);
                } else {
                    //error
                }
            }

            RenderDelay(&m, measure_frames_ - last_frame);
            m.push_back(CFplayer::END);
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

    // Print the asm-representation of a rendered track.
    std::string ToText(const std::string& songname) {
        std::string r;
        size_t mn = 0;
        absl::StrAppendFormat(&r, "%s:\n", DataName(songname));
        absl::StrAppendFormat(&r, "    .WORD %s\n", SequenceName(songname));
        for(mn=0; mn < measures_.size(); ++mn) {
            absl::StrAppendFormat(&r, "    .WORD %s\n", MeasureName(songname, mn+1));
        }

        absl::StrAppendFormat(&r, "%s:\n", SequenceName(songname));
        absl::StrAppendFormat(&r, "    .BYT ");
        size_t i = 0;
        fprintf(stderr, "INFO: %s length=%d\n",
                SequenceName(songname).c_str(), track_->sequence_size());
        for(auto val : track_->sequence()) {
            // In the protofile, sequences index into measures as zero-based,
            // but are 1-based in the assembly.
            if (val >=0 && val <= 127) {
                val += 1;
            }
            absl::StrAppendFormat(&r, "%c$%02x", i == 0 ? ' ' : ',', uint8_t(val));
            ++i;
        }
        absl::StrAppendFormat(&r, ",$00\n");

        mn = 0;
        for(const auto& measure : measures_) {
            ++mn;
            absl::StrAppendFormat(&r, "%s:\n", MeasureName(songname, mn));
            absl::StrAppendFormat(&r, "    .BYT ");
            for(size_t i=0; i<measure.size(); ++i) {
                absl::StrAppendFormat(&r, "%c$%02x", i == 0 ? ' ' : ',', measure[i]);
            }
            absl::StrAppendFormat(&r, "\n");
        }
        return r;
    }

    // Render a track to assembly.
    std::string Render(const std::string& songname) {
        RenderMeasures();
        return ToText(songname);
    }

    void set_note_offset(int offset) { note_offset_ = offset; }

  private:
    const proto::Track *track_;
    InstrumentManager* manager_;
    int measure_frames_;
    int seqnum_ = 0;
    int frame_ = 0;
    int note_offset_ = 0;
    std::vector<std::vector<uint8_t>> measures_;
    bool drum_channel_;
};

// Renders an entire song to the asm-representation.
class SongPlayer {
  public:
    SongPlayer()
      {}

    // Reads a textpb-representation of the song.
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
            std::string title = File::Basename(filename);
            size_t ext = title.rfind(".");
            if (ext != std::string::npos) {
                title = title.substr(0, ext);
            }
            song_.set_title(title);
        }
        player_.clear();
        for(const auto& track : song_.tracks()) {
            int32_t tracknum = track.number();
            auto tp = TrackPlayer(&track, &manager_, MeasureFrames());
            for(const auto& channel : config_.channel()) {
                // Track numbers are 1-based in the config file.
                if (channel.channel() == tracknum + 1) {
                    if (channel.nes_channel() > 0) {
                        tracknum = channel.nes_channel() - 1;
                    } else {
                        tracknum = channel.channel() - 1;
                    }
                    tp.set_note_offset(channel.note_offset());
                    break;
                }
            }
            player_.emplace(tracknum, tp);
        }
        return true;
    }

    // Read the midi config file which contains the channel and instrument
    // mappings.
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
        manager_.LoadConfig(&config_);
    }

    // Compute the number of frames in a measure.
    int MeasureFrames() {
        double bps = song_.bpm() / 60.0;
        double beat = 1.0 / bps;
        double measure = song_.time_signature().numerator() * beat;
        int frames = (measure * absl::GetFlag(FLAGS_fps) + 0.5);
        return frames;
    }

    // Print the song tracks in "naive" ordering, which is the smae order 
    // they appear in the proto file.
    std::string NaiveOrder(const std::string& title) {
        std::string r;
        absl::StrAppendFormat(&r, "    .BYT %d, 0\n", song_.tracks_size());
        for(auto& pair : player_) {
            absl::StrAppendFormat(&r, "    .WORD %s\n", pair.second.DataName(title));
        }
        return r;
    }

    // Print the song tracks in "config" ordering, which is the order of the
    // channels defined in the config (which should match the ordering of
    // the APU channels on the NES).
    std::string ConfigOrder(const std::string& title) {
        std::string r;
        int32_t maxtrack = 0;
        for (const auto & p : player_) {
            maxtrack = std::max(maxtrack, p.first);
        }
        maxtrack += 1;

        absl::StrAppendFormat(&r, "    .BYT %d, 0\n", maxtrack);
        for(int32_t chan=0; chan < maxtrack; chan++) {
            // config uses 1-based MIDI channel numbers, while the player
            // uses zero-based channel numbers.
            auto it = player_.find(chan);
            if (it != player_.end()) {
                absl::StrAppendFormat(&r, "    .WORD %s\n", it->second.DataName(title));
            } else {
                absl::StrAppendFormat(&r, "    .WORD 0\n");
            }
        }
        return r;
    }

    // Render the song to asm-representation.
    std::string Render() {
        std::string r;
        std::string title = Symbol(song_.title());
        absl::StrAppendFormat(&r, ".export _%s\n", title);
        absl::StrAppendFormat(&r, "_%s:\n", title);
        if (config_.channel().empty()) {
            absl::StrAppend(&r, NaiveOrder(title));
        } else {
            absl::StrAppend(&r, ConfigOrder(title));
        }
        for(auto& pair : player_) {
            absl::StrAppend(&r, pair.second.Render(title));
        }
        return r;
    }

    // Render the known instruments to asm-representation.
    std::string RenderInstruments() {
        std::string inst = manager_.RenderInstruments();
        std::string drums = manager_.RenderDrums(absl::GetFlag(FLAGS_drumtable_size));
        return absl::StrCat(drums, "\n", inst);
    }

    const std::string& title() { return song_.title(); }

  private:
    proto::Song song_;
    proto::MidiConfig config_;
    InstrumentManager manager_;
    std::map<int32_t, TrackPlayer> player_;
};

}  // namespace converter

const char kUsage[] =
R"ZZZ(<optional flags> [proto-files ...]

Description:
  Proto-MIDI file player.

Flags:
)ZZZ";

int main(int argc, char *argv[]) {
    absl::SetProgramUsageMessage(kUsage);
    auto args = absl::ParseCommandLine(argc, argv);

    converter::SongPlayer sp;
    if (!absl::GetFlag(FLAGS_config).empty()) {
        sp.LoadConfig(absl::GetFlag(FLAGS_config));
    }
    if (argc < 2) {
        fprintf(stderr, "No files specified!");
        return 1;
    }

    std::string song_table;
    std::string songs;
    absl::StrAppendFormat(&song_table, ".export _song_table\n");
    absl::StrAppendFormat(&song_table, "_song_table:\n");
    std::map<std::string, bool> rendered; 
    size_t i;
    for(i=1; i<args.size(); ++i) {
        std::string arg(args[i]);
        if (arg.empty()) {
            absl::StrAppendFormat(&song_table, "    .WORD 0\n");
        } else {
            sp.Load(arg);
            absl::StrAppendFormat(&song_table, "    .WORD _%s\n", converter::Symbol(sp.title()));
            if (!rendered[arg]) {
                absl::StrAppend(&songs, sp.Render());
                rendered[arg] = true;
            }
        }
    }
    for(; i < absl::GetFlag(FLAGS_songtable_size) + 1; ++i) {
        absl::StrAppendFormat(&song_table, "    .WORD 0\n");
    }
    absl::PrintF("%s\n%s\n%s\n",
                 song_table,
                 sp.RenderInstruments(),
                 songs);

    return 0;
}
