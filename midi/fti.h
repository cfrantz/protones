#ifndef PROTONES_MIDI_FTI_H
#define PROTONES_MIDI_FTI_H

#include "proto/fti.pb.h"
#include "util/status.h"
#include "util/statusor.h"

namespace protones {

StatusOr<proto::FTInstrument> LoadFTI(const std::string& filename);
util::Status SaveFTI(const std::string& filename, proto::FTInstrument& inst);

}  // namespace protones
#endif // PROTONES_MIDI_FTI_H
