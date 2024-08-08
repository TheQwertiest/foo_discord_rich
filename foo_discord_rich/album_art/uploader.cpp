#include <stdafx.h>

#include "uploader.h"

#include <component_paths.h>

#include <cpr/cpr.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

std::chrono::seconds kMaxWaitTime{ 10 };

}

namespace
{

struct ArtData
{
    album_art_data_ptr pArtData;
    std::optional<qwr::u8string> pathOpt;
};

} // namespace

namespace
{

const qwr::u8string& GetTempImageFilePathTemplate()
{
    static const qwr::u8string tmpImagePath{ ( drp::path::ImageDir() / "tmp_image" ).u8string() };
    return tmpImagePath;
}

std::optional<ArtData> GetArtData( const metadb_handle_ptr& handle, abort_callback& aborter )
{
    const auto pArtMgr = album_art_manager_v3::get();

    const auto handles = pfc::list_single_ref_t<metadb_handle_ptr>( handle );
    const auto artTypeGuid = album_art_ids::cover_front;
    const auto guids = pfc::list_single_ref_t<GUID>( artTypeGuid );

    try
    {
        const auto pArtExtractor = pArtMgr->open_v3( handles, guids, nullptr, aborter );
        if ( !pArtExtractor.is_valid() )
        {
            return std::nullopt;
        }

        const auto pArt = pArtExtractor->query( artTypeGuid, aborter );
        const auto pathOpt = [&]() -> std::optional<qwr::u8string> {
            const auto pPathList = pArtExtractor->query_paths( artTypeGuid, aborter );
            if ( !pPathList->get_count() )
            {
                return std::nullopt;
            }
            return pPathList->get_path( 0 );
        }();

        return ArtData{ pArt, pathOpt };
    }
    catch ( const exception_album_art_not_found& /*e*/ )
    {
        return std::nullopt;
    }
}

/// @throw qwr::qwrException
/// @throw exception_aborted
qwr::u8string SaveArtToFile( const album_art_data_ptr& pArtData, abort_callback& aborter )
{
    const auto pApi = fb2k::imageLoaderLite::get();

    const auto mimeOpt = [&]() -> std::optional<qwr::u8string> {
        try
        {
            const auto imageInfo = pApi->getInfo( pArtData->get_ptr(), pArtData->get_size(), aborter );
            if ( !imageInfo.mime )
            {
                return std::nullopt;
            }

            return { imageInfo.mime };
        }
        catch ( const pfc::exception& /*e*/ )
        {
            return std::nullopt;
        }
    }();

    aborter.check();

    const auto fileExtOpt = [&]() -> std::optional<qwr::u8string> {
        if ( !mimeOpt )
        {
            return std::nullopt;
        }
        qwr::u8string_view mime = *mimeOpt;

        // Use mime type extension for image/ mime types. Not perfect and will fail for some of the more exotic types like svg
        constexpr qwr::u8string_view imagePrefix = "image/";
        if ( !mime.starts_with( imagePrefix ) )
        {
            return std::nullopt;
        }

        mime.remove_prefix( imagePrefix.size() );
        if ( mime.empty() )
        {
            return std::nullopt;
        }

        return qwr::u8string{ mime.data(), mime.size() };
    }();

    const auto imagePath = GetTempImageFilePathTemplate() + fileExtOpt.value_or( "jpg" );
    try
    {
        service_ptr_t<file> file_ptr;
        filesystem::g_open_write_new( file_ptr, imagePath.c_str(), aborter );
        file_ptr->write( pArtData->get_ptr(), pArtData->get_size(), aborter );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( "Failed to save temporary image file: ", e.what() );
    }

    return imagePath;
}

} // namespace

