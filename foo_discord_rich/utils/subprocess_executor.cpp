#include <stdafx.h>

#include "subprocess_executor.h"

#include <qwr/abort_callback.h>
#include <qwr/winapi_error_helpers.h>

namespace drp
{

SubprocessExecutor::SubprocessExecutor( const qwr::u8string& command )
{
    CreatePipes();
    CreateProcess( command );
    CreateJob();
}

void SubprocessExecutor::Start()
{
    qwr::QwrException::ExpectTrue( handles_.hThread.get(), "SubprocessExecutor error: null hThread" );

    auto iRet = ::ResumeThread( handles_.hThread.get() );
    qwr::error::CheckWinApi( iRet != -1, "ResumeThread" );
}

void SubprocessExecutor::WriteData( const qwr::u8string& data )
{
    qwr::QwrException::ExpectTrue( handles_.hStdinWrite.get(), "SubprocessExecutor error: null hStdinWrite" );

    DWORD written = 0;
    auto bRet = ::WriteFile( handles_.hStdinWrite.get(), data.data(), static_cast<DWORD>( data.size() ), &written, nullptr );
    qwr::error::CheckWinApi( bRet, "WriteFile" );

    handles_.hStdinWrite.reset();
}

DWORD SubprocessExecutor::WaitUntilCompleted( const std::chrono::seconds& timeout )
{
    qwr::QwrException::ExpectTrue( handles_.hProcess.get(), "SubprocessExecutor error: null hProcess" );
    qwr::QwrException::ExpectTrue( handles_.hThread.get(), "SubprocessExecutor error: null hThread" );
    qwr::QwrException::ExpectTrue( handles_.hStdinRead.get(), "SubprocessExecutor error: null hStdinRead" );
    qwr::QwrException::ExpectTrue( handles_.hStdoutWrite.get(), "SubprocessExecutor error: null hStdoutWrite" );
    qwr::QwrException::ExpectTrue( handles_.hStderrWrite.get(), "SubprocessExecutor error: null hStderrWrite" );

    qwr::TimedAbortCallback aborter{ "", timeout };

    std::array handlesToWait{ handles_.hProcess.get(), aborter.get_handle() };
    const auto waitResult = WaitForMultipleObjects( static_cast<DWORD>( handlesToWait.size() ), handlesToWait.data(), false, static_cast<DWORD>( std::chrono::milliseconds{ timeout }.count() ) );
    if ( waitResult != WAIT_OBJECT_0 )
    {
        if ( waitResult == WAIT_TIMEOUT )
        {
            throw qwr::QwrException( "SubprocessExecutor error: process timed out" );
        }

        assert( waitResult == WAIT_OBJECT_0 + 1 && aborter.is_aborting() );
        aborter.check();
    }

    DWORD exitCode = 0;
    auto bRet = ::GetExitCodeProcess( handles_.hProcess.get(), &exitCode );
    qwr::error::CheckWinApi( bRet, "GetExitCodeProcess" );

    handles_.hProcess.reset();
    handles_.hThread.reset();
    handles_.hStdinRead.reset();
    handles_.hStdoutWrite.reset();
    handles_.hStderrWrite.reset();

    return exitCode;
}

std::optional<qwr::u8string> SubprocessExecutor::GetOutput()
{
    qwr::QwrException::ExpectTrue( handles_.hStdoutRead.get(), "SubprocessExecutor error: null hStdoutRead" );

    auto hRead = std::move( handles_.hStdoutRead );
    return ReadDataFromPipe( hRead.get() );
}

std::optional<qwr::u8string> SubprocessExecutor::GetErrorOutput()
{
    qwr::QwrException::ExpectTrue( handles_.hStderrRead.get(), "SubprocessExecutor error: null hStderrRead" );

    auto hRead = std::move( handles_.hStderrRead );
    return ReadDataFromPipe( hRead.get() );
}

void SubprocessExecutor::CreatePipes()
{
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
    saAttr.bInheritHandle = true;
    saAttr.lpSecurityDescriptor = nullptr;

    {
        HANDLE hRead{};
        HANDLE hWrite{};
        auto bRet = ::CreatePipe( &hRead, &hWrite, &saAttr, 0 );
        qwr::error::CheckWinApi( bRet, "CreatePipe(stdin)" );

        handles_.hStdinRead.reset( hRead );
        handles_.hStdinWrite.reset( hWrite );

        bRet = SetHandleInformation( hWrite, HANDLE_FLAG_INHERIT, 0 );
        qwr::error::CheckWinApi( bRet, "SetHandleInformation(stdin)" );
    }

    {
        HANDLE hRead{};
        HANDLE hWrite{};
        auto bRet = ::CreatePipe( &hRead, &hWrite, &saAttr, 0 );
        qwr::error::CheckWinApi( bRet, "CreatePipe(stdout)" );

        handles_.hStdoutRead.reset( hRead );
        handles_.hStdoutWrite.reset( hWrite );

        bRet = SetHandleInformation( hRead, HANDLE_FLAG_INHERIT, 0 );
        qwr::error::CheckWinApi( bRet, "SetHandleInformation(stdout)" );
    }

    {
        HANDLE hRead{};
        HANDLE hWrite{};
        auto bRet = ::CreatePipe( &hRead, &hWrite, &saAttr, 0 );
        qwr::error::CheckWinApi( bRet, "CreatePipe(stderr)" );

        handles_.hStderrRead.reset( hRead );
        handles_.hStderrWrite.reset( hWrite );

        bRet = SetHandleInformation( hRead, HANDLE_FLAG_INHERIT, 0 );
        qwr::error::CheckWinApi( bRet, "SetHandleInformation(stderr)" );
    }
}

void SubprocessExecutor::CreateProcess( const qwr::u8string& command )
{
    auto commandW = qwr::unicode::ToWide( command );
    PROCESS_INFORMATION pi{};
    STARTUPINFO si{};
    si.cb = sizeof( si );
    si.hStdInput = handles_.hStdinRead.get();
    si.hStdOutput = handles_.hStdoutWrite.get();
    si.hStdError = handles_.hStderrWrite.get();
    si.dwFlags |= STARTF_USESTDHANDLES;
    auto bRet = ::CreateProcess( nullptr,
                                 commandW.data(),
                                 nullptr,
                                 nullptr,
                                 true,
                                 CREATE_SUSPENDED | CREATE_NO_WINDOW,
                                 nullptr,
                                 nullptr,
                                 &si,
                                 &pi );
    qwr::error::CheckWinApi( bRet, "CreateProcess" );

    handles_.hProcess.reset( pi.hProcess );
    handles_.hThread.reset( pi.hThread );
}

void SubprocessExecutor::CreateJob()
{
    const auto hJob = ::CreateJobObject( nullptr, nullptr );
    qwr::error::CheckWinApi( hJob, "CreateJobObject" );

    handles_.hJob.reset( hJob );

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    auto bRet = ::SetInformationJobObject( hJob, JobObjectExtendedLimitInformation, &jeli, sizeof( jeli ) );
    qwr::error::CheckWinApi( bRet, "SetInformationJobObject" );

    bRet = ::AssignProcessToJobObject( hJob, handles_.hProcess.get() );
    qwr::error::CheckWinApi( bRet, "AssignProcessToJobObject" );
}

std::optional<qwr::u8string> SubprocessExecutor::ReadDataFromPipe( HANDLE hPipe )
{
    std::array<char, 2048> stdoutBuf{};
    DWORD dwRead = 0;
    if ( !::PeekNamedPipe( hPipe, stdoutBuf.data(), static_cast<DWORD>( stdoutBuf.size() ), &dwRead, nullptr, nullptr ) || !dwRead )
    {
        return std::nullopt;
    }

    stdoutBuf.fill( 0 );
    dwRead = 0;
    auto bRet = ::ReadFile( hPipe, stdoutBuf.data(), static_cast<DWORD>( stdoutBuf.size() ), &dwRead, nullptr );
    qwr::error::CheckWinApi( bRet, "ReadFile" );

    qwr::u8string_view output{ stdoutBuf.data(), strlen( stdoutBuf.data() ) };
    output = output.substr( 0, output.find_last_not_of( " \t\n\r" ) + 1 );
    if ( output.empty() )
    {
        return std::nullopt;
    }

    return qwr::u8string{ output.data(), output.size() };
}

} // namespace drp
