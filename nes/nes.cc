
#include <gflags/gflags.h>
#include <unistd.h>
#include "imgui.h"
#include "google/protobuf/text_format.h"

#include "nes/nes.h"

#include "nes/cpu6502.h"
#include "nes/apu.h"
#include "nes/cartridge.h"
#include "nes/controller.h"
#include "nes/fm2.h"
#include "nes/mapper.h"
#include "nes/mem.h"
#include "midi/midi.h"
#include "nes/ppu.h"
#include "proto/config.pb.h"
#include "util/config.h"

DEFINE_string(fm2, "", "FM2 Movie file.");
DEFINE_string(midi, "", "Midi configuration textpb.");
DEFINE_double(fps, 60.0988, "Desired NES fps.");
namespace protones {

using namespace std::placeholders;
using proto::ControllerButtons;

const uint32_t standard_palette[] = {
    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4,
    0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08,
    0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE,
    0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
    0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32,
    0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF,
    0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
    0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082,
    0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
    0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF,
    0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
    0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC,
    0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000,
};

NES::NES() :
    pause_(false),
    step_(false),
    debug_(false),
    reset_(false),
    lag_(false),
    has_movie_(false),
    frame_(0),
    remainder_(0)
{
    mem_ = new Mem(this);
    devices_.emplace_back(mem_);

    apu_ = new APU(this);
    devices_.emplace_back(apu_);

    cpu_ = new Cpu();
    cpu_->memory(mem_);
    devices_.emplace_back(cpu_);

    movie_ = new FM2Movie(this);
    devices_.emplace_back(movie_);

    ppu_ = new PPU(this);
    devices_.emplace_back(ppu_);

    cart_ = new Cartridge(this);
    devices_.emplace_back(cart_);

    midi_ = new MidiConnector(this);
    devices_.emplace_back(midi_);
    if (!FLAGS_midi.empty()) {
        midi_->LoadConfig(FLAGS_midi);
    }

    controller_[0] = new Controller(this, 0);
    controller_[1] = new Controller(this, 1);
    controller_[2] = new Controller(this, 2);
    controller_[3] = new Controller(this, 3);
    devices_.emplace_back(controller_[0]);
    devices_.emplace_back(controller_[1]);
    devices_.emplace_back(controller_[2]);
    devices_.emplace_back(controller_[3]);

    mapper_ = nullptr;

    for(size_t i=0; i<sizeof(palette_)/sizeof(palette_[0]); i++) {
        palette_[i] = (standard_palette[i] & 0xFF00FF00) |
                      ((standard_palette[i] >> 16 ) & 0xFF) |
                      ((standard_palette[i] & 0xFF) << 16);
    }
    const auto& config = ConfigLoader<proto::Configuration>::GetConfig();
    for(const auto& b : config.controls().buttons()) {
        buttons_[b.scancode()] = b.button();
    }

}

void NES::Shutdown() {
    cpu_->SaveRwLog();
}

void NES::LoadFile(const std::string& filename) {
    if (!FLAGS_fm2.empty()) {
        movie_->Load(FLAGS_fm2);
        has_movie_ = true;
    }
    cart_->LoadFile(filename);
    mapper_ = MapperRegistry::New(this, cart_->mapper());
}

bool NES::LoadStateFromFile(const std::string& filename) {
    FILE* fp;
    std::string data;
    if ((fp = fopen(filename.c_str(), "rb")) != nullptr) {
        fseek(fp, 0, SEEK_END);
        data.resize(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        fread(&data.front(), 1, data.size(), fp);
        fclose(fp);
    } else {
        //console_.AddLog("[error] Could not LoadState from %s",
        //                filename.c_str());
        return false;
    }
    return LoadState(data);
}

bool NES::LoadState(const std::string& data) {
    state_.Clear();
    if (!state_.ParseFromString(data)) {
        if (!google::protobuf::TextFormat::ParseFromString(data, &state_)) {
            //console_.AddLog("[error] Could not parse data from %s",
            //                filename.c_str());
            return false;
        }
    }

    apu_->LoadState(state_.mutable_apu());
    cpu_->LoadState(state_.mutable_cpu());
    mem_->LoadState(&state_);
    ppu_->LoadState(state_.mutable_ppu());
    mapper_->LoadState(state_.mutable_mapper());
    cart_->LoadState(state_.mutable_mapper());
    return true;
}

bool NES::LoadEverdriveStateFromFile(const std::string& filename) {
    FILE* fp;
    uint8_t data[32*1024];
    size_t len = 0;
    if ((fp = fopen(filename.c_str(), "rb")) != nullptr) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (len <= sizeof(data)) {
            fread(&data, 1, len, fp);
        }
        fclose(fp);
    } else {
        //console_.AddLog("[error] Could not LoadState from %s",
        //                filename.c_str());
        return false;
    }
    if (len > sizeof(data)) {
        return false;
    }
    mem_->LoadEverdriveState(data);
    cpu_->LoadEverdriveState(data);
    mapper_->LoadEverdriveState(data);
    return true;
}

std::string NES::SaveState(bool text) {
    apu_->SaveState(state_.mutable_apu());
    cpu_->SaveState(state_.mutable_cpu());
    mem_->SaveState(&state_);
    ppu_->SaveState(state_.mutable_ppu());
    mapper_->SaveState(state_.mutable_mapper());
    cart_->SaveState(state_.mutable_mapper());

    std::string data;
    if (text) {
        google::protobuf::TextFormat::PrintToString(state_, &data);
    } else {
        state_.SerializeToString(&data);
    }
    return data;
}

bool NES::SaveStateToFile(const std::string& filename, bool text) {
    std::string data = SaveState(text);
    FILE* fp;
    if ((fp = fopen(filename.c_str(), "wb")) != nullptr) {
        fwrite(data.data(), 1, data.size(), fp);
        fclose(fp);
    } else {
        //console_.AddLog("[error] Could not SaveState to %s", filename.c_str());
        return false;
    }
    return true;
}


void NES::HandleKeyboard(SDL_Event *event) {
    switch(event->type) {
    case SDL_KEYDOWN: {
        ControllerButtons b = buttons_[event->key.keysym.scancode];
        switch (b) {
        case ControllerButtons::ControllerPause:
            pause_ = !pause_;
            break;
        case ControllerButtons::ControllerFrameStep:
            pause_ = true;
            step_ = true;
            break;
        case ControllerButtons::ControllerReset:
            Reset();
            break;
        case ControllerButtons::Controller2UpA:
            controller_[1]->set_buttons(Controller::BUTTON_UP |
                                        Controller::BUTTON_A);
            break;
        default:
            controller_[0]->set_buttons(event);
        }
        }
        break;
    case SDL_KEYUP: {
        ControllerButtons b = buttons_[event->key.keysym.scancode];
        switch (b) {
        case ControllerButtons::Controller2UpA:
            controller_[1]->set_buttons(0);
            break;
        default:
            controller_[0]->set_buttons(event);
        }
        }
        break;
    default:
        ;
    }
}

uint64_t NES::cpu_cycles() {
    return cpu_->cycles();
}

void NES::Reset() {
    cpu_->reset();
    ppu_->Reset();
}

bool NES::Emulate() {
    // TODO(cfrantz): RegisterValue(4) is the PRG bank mapping for MMC1.
    // This needs be abstracted into a more general solution.
    int addr = cpu_->pc() | (mapper_->RegisterValue(4) << 16);
    const int n = cpu_->Execute();
    frame_profile_[addr] += n;
    for(int i=0; i<n*3; i++) {
        //if (cpu_->irq_pending()) { ppu_->set_debug_dot(0xFF00FF00); }
        // The PPU is clocked at 3 dots per CPU clock
        ppu_->Emulate();
        mapper_->Emulate();
        cart_->Emulate();
    }
    for(int i=0; i<n; i++) {
        apu_->Emulate();
    }
    return true;
}

bool NES::EmulateFrame() {
    midi_->Emulate();
    if (pause_) {
        if (!step_) return true;
        step_ = false;
    }
    double count = double(frequency) / FLAGS_fps - remainder_;
    double eof = cpu_->cycles() + count;
    frame_profile_.clear();

    movie_->Emulate();
    // Assume there will be lag during this frame.  If the game reads the
    // controllers on time, the controller emulation will clear the lag flag.
    lag_ = true;
    while(double(cpu_->cycles()) < eof) {
        if (!Emulate())
            return false;
    }
    frame_++;
    remainder_ = double(cpu_->cycles()) - eof;
    return true;
}

void NES::IRQ() {
    cpu_->irq();
}

void NES::NMI() {
    cpu_->nmi();
}

void NES::Stall(int s) {
    cpu_->Stall(s);
}

}  // namespace protones