namespace drp
{

std::optional<qwr::u8string> UploadArt( const metadb_handle_ptr& handle, const qwr::u8string& uploaderPath )
{
    // TODO: split to different methods
    auto& aborter = fb2k::mainAborter();

    const auto artDataOpt = GetArtData( handle, aborter );
    if ( !artDataOpt )
    {
        return std::nullopt;
    }
    const auto& artData = *artDataOpt;

    aborter.check();

    const auto artPath = [&] {
        if ( artData.pathOpt )
        {
            return *artData.pathOpt;
        }

        return SaveArtToFile( artData.pArtData, aborter );
    }();

    aborter.check();

    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
    saAttr.bInheritHandle = true;
    saAttr.lpSecurityDescriptor = nullptr;

    HANDLE hStdoutRead{};
    HANDLE hStdoutWrite{};
    auto bRet = ::CreatePipe( &hStdoutRead, &hStdoutWrite, &saAttr, 0 );
    qwr::error::CheckWinApi( bRet, "CreatePipe(stdout)" );

    qwr::final_action autoStdoutReadPipe( [hStdoutRead] {
        ::CloseHandle( hStdoutRead );
    } );
    qwr::final_action autoStdoutWritePipe( [hStdoutWrite] {
        ::CloseHandle( hStdoutWrite );
    } );

    bRet = SetHandleInformation( hStdoutRead, HANDLE_FLAG_INHERIT, 0 );
    qwr::error::CheckWinApi( bRet, "SetHandleInformation(stdout)" );

    HANDLE hStdinRead{};
    HANDLE hStdinWrite{};
    bRet = ::CreatePipe( &hStdinRead, &hStdinWrite, &saAttr, 0 );
    qwr::error::CheckWinApi( bRet, "CreatePipe(stdin)" );

    qwr::final_action autoStdinReadPipe( [hStdinRead] {
        ::CloseHandle( hStdinRead );
    } );
    qwr::final_action autoStdinWritePipe( [hStdinWrite] {
        ::CloseHandle( hStdinWrite );
    } );

    bRet = SetHandleInformation( hStdinWrite, HANDLE_FLAG_INHERIT, 0 );
    qwr::error::CheckWinApi( bRet, "SetHandleInformation(stdin)" );

    const auto hJob = ::CreateJobObject( nullptr, nullptr );
    qwr::error::CheckWinApi( hJob, "CreateJobObject" );

    qwr::final_action autoJob( [hJob] {
        ::CloseHandle( hJob );
    } );

    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        bRet = ::SetInformationJobObject( hJob, JobObjectExtendedLimitInformation, &jeli, sizeof( jeli ) );
        qwr::error::CheckWinApi( bRet, "SetInformationJobObject" );
    }

    auto uploaderPathW = qwr::unicode::ToWide( uploaderPath );
    PROCESS_INFORMATION pi{};
    STARTUPINFO si{};
    si.cb = sizeof( si );
    si.hStdError = hStdoutWrite; // TODO: handle
    si.hStdOutput = hStdoutWrite;
    si.hStdInput = hStdinRead;
    si.dwFlags |= STARTF_USESTDHANDLES;
    bRet = ::CreateProcess( nullptr,
                            uploaderPathW.data(),
                            nullptr,
                            nullptr,
                            true,
                            CREATE_SUSPENDED | CREATE_NO_WINDOW,
                            nullptr,
                            nullptr,
                            &si,
                            &pi );
    qwr::error::CheckWinApi( bRet, "CreateProcess" );

    qwr::final_action autoProcess( [pi] {
        ::CloseHandle( pi.hProcess );
        ::CloseHandle( pi.hThread );
    } );

    bRet = ::AssignProcessToJobObject( hJob, pi.hProcess );
    qwr::error::CheckWinApi( bRet, "AssignProcessToJobObject" );

    auto iRet = ::ResumeThread( pi.hThread );
    qwr::error::CheckWinApi( iRet != -1, "ResumeThread" );

    {
        const auto artPathW = qwr::unicode::ToWide( artPath );
        DWORD written = 0;
        bRet = ::WriteFile( hStdinWrite, artPathW.data(), artPathW.size(), &written, nullptr );
        qwr::error::CheckWinApi( bRet, "AssignProcessToJobObject" );
        autoStdinWritePipe.cancel();
        ::CloseHandle( hStdinWrite );
    }

    {
        std::array handlesToWait{ pi.hProcess, aborter.get_handle() };
        const auto waitResult = WaitForMultipleObjects( handlesToWait.size(), handlesToWait.data(), false, static_cast<DWORD>( std::chrono::milliseconds{ kMaxWaitTime }.count() ) );
        if ( waitResult != WAIT_OBJECT_0 )
        {
            if ( waitResult == WAIT_TIMEOUT )
            {
                throw qwr::QwrException( "Upload process timed out" );
            }

            assert( waitResult == WAIT_OBJECT_0 + 1 );
            aborter.check();
        }

        autoProcess.cancel();
        ::CloseHandle( pi.hProcess );
        ::CloseHandle( pi.hThread );

        autoStdoutWritePipe.cancel();
        ::CloseHandle( hStdoutWrite );
        autoStdinReadPipe.cancel();
        ::CloseHandle( hStdinRead );
    }

    {
        std::array<char, 2048> stdoutBuf{};
        DWORD dwRead = 0;
        if ( !PeekNamedPipe( hStdoutRead, stdoutBuf.data(), stdoutBuf.size(), &dwRead, nullptr, nullptr ) || !dwRead )
        {
            throw qwr::QwrException( "Upload process didn't write anything to stdout" );
        }

        stdoutBuf.fill( 0 );
        dwRead = 0;
        bRet = ::ReadFile( hStdoutRead, stdoutBuf.data(), stdoutBuf.size(), &dwRead, nullptr );
        qwr::error::CheckWinApi( bRet, "ReadFile" );

        qwr::u8string_view url{ stdoutBuf.data(), strlen( stdoutBuf.data() ) };
        url = url.substr( 0, url.find_last_not_of( " \t\n\r" ) + 1 );
        if ( url.empty() )
        {
            return std::nullopt;
        }

        return qwr::u8string{ url.data(), url.size() };
    }
}

} // namespace drp
