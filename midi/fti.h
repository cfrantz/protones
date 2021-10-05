#ifndef PROTONES_MIDI_FTI_H
#define PROTONES_MIDI_FTI_H

#include "proto/fti.pb.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace protones {

absl::StatusOr<proto::FTInstrument> LoadFTI(const std::string& filename);
absl::Status SaveFTI(const std::string& filename, proto::FTInstrument& inst);

}  // namespace protones
#endif // PROTONES_MIDI_FTI_H
