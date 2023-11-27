#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "util/file.h"
#include "util/sound/file.h"

ABSL_FLAG(std::string, output, "", "Output filename");
ABSL_FLAG(double, rate, 33143.9, "DPCM sampling rate");
ABSL_FLAG(int, crush, 0, "Crush to the number of bits");

const char kUsage[] = R"usage(
DPCM sample converter

dpcm_convert [resample | wav_to_dpcm | dpcm_to_wav] --output=FILE INFILE
)usage";

int main(int argc, char* argv[]) {
    absl::SetProgramUsageMessage(kUsage);
    auto args = absl::ParseCommandLine(argc, argv);

    if (args.size() != 3) {
        LOG(FATAL) << "No command or input file";
    }
    std::string command = args[1];

    if (command == "resample" || command == "wav_to_dpcm") {
        auto file = sound::File::Load(args[2]);
        if (!file.ok()) {
            LOG(FATAL) << "Error loading file: " << file.status();
        }
        auto sound = std::move(*file);
        sound->ConvertToMono();
        sound->Resample(absl::GetFlag(FLAGS_rate));
        if (absl::GetFlag(FLAGS_crush)) {
            sound->BitCrush(absl::GetFlag(FLAGS_crush));
        }
        absl::Status sts;
        if (command == "resample") {
            sts = sound->Save(absl::GetFlag(FLAGS_output));
        } else {
            auto dpcm = sound->DpcmEncode();
            auto file = File::Open(absl::GetFlag(FLAGS_output), "wb");
            sts = file->Write(dpcm.data(), dpcm.size());
            file->Close();
        }
        if (!sts.ok()) {
            LOG(ERROR) << "Saving file: " << sts;
        }
    } else if (command == "dpcm_to_wav") {
        auto file = File::GetContents(args[2]);
        if (!file.ok()) {
            LOG(FATAL) << "Error loading file: " << file.status();
        }
        auto dpcm = *file;
        auto wav = sound::File::DpcmDecode(
            reinterpret_cast<const uint8_t*>(dpcm.data()), dpcm.size(),
            absl::GetFlag(FLAGS_rate));
        auto sts = wav->Save(absl::GetFlag(FLAGS_output));
        if (!sts.ok()) {
            LOG(ERROR) << "Saving file: " << sts;
        }
    } else {
        LOG(ERROR) << "Unknown command: " << command;
    }
    return 0;
}
