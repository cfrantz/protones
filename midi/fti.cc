#include <stdint.h>
#include "midi/fti.h"

#include "absl/strings/match.h"
#include "google/protobuf/text_format.h"
#include "util/file.h"

namespace protones {
namespace {
util::Status Error(std::string err) {
        return util::Status(util::error::Code::INVALID_ARGUMENT, err);
}
}

util::Status ParseEnvelope(File* file, proto::Envelope* env) {
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

    return util::Status();
}

util::Status ParseBasicFTI(File* file, proto::FTInstrument* inst) {
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
    return util::Status();
}

StatusOr<proto::FTInstrument> ParseFTI(File* file) {
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

    util::Status status;
    switch(inst.kind()) {
        case proto::FTInstrument_Kind_NES2A03:
        case proto::FTInstrument_Kind_VRC6:
            status = ParseBasicFTI(file, &inst);
            if (!status.ok()) return status;
            break;
        default:
            return Error("Expansion not supported");
    }

    return inst;
}

util::Status SaveEnvelope(File* file, const proto::Envelope* env) {
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
    return util::Status();
}

util::Status SaveBasicFTI(File* file, const proto::FTInstrument& inst) {
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
    return util::Status();
}

util::Status SaveFTInstrument(File* file, proto::FTInstrument& inst) {
    file->Write("FTI2.4", 6);
    file->Write(static_cast<uint8_t>(inst.kind()));
    file->Write(static_cast<uint32_t>(inst.name().size()));
    file->Write(inst.name().data(), inst.name().size());

    util::Status status;
    switch(inst.kind()) {
        case proto::FTInstrument_Kind_NES2A03:
        case proto::FTInstrument_Kind_VRC6:
            status = SaveBasicFTI(file, inst);
            if (!status.ok()) return status;
            break;
        default:
            return Error("Expansion not supported");
    }
    return util::Status();
}

StatusOr<proto::FTInstrument> LoadFTI(const std::string& filename) {
    if (absl::EndsWith(filename, ".textpb")
        || absl::EndsWith(filename, ".textproto")
        || absl::EndsWith(filename, ".txt")) {
        std::string buffer;
        if (!File::GetContents(filename, &buffer)) {
            return Error("Unable to read FTI file");
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

util::Status SaveFTI(const std::string& filename, proto::FTInstrument& inst) {
    if (absl::EndsWith(filename, ".textpb")
        || absl::EndsWith(filename, ".textproto")
        || absl::EndsWith(filename, ".txt")) {
        std::string buffer;
        google::protobuf::TextFormat::PrintToString(inst, &buffer);
        if (!File::SetContents(filename, buffer)) {
            return Error("Unable to write FTI file");
        }
        return util::Status();
    } else {
        auto f = File::Open(filename, "wb");
        if (!f) {
            return Error("Unable to write FTI file");
        }
        return SaveFTInstrument(f.get(), inst);
    }
}

}  // namespace protones
