#pragma once

namespace drp
{

class SubprocessExecutor
{
public:
    /// @throw qwr::qwrException
    SubprocessExecutor( const qwr::u8string& command );

public:
    /// @throw qwr::qwrException
    void Start();

    /// @throw qwr::qwrException
    void WriteData( const qwr::u8string& data );

    /// @throw qwr::qwrException
    /// @throw exception_aborted
    DWORD WaitUntilCompleted( const std::chrono::seconds& timeout );

    /// @throw qwr::qwrException
    std::optional<qwr::u8string> GetOutput();

    /// @throw qwr::qwrException
    std::optional<qwr::u8string> GetErrorOutput();

private:
    /// @throw qwr::qwrException
    void CreatePipes();

    /// @throw qwr::qwrException
    /// @throw exception_aborted
    void CreateProcess( const qwr::u8string& command );

    /// @throw qwr::qwrException
    void CreateJob();

    /// @throw qwr::qwrException
    static std::optional<qwr::u8string> ReadDataFromPipe( HANDLE hPipe );

private:
    using UniqueHandlePtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype( &CloseHandle )>;

    struct JobHandles
    {
        UniqueHandlePtr hStdinRead{ nullptr, CloseHandle };
        UniqueHandlePtr hStdinWrite{ nullptr, CloseHandle };
        UniqueHandlePtr hStdoutRead{ nullptr, CloseHandle };
        UniqueHandlePtr hStdoutWrite{ nullptr, CloseHandle };
        UniqueHandlePtr hStderrRead{ nullptr, CloseHandle };
        UniqueHandlePtr hStderrWrite{ nullptr, CloseHandle };
        UniqueHandlePtr hProcess{ nullptr, CloseHandle };
        UniqueHandlePtr hThread{ nullptr, CloseHandle };
        UniqueHandlePtr hJob{ nullptr, CloseHandle };
    };

    JobHandles handles_;
};

} // namespace drp
