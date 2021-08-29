#include <stdint.h>
#include "midi/fti.h"

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
        case proto::FTInstrument_Kind_NONE:
        case proto::FTInstrument_Kind_VRC6:
            status = ParseBasicFTI(file, &inst);
            if (!status.ok()) return status;
            break;
        default:
            return Error("Expansion not supported");
    }

    return inst;
}

StatusOr<proto::FTInstrument> LoadFTI(const std::string& filename) {
    std::string buffer;
    auto f = File::Open(filename, "rb");
    if (!f) {
        return Error("Unable to read FTI file");
    }
    return ParseFTI(f.get());
}

}  // namespace protones
