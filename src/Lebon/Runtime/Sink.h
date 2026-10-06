#ifndef RUNTIME_SINK_H_DEFINED
#define RUNTIME_SINK_H_DEFINED

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace Runtime
{
    // Where the program output goes. Every print of every back end ends here,
    // diagnostics (errors, dumps, timings) don't.
    class OutputSink
    {
    public:
        virtual ~OutputSink() = default;

        virtual void Write(std::string_view _text) = 0;
        virtual void Flush() {}
    };

    // Writes to a C stream, stdout by default
    class ConsoleSink : public OutputSink
    {
    public:
        explicit ConsoleSink(std::FILE* _stream = stdout) : m_stream(_stream) {}

        void Write(std::string_view _text) override;
        void Flush() override;

    private:
        std::FILE* m_stream;
    };

    // Writes to a text file, created or truncated when the sink is opened.
    // Newlines are written as '\n' on every platform, so the files of two runs
    // can be compared byte for byte.
    class FileSink : public OutputSink
    {
    public:
        // Returns false if the file can't be opened, the sink then drops everything
        bool Open(std::filesystem::path const& _path);
        bool IsOpen() const { return m_file.is_open(); }
        void Close();

        void Write(std::string_view _text) override;
        void Flush() override;

    private:
        std::ofstream m_file;
    };

    // Keeps everything in memory, for tests comparing the output of the back ends
    class BufferSink : public OutputSink
    {
    public:
        void Write(std::string_view _text) override { m_buffer.append(_text); }

        std::string const& Str() const { return m_buffer; }
        void Clear() { m_buffer.clear(); }

    private:
        std::string m_buffer;
    };

    // Drops everything, for benchmarks
    class NullSink : public OutputSink
    {
    public:
        void Write(std::string_view) override {}
    };
}

#endif
