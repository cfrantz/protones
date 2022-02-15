#include <stdint.h>
#include "midi/fti.h"

#include "absl/strings/match.h"
#include "google/protobuf/text_format.h"
#include "util/file.h"

namespace protones {
namespace {
absl::Status Error(std::string err) {
    return absl::InvalidArgumentError(err);
}
}

absl::Status ParseEnvelope(File* file, proto::Envelope* env) {
    uint32_t count;
    if (!file->Read(&count)) {
        return Error("Couldn't read Envelope::count field");
    }

    int32_t loop;
    if (!file->Read(&loop)) {
        return Error("Couldn't read Envelope::loop field");
    }

    int32_t release;
    if (!file->Read(&release)) {
        return Error("Couldn't read Envelope::release field");
    }

    uint32_t setting;
    if (!file->Read(&setting)) {
        return Error("Couldn't read Envelope::setting field");
    }

    int8_t sequence[count];
    int64_t len = count;
    if (!file->Read(sequence, &len)) {
        return Error("Couldn't read Envelope::sequence field");
    }
    for(const auto& s : sequence) {
        env->add_sequence(s);
    }

    // If the loop or release points are beyond the length of the
    // sequence, clamp them.
    if (loop >= static_cast<int32_t>(count)) {
        loop = count - 1;
    }
    if (release >= static_cast<int32_t>(count)) {
        release = count - 1;
    }

    // Fixups for loop/release points (used in FamiStudio).
    if (release != -1) {
        if (loop == -1) {
            loop = release;
        }
        release += 1;
    }
    env->set_loop(loop);
    env->set_release(release);

    return absl::OkStatus();
}

absl::Status ParseBasicFTI(File* file, proto::FTInstrument* inst) {
    uint8_t count;
    if (!file->Read(&count)) {
        return Error("Couldn't read count field");
    }
    if (count != 5) {
        return Error("Unexpected number of envelopes");
    }

    for(size_t i=0; i<count; i++) {
        uint8_t present;
        if (!file->Read(&present)) {
            return Error("Couldn't read present field");
        }
        if (present) {
            proto::Envelope* env =
                (i==0) ? inst->mutable_volume() :
                (i==1) ? inst->mutable_arpeggio() :
                (i==2) ? inst->mutable_pitch() :
                (i==3) ? inst->mutable_hipitch() :
                (i==4) ? inst->mutable_duty() : nullptr;
            auto status = ParseEnvelope(file, env);
            if (!status.ok()) {
                return status;
            }
            env->set_kind(static_cast<proto::Envelope::Kind>(i+1));
        }
    }
    return absl::OkStatus();
}

absl::Status ParseVRC7(File* file, proto::FTInstrument* inst) {
    auto* vrc7 = inst->mutable_vrc7();
    uint32_t patch;
    if (!file->Read(&patch)) {
        return Error("Couldn't read patch field");
    }
    vrc7->set_patch(patch);

    if (patch == 0) {
        uint8_t regs[8];
        if (!file->Read(&regs)) {
            return Error("Couldn't read patch registers");
        }
        for(const auto& v : regs) {
            vrc7->add_regs(v);
        }
    }
    return absl::OkStatus();
}

struct AssignmentData {
    uint8_t index;
    uint8_t sample;
    uint8_t pitch;
    uint8_t delta;
};

absl::Status ParseDPCM(File* file, proto::FTInstrument* inst) {
    uint32_t assigned;
    if (!file->Read(&assigned)) {
        return Error("Couldn't read DPCM.assigned field");
    }
    for(uint32_t i=0; i<assigned; ++i) {
        AssignmentData ad;
        if (!file->Read(&ad)) {
            return Error("Couldn't read DPCM.assignment_data field");
        }
        auto *dpcm = inst->add_dpcm();
        dpcm->set_note(ad.index);
        dpcm->set_sample(ad.sample - 1);
        dpcm->set_pitch(ad.pitch & 0x7f);
        dpcm->set_loop((ad.pitch & 0x80) != 0);
    }
    uint32_t count;
    if (!file->Read(&count)) {
        return Error("Couldn't read count field");
    }
    for(uint32_t i=0; i<count; ++i) {
        uint32_t index, namelen, size;
        proto::DPCMSample sample;
        if (!file->Read(&index)) {
            return Error("Couldn't read DPCM.sample.index field");
        }
        if (!file->Read(&namelen)) {
            return Error("Couldn't read DPCM.sample.namelen field");
        }
        if (!file->Read(sample.mutable_name(), namelen)) {
            return Error("Couldn't read DPCM.sample.name field");
        }
        if (!file->Read(&size)) {
            return Error("Couldn't read DPCM.sample.size field");
        }
        if (!file->Read(sample.mutable_data(), size)) {
            return Error("Couldn't read DPCM.sample.data field");
        }
        sample.set_size(int32_t(size));
        (*inst->mutable_sample())[index] = sample;
    }
    return absl::OkStatus();
}


absl::StatusOr<proto::FTInstrument> ParseFTI(File* file) {
    proto::FTInstrument inst;

    std::string sig;
    if (!file->Read(&sig, 3)) {
        return Error("Unable to read FTI file");
    }
    if (sig != "FTI") {
        return Error("Bad FTI signature");
    }

    std::string version;
    if (!file->Read(&version, 3)) {
        return Error("Unable to read FTI file");
    }
    if (version != "2.4") {
        return Error("Bad FTI version");
    }

    uint8_t exp;
    if (!file->Read(&exp)) {
        return Error("Couldn't read expansion field");
    }
    inst.set_kind(static_cast<proto::FTInstrument::Kind>(exp));
    uint32_t namelen;
    if (!file->Read(&namelen)) {
        return Error("Couldn't read namelen field");
    }
    if (!file->Read(inst.mutable_name(), namelen)) {
        return Error("Couldn't read name field");
    }

    absl::Status status;
    switch(inst.kind()) {
        case proto::FTInstrument_Kind_NES2A03:
        case proto::FTInstrument_Kind_VRC6:
            status = ParseBasicFTI(file, &inst);
            if (!status.ok()) return status;
            break;
        case proto::FTInstrument_Kind_VRC7:
            status = ParseVRC7(file, &inst);
            if (!status.ok()) return status;
            break;
        default:
            return Error("Expansion not supported");
    }

    if (inst.kind() == proto::FTInstrument_Kind_NES2A03) {
        status = ParseDPCM(file, &inst);
        //if (!status.ok()) return status;
    }
    return inst;
}

absl::Status SaveEnvelope(File* file, const proto::Envelope* env) {
    // count
    file->Write(static_cast<uint32_t>(env->sequence_size()));
    file->Write(env->loop());
    // Undo the release fixup done by the loader.
    int32_t release = env->release();
    if (release != -1) {
        release -= 1;
    }
    file->Write(release);
    // Settings
    file->Write(static_cast<uint32_t>(0));
    // Sequence values
    for(const auto& val : env->sequence()) {
        file->Write(static_cast<int8_t>(val));
    }
    return absl::OkStatus();
}

absl::Status SaveBasicFTI(File* file, const proto::FTInstrument& inst) {
    file->Write(static_cast<uint8_t>(5));

    for(size_t i=0; i<5; i++) {
        const proto::Envelope* env =
            (i==0) ? &inst.volume() :
            (i==1) ? &inst.arpeggio() :
            (i==2) ? &inst.pitch() :
            (i==3) ? &inst.hipitch() :
            (i==4) ? &inst.duty() : nullptr;
        if (env == nullptr || env->kind() == proto::Envelope_Kind_UNKNOWN) {
            // Not Present
            file->Write(static_cast<uint8_t>(0));
        } else {
            // Present
            file->Write(static_cast<uint8_t>(1));
            auto status = SaveEnvelope(file, env);
            if (!status.ok()) {
                return status;
            }
        }
    }
    return absl::OkStatus();
}

absl::Status SaveVRC7(File* file, const proto::FTInstrument& inst) {
    const auto& vrc7 = inst.vrc7();
    file->Write(vrc7.patch());
    if (vrc7.patch() == 0) {
        for(const auto& reg : vrc7.regs()) {
            file->Write(static_cast<uint8_t>(reg));
        }
    }
    return absl::OkStatus();
}

absl::Status SaveFTInstrument(File* file, proto::FTInstrument& inst) {
    file->Write("FTI2.4", 6);
    file->Write(static_cast<uint8_t>(inst.kind()));
    file->Write(static_cast<uint32_t>(inst.name().size()));
    file->Write(inst.name().data(), inst.name().size());

    absl::Status status;
    switch(inst.kind()) {
        case proto::FTInstrument_Kind_NES2A03:
        case proto::FTInstrument_Kind_VRC6:
            status = SaveBasicFTI(file, inst);
            if (!status.ok()) return status;
            break;
        case proto::FTInstrument_Kind_VRC7:
            status = SaveVRC7(file, inst);
            if (!status.ok()) return status;
            break;
        default:
            return Error("Expansion not supported");
    }
    return absl::OkStatus();
}

absl::StatusOr<proto::FTInstrument> LoadFTI(const std::string& filename) {
    if (absl::EndsWith(filename, ".textpb")
        || absl::EndsWith(filename, ".textproto")
        || absl::EndsWith(filename, ".txt")) {
        std::string buffer;
        if (!File::GetContents(filename, &buffer)) {
            return Error("Unable to read FTI textpb file");
        }
        proto::FTInstrument inst;
        if (!google::protobuf::TextFormat::ParseFromString(buffer, &inst)) {
            return Error("Unable to parse FTI textproto");
        }
        return inst;
    } else {
        auto f = File::Open(filename, "rb");
        if (!f) {
            return Error("Unable to read FTI file");
        }
        return ParseFTI(f.get());
    }
}

absl::Status SaveFTI(const std::string& filename, proto::FTInstrument& inst) {
    if (absl::EndsWith(filename, ".textpb")
        || absl::EndsWith(filename, ".textproto")
        || absl::EndsWith(filename, ".txt")) {
        std::string buffer;
        google::protobuf::TextFormat::PrintToString(inst, &buffer);
        if (!File::SetContents(filename, buffer)) {
            return Error("Unable to write FTI file");
        }
        return absl::OkStatus();
    } else {
        auto f = File::Open(filename, "wb");
        if (!f) {
            return Error("Unable to write FTI file");
        }
        return SaveFTInstrument(f.get(), inst);
    }
}

}  // namespace protones
