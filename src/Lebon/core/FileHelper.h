#ifndef IO_FILEHELPER_H
#define IO_FILEHELPER_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Error.h"

namespace fs = std::filesystem;

namespace FileHelper
{
    // --- Queries (never throw, never fail loudly) ---
    bool Exists(fs::path const& _path);
    bool IsFile(fs::path const& _path);
    bool IsDir(fs::path const& _path);

    // --- Directories ---
    Error CreateDir(fs::path const& _dir);
    Error DeleteDir(fs::path const& _dir);
    Error ListDir(fs::path const& _dir, std::vector<fs::path>& _out);

    // --- Files ---
    enum class WriteOutcome { Written, Unchanged };

    Error ReadFile(fs::path const& _file, std::string& _out);
    Error WriteFileAtomic(fs::path const& _file, std::string_view _content);
    Error WriteFileIfChanged(fs::path const& _file, std::string_view _content, WriteOutcome& _out);
    Error WriteFileIfAbsent(fs::path const& _file, std::string_view _content, WriteOutcome& _out);
    Error TouchFile(fs::path const& _file, WriteOutcome& _out);
    Error RemoveFile(fs::path const& _file);
    Error CopyFileTo(fs::path const& _src, fs::path const& _dst, bool _overwrite);

    // --- Path helpers ---
    fs::path NormalizePath(fs::path const& _path);
    fs::path RelativeTo(fs::path const& _path, fs::path const& _base);
    std::string ToBackSlashes(fs::path const& _path);
    std::string ToForwardSlashes(fs::path const& _path);
    std::string ExtLower(fs::path const& _path);
    Error ExecutablePath(fs::path& _out);
    Error FindUpwards(fs::path const& _start, fs::path const& _marker, fs::path& _out);
}

#endif
