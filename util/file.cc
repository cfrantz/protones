#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#include <libgen.h>
#include <limits.h>

#include "util/file.h"

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "util/posix_status.h"

absl::StatusOr<Stat> Stat::Filename(const std::string& filename) {
    Stat s;
    if (stat(filename.c_str(), &s.stat_) == -1) {
        return util::PosixStatus(errno);
    }
    return s;
}

absl::StatusOr<Stat> Stat::FileDescriptor(int fd) {
    Stat s;
    if (fstat(fd, &s.stat_) == -1) {
        return util::PosixStatus(errno);
    }
    return s;
}

absl::StatusOr<Stat> Stat::Link(const std::string& filename) {
    Stat s;
    if (stat(filename.c_str(), &s.stat_) == -1) {
        return util::PosixStatus(errno);
    }
    return s;
}

std::unique_ptr<File> File::Open(const std::string& filename,
                                 const std::string& mode) {
    FILE* fp = fopen(filename.c_str(), mode.c_str());
    if (fp == nullptr)
        return nullptr;
    return std::unique_ptr<File>(new File(fp));
}

bool File::GetContents(const std::string& filename, std::string* contents) {
    std::unique_ptr<File> f(Open(filename, "rb"));
    if (f == nullptr)
        return false;
    return f->Read(contents);
}

bool File::SetContents(const std::string& filename, const std::string& contents) {
    std::unique_ptr<File> f(Open(filename, "wb"));
    if (f == nullptr)
        return false;
    return f->Write(contents);
}

std::string File::Basename(const std::string& path) {
    char buf[PATH_MAX];
    memcpy(buf, path.data(), path.size());
    buf[path.size()] = '\0';
    return std::string(basename(buf));
}

std::string File::Dirname(const std::string& path) {
    char buf[PATH_MAX];
    memcpy(buf, path.data(), path.size());
    buf[path.size()] = '\0';
    return std::string(dirname(buf));
}

absl::Status File::Access(const std::string& path) {
    if (access(path.c_str(), F_OK) == -1) {
        return util::PosixStatus(errno);
    }
    return absl::OkStatus();
}

absl::Status File::MakeDir(const std::string& path, mode_t mode) {
    int rc;
#ifndef _WIN32
    rc = mkdir(path.c_str(), mode);
#else
    rc = mkdir(path.c_str());
#endif
    if (rc == -1) {
        return util::PosixStatus(errno);
    }
    return absl::OkStatus();
}

absl::Status File::MakeDirs(const std::string& path, mode_t mode) {
    std::vector<std::string> p = absl::StrSplit(path, absl::ByAnyChar("\\/"));
    if (p[0] == "") {
        p[0] = "/";
    }

    std::string mpath;
    for(size_t i=0; i<p.size(); ++i) {
        if (i == 0) {
            mpath = p.at(i);
        } else {
            mpath = absl::StrCat(mpath,
                                 mpath.back() == '/' ? "" : "/",
                                 p.at(i));
        }
        absl::Status status = Access(mpath);
        if (status.ok()) {
            continue;
        }
        status = MakeDir(mpath, mode);
        if (!status.ok()) {
            return status;
        }
    }
    return absl::OkStatus();
}

File::File(FILE* fp) : fp_(fp) {}

File::~File() {
    Close();
}

Stat File::FStat() {
    return Stat::FileDescriptor(fileno(fp_)).value();
}

int64_t File::Length() {
//	long pos = ftell(fp_);
//	fseek(fp_, 0, SEEK_END);
//	long end = ftell(fp_);
//	fseek(fp_, pos, SEEK_SET);
//	return end;
    return FStat().Size();
}

bool File::Read(void* buf, int64_t *len) {
    *len = fread(buf, 1, *len, fp_);
    return !ferror(fp_);
}

bool File::Read(std::string* buf, int64_t len) {
    buf->resize(len);
    return Read(&buf->front(), &len);
}

bool File::Read(std::string* buf) {
    return Read(buf, Length());
}

bool File::Write(const void* buf, int64_t len) {
    fwrite(buf, 1, len, fp_);
    return !ferror(fp_);
}

bool File::Write(const std::string& buf, int64_t len) {
    return Write(buf.data(), len);
}

bool File::Write(const std::string& buf) {
    return Write(buf.data(), buf.size());
}

void File::Close() {
    if (fp_) {
        fclose(fp_);
        fp_ = nullptr;
    }
}
