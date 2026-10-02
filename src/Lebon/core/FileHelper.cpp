#include "FileHelper.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <system_error>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace FileHelper
{
    namespace
    {
        constexpr int kReplaceRetries = 4;

        fs::path MakeTempPath(fs::path const& _target)
        {
            static std::atomic<std::uint64_t> sCounter{ 0 };

            fs::path dir = _target.parent_path();
            if (dir.empty())
                dir = fs::path(".");

            std::random_device rd;
            std::uint64_t const pid = static_cast<std::uint64_t>(::GetCurrentProcessId());
            std::uint64_t const seq = sCounter.fetch_add(1, std::memory_order_relaxed);
            std::uint64_t const rnd = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();

            std::ostringstream name;
            name << _target.filename().string() << ".solgen-tmp-"
                 << std::hex << pid << '-' << seq << '-' << rnd;

            return dir / name.str();
        }
        
        bool IsTransient(std::error_code const& _ec)
        {
            long const v = _ec.value();
            return v == ERROR_SHARING_VIOLATION
                || v == ERROR_ACCESS_DENIED
                || v == ERROR_LOCK_VIOLATION
                || _ec == std::errc::permission_denied;
        }

        Error IoWith(std::string _message, fs::path _path, std::error_code const& _ec)
        {
            Error e = Error::Io(std::move(_message), std::move(_path));
            e.details.push_back(_ec.message());
            return e;
        }
    }

    bool Exists(fs::path const& _path)
    {
        std::error_code ec;
        return fs::exists(_path, ec);
    }

    bool IsFile(fs::path const& _path)
    {
        std::error_code ec;
        return fs::is_regular_file(_path, ec);
    }

    bool IsDir(fs::path const& _path)
    {
        std::error_code ec;
        return fs::is_directory(_path, ec);
    }

    Error CreateDir(fs::path const& _dir)
    {
        if (IsDir(_dir))
            return Error::Ok();

        std::error_code ec;
        fs::create_directories(_dir, ec);
        if (ec)
            return IoWith("failed to create directory", _dir, ec);

        return Error::Ok();
    }

    Error DeleteDir(fs::path const& _dir)
    {
        if (IsDir(_dir) == false)
            return Error::Ok();

        std::error_code ec;
        fs::remove_all(_dir, ec);
        if (ec)
            return IoWith("failed to delete directory", _dir, ec);

        return Error::Ok();
    }

    Error ListDir(fs::path const& _dir, std::vector<fs::path>& _out)
    {
        _out.clear();

        if (IsDir(_dir) == false)
            return Error::Io("directory does not exist", _dir);

        std::error_code ec;
        fs::recursive_directory_iterator it(
            _dir, fs::directory_options::skip_permission_denied, ec);
        if (ec)
            return IoWith("cannot open directory", _dir, ec);

        fs::recursive_directory_iterator const end;
        while (it != end)
        {
            _out.push_back(it->path());
            it.increment(ec);
            if (ec)
            {
                _out.clear();
                return IoWith("error while scanning directory", _dir, ec);
            }
        }

        std::sort(_out.begin(), _out.end());
        return Error::Ok();
    }

    Error ReadFile(fs::path const& _file, std::string& _out)
    {
        if (Exists(_file) == false)
            return Error::Io("file does not exist", _file);

        std::ifstream in(_file, std::ios::binary);
        if (!in)
            return Error::Io("cannot open file for reading", _file);

        std::ostringstream ss;
        ss << in.rdbuf();
        if (in.bad())
            return Error::Io("error while reading file", _file);

        _out = ss.str();
        return Error::Ok();
    }

    Error WriteFileAtomic(fs::path const& _file, std::string_view _content)
    {
        fs::path dir = _file.parent_path();
        if (dir.empty())
            dir = fs::path(".");

        if (Error e = CreateDir(dir))
            return e;

        fs::path const temp = MakeTempPath(_file);

        {
            std::ofstream out(temp, std::ios::binary | std::ios::trunc);
            if (!out)
                return Error::Io("cannot create temp file", temp);

            out.write(_content.data(), static_cast<std::streamsize>(_content.size()));
            out.flush();

            if (!out)
            {
                out.close();
                std::error_code rmec;
                fs::remove(temp, rmec);
                return Error::Io("failed writing temp file", temp);
            }
        }

        std::error_code ec;
        for (int attempt = 0; ; ++attempt)
        {
            fs::rename(temp, _file, ec);
            if (!ec)
                return Error::Ok();

            if (attempt >= kReplaceRetries || IsTransient(ec) == false)
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(20 * (1 << attempt)));
        }

        std::error_code rmec;
        fs::remove(temp, rmec);
        return IoWith("failed to replace file", _file, ec);
    }

    Error WriteFileIfChanged(fs::path const& _file, std::string_view _content, WriteOutcome& _out)
    {
        if (Exists(_file))
        {
            std::string current;
            if (Error e = ReadFile(_file, current))
                return e;

            if (current == _content)
            {
                _out = WriteOutcome::Unchanged;
                return Error::Ok();
            }
        }

        if (Error e = WriteFileAtomic(_file, _content))
            return e;

        _out = WriteOutcome::Written;
        return Error::Ok();
    }

    Error WriteFileIfAbsent(fs::path const& _file, std::string_view _content, WriteOutcome& _out)
    {
        if (Exists(_file))
        {
            _out = WriteOutcome::Unchanged;
            return Error::Ok();
        }

        if (Error e = WriteFileAtomic(_file, _content))
            return e;

        _out = WriteOutcome::Written;
        return Error::Ok();
    }

    Error TouchFile(fs::path const& _file, WriteOutcome& _out)
    {
        return WriteFileIfAbsent(_file, std::string_view{}, _out);
    }

    Error RemoveFile(fs::path const& _file)
    {
        std::error_code ec;
        fs::remove(_file, ec);
        if (ec)
            return IoWith("failed to delete file", _file, ec);

        return Error::Ok();
    }

    Error CopyFileTo(fs::path const& _src, fs::path const& _dst, bool _overwrite)
    {
        if (Error e = CreateDir(_dst.parent_path()))
            return e;

        fs::copy_options const opts = _overwrite
            ? fs::copy_options::overwrite_existing
            : fs::copy_options::skip_existing;

        std::error_code ec;
        fs::copy_file(_src, _dst, opts, ec);
        if (ec)
            return IoWith("failed to copy file", _src, ec);

        return Error::Ok();
    }

    fs::path NormalizePath(fs::path const& _path)
    {
        std::error_code ec;
        fs::path result = fs::weakly_canonical(_path, ec);
        if (ec || result.empty())
            result = _path.lexically_normal();

        return result;
    }

    fs::path RelativeTo(fs::path const& _path, fs::path const& _base)
    {
        return _path.lexically_normal().lexically_relative(_base.lexically_normal());
    }

    std::string ToBackSlashes(fs::path const& _path)
    {
        std::string str = _path.string();
        std::replace(str.begin(), str.end(), '/', '\\');
        return str;
    }

    std::string ToForwardSlashes(fs::path const& _path)
    {
        std::string str = _path.string();
        std::replace(str.begin(), str.end(), '\\', '/');
        return str;
    }

    std::string ExtLower(fs::path const& _path)
    {
        std::string ext = _path.extension().string();
        for (char& c : ext)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        return ext;
    }

    Error ExecutablePath(fs::path& _out)
    {
        std::vector<wchar_t> buffer(MAX_PATH);

        while (buffer.size() <= (1u << 16))
        {
            ::SetLastError(0);
            DWORD const len = ::GetModuleFileNameW(
                nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

            if (len == 0)
            {
                Error e = Error::Io("cannot determine executable path");
                e.details.push_back(
                    std::system_category().message(static_cast<int>(::GetLastError())));
                return e;
            }

            if (len < buffer.size())
            {
                _out = fs::path(std::wstring(buffer.data(), len));
                return Error::Ok();
            }

            buffer.resize(buffer.size() * 2);
        }

        return Error::Io("executable path is unreasonably long");
    }

    Error FindUpwards(fs::path const& _start, fs::path const& _marker, fs::path& _out)
    {
        std::error_code ec;
        fs::path current = fs::absolute(_start, ec);
        if (ec)
        {
            Error e = Error::Io("unvalid start path");
            e.details.push_back(ec.message());
            return e;
        }
        current = current.lexically_normal();

        while (true)
        {
            if (Exists(current / _marker))
            {
                _out = current;
                return Error::Ok();
            }

            fs::path parent = current.parent_path();
            if (parent == current)
                return Error::Io("unable to find marker '" + _marker.string() + "' from '" + _start.string() + "'");

            current = std::move(parent);
        }
    }
}
