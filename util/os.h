#ifndef Z2HD_UTIL_OS_H
#define Z2HD_UTIL_OS_H
#include <vector>
#include <string>

namespace os {

std::string GetCWD();
void SchedulerYield();

int64_t utime_now();
std::string CTime(int64_t time_us);

std::string TempFilename(const std::string& filename);
std::string ExePath();
std::string ResourceDir(const std::string& name);
inline std::string ResourceDir() { return ResourceDir(""); }
int System(const std::string& cmd, bool background=false);

namespace path {
std::string Join(const std::vector<std::string>& components);
std::vector<std::string> Split(const std::string& path);
}
}

#endif // Z2HD_UTIL_OS_H
