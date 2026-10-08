#include "Sink.h"

namespace Runtime
{
    void ConsoleSink::Write(std::string_view _text)
    {
        std::fwrite(_text.data(), 1, _text.size(), m_stream);
    }

    void ConsoleSink::Flush()
    {
        std::fflush(m_stream);
    }

    bool FileSink::Open(std::filesystem::path const& _path)
    {
        Close();
        // Binary mode, otherwise Windows turns every '\n' into "\r\n"
        m_file.open(_path, std::ios::binary | std::ios::trunc);
        return m_file.is_open();
    }

    void FileSink::Close()
    {
        if (m_file.is_open())
            m_file.close();
    }

    void FileSink::Write(std::string_view _text)
    {
        if (m_file.is_open())
            m_file.write(_text.data(), static_cast<std::streamsize>(_text.size()));
    }

    void FileSink::Flush()
    {
        if (m_file.is_open())
            m_file.flush();
    }
}
