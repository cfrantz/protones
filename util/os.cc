#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sched.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "util/os.h"
#include "util/logging.h"

namespace os {
std::string GetCWD() {
    char path[PATH_MAX];
    std::string result;

    if (getcwd(path, PATH_MAX)) {
        result = std::string(path);
    }
    return result;
}

void SchedulerYield() {
#ifndef _WIN32
    sched_yield();
#else
    SwitchToThread();
#endif
}

int64_t utime_now() {
    int64_t now;
#ifndef _WIN32
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    now = tm.tv_sec * 1000000LL + tm.tv_nsec/1000;
#else
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    // FileTime is in 100ns increments.  Convert to microseconds.
    now = (int64_t(ft.dwLowDateTime) | int64_t(ft.dwHighDateTime) << 32) / 10;
    // Windows time is since Jan 1, 1601
    // Unix time is since Jan 1, 1970
#define SEC_TO_UNIX_EPOCH 11644473600LL
    now -= SEC_TO_UNIX_EPOCH * 1000000LL;
#endif
    return now;
}

std::string CTime(int64_t time_us) {
    time_t t = time_us / 1000000;
    char buf[32];
#ifndef _WIN32
    ctime_r(&t, buf);
#else
    ctime_s(buf, sizeof(buf), &t);
#endif
    if (buf[24] == '\n') buf[24] = 0;
    return buf;
}

std::string TempFilename(const std::string& filename) {
    const char *temp = getenv("TEMP");
#ifdef _WIN32
    std::string tmp = temp ? temp : "c:/temp";
#else
    std::string tmp = temp ? temp : "/tmp";
#endif
    return path::Join({tmp, filename});
}

int System(const std::string& cmd, bool background) {
    std::string header, trailer;
#ifdef _WIN32
    header = "cmd.exe /c start ";
#else
    trailer = " &";
#endif
    return system(absl::StrCat(header, cmd, trailer).c_str());
}

std::string ExePath() {
    char exepath[PATH_MAX] = {0, };
#ifdef _WIN32
    GetModuleFileName(nullptr, exepath, PATH_MAX);
#else
    char self[PATH_MAX];
#ifdef __linux__
    snprintf(self, sizeof(self), "/proc/self/exe");
#elif defined(__FreeBSD__)
    snprintf(self, sizeof(self), "/proc/%d/file", getpid());
#else
#error "Don't know how to get ExePath"
#endif
    if (!realpath(self, exepath)) {
        perror("realpath");
    }
#endif
    return std::string(exepath);
}

std::string ResourceDir(const std::string& name) {
    std::string exe = ExePath();
    if (exe.find("_bazel_") != std::string::npos) {
        LOG(INFO, "ExePath contains '_bazel_'. Assuming ResourceDir is CWD.");
        return GetCWD();
    }

    std::vector<std::string> path = path::Split(exe);
    if (exe.at(0) == '/') {
        path.insert(path.begin(), "/");
    }
    std::string executable = path.back();
    path.pop_back(); // Remove exe filename
#ifdef _WIN32
    LOG(INFO, "Windows: assuming ResourceDir is", path::Join(path));
#else
    if (path.back() == "bin") {
        path.pop_back();
        path.push_back("share");
        path.push_back(name.empty() ? executable : name);
    } else {
        LOG(INFO, "Unix: Did not find 'bin' in ExePath. Assuming ResourcDir"
                  " is ", path::Join(path));
    }
#endif
    return path::Join(path);
}

namespace path {
char kPathSep = '/';

std::string Join(const std::vector<std::string>& components) {
    std::string result;

    for(const auto& p : components) {
        if (!result.empty() && result.back() != kPathSep) {
            result.push_back(kPathSep);
        }
        if (p.front() == kPathSep) {
            result.clear();
        }
        if (p.back() == kPathSep) {
            size_t len = p.length() - 1;
            //  In case we're adding a single "/" component.
            if (len == 0) len = 1;
            result.append(p, 0, len);
        } else {
            result.append(p);
        }
    }
    return result;
}

std::vector<std::string> Split(const std::string& path) {
    return absl::StrSplit(path, absl::ByAnyChar("\\/"));
}

} // namespace path
} // namespace os
