#include <stdafx.h>

#include "uploader.h"

#include <fb2k/config.h>

#include <ctime>
#include <fb2k/artwork_metadb.h>

#include "image_hasher.h"
#include "foobar2000/SDK/component.h"

namespace drp::uploader
{
/**
 * Used to restrict uploading to one image at a time.
 * used by defining a variable of this type. Lock is automatically acquired,
 * and it is unlocked after the variable goes out of scope
 */
class upload_lock
{
public:
    upload_lock()
    {
        lock_.lock();
    }
    ~upload_lock()
    {
        lock_.unlock();
    }

    static bool is_locked()
    {
        const auto locked = lock_.try_lock();
        if ( locked )
        {
            lock_.unlock();
        }
        return !locked;
    }

private:
    static std::mutex lock_;
};

// Must initialize static class variables outside of the class
std::mutex upload_lock::lock_;

threaded_process_artwork_uploader::threaded_process_artwork_uploader(
    const pfc::map_t<metadb_index_hash, metadb_handle_ptr>& hashes) : hashes_(hashes)
{}

void threaded_process_artwork_uploader::on_init(ctx_t p_wnd) {}
    
void threaded_process_artwork_uploader::run(threaded_process_status &p_status, abort_callback &p_abort)
{
    pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
    const auto total_count = hashes_.get_count();
    t_size currIdx = 0;
    
    for (auto iter = hashes_.first(); iter.is_valid(); ++iter) {
        try
        {
            p_status.set_progress_float( currIdx / (double)total_count );
            const auto kv = *iter;
            
            if (record_get( kv.m_key ).artwork_url.get_length() > 0)
            {
                p_abort.check();
                currIdx++;
                continue;
            }

            pfc::string8 artwork_url;

            if (extractAndUploadArtwork(kv.m_value, p_abort, artwork_url, kv.m_key))
            {
                lstChanged += kv.m_key;
            }
            
            p_abort.check();
        }
        catch (exception_aborted)
        {
            return;
        }
        currIdx++;
    }

    p_status.set_progress_float(1);
    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": " << lstChanged.get_count() << " entries updated";


    if (lstChanged.get_count() > 0) {
        // This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
        fb2k::inMainThread([lstChanged]
        {
            cached_index_api()->dispatch_refresh(guid::artwork_url_index, lstChanged);
        });
    }
}

void threaded_process_artwork_uploader::on_done(ctx_t p_wnd,bool p_was_aborted)
{
}

bool extractAndUploadArtwork(const metadb_handle_ptr track, abort_callback &abort, pfc::string8 &artwork_url, metadb_index_hash hash)
{
    if ( config::uploadArtworkCommand.GetValue().length() == 0 )
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": No upload command given";
        return false;
    }

    bool wasLocked = upload_lock::is_locked();
    upload_lock lock;
    abort.check();

    // If we were locked check if this tracks artwork was uploaded and use that if it is found
    if (wasLocked)
    {
        const auto rec = record_get( hash );
        if (rec.artwork_url.get_length() > 0)
        {
            artwork_url = rec.artwork_url;
            return true;
        }
    }

    auto artwork = extractArtwork( track, abort );
    abort.check();
    if (artwork.success)
    {
#ifdef _DEBUG
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Artwork path after extract " << artwork.path;
#endif
        artwork_url = uploadArtwork( artwork, abort );
        if (artwork_url.get_length() > 0)
        {
            drp::artwork_url_set( hash, artwork_url );
            return true;
        }
    }

    return false;
}

artwork_info extractArtwork( const metadb_handle_ptr track, abort_callback &abort )
{
    auto aam = album_art_manager_v3::get();
    auto extractor = aam->open(pfc::list_single_ref_t(track),
                           pfc::list_single_ref_t(album_art_ids::cover_front), abort);

    if (!extractor.is_valid())
    {
        #ifdef _DEBUG
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Invalid artwork extractor instance found";
        #endif
        return artwork_info();
    }

    try {
        abort.check();
        artwork_info info;

        const auto paths = extractor->query_paths( album_art_ids::cover_front, abort );
        if (paths->get_count() > 0)
        {
            info.path = pfc::string8(paths->get_path(0));
            auto loc = track->get_location().get_path();

            #ifdef _DEBUG
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": extracted filepath " << info.path << " track filepath " << loc << " is valid? " << paths.is_valid();
            #endif
            // Artwork location same as file means artwork is embedded
            if (info.path.equals(loc))
            {
                info.path = "";
            }
        }
        else
        {
            info.path = "";
        }
        
        info.data = extractor->query(album_art_ids::cover_front, abort);
        info.success = true;

        #ifdef _DEBUG
        if (info.path != "")
        {
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": found existing path " << info.path;
        }
        #endif
        return info;
    } catch (const exception_album_art_not_found&) {
        return artwork_info();
    } catch (const exception_aborted&) {
        throw;
    } catch (...) {
        return artwork_info();
    }
}

/**
 * Initialize different process variables to be used with CreateProcess
 */
bool initializeProcessVariables(
    HANDLE &g_hChildStd_IN_Rd,
    HANDLE &g_hChildStd_IN_Wr,
    HANDLE &g_hChildStd_OUT_Rd,
    HANDLE &g_hChildStd_OUT_Wr,
    SECURITY_ATTRIBUTES &saAttr)
{
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if ( !CreatePipe( &g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0 ) )
        return false;

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if ( !SetHandleInformation( g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) )
        return false;

    // Create a pipe for the child process's STDIN.

    if ( !CreatePipe( &g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0 ) )
        return false;

    // Ensure the write handle to the pipe for STDIN is not inherited.

    if ( !SetHandleInformation( g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0 ) )
        return false;

    return true;
}

void closePipes(HANDLE pipe1, HANDLE pipe2, HANDLE pipe3, HANDLE pipe4)
{
    if (pipe1 != NULL) CloseHandle(pipe1);
    if (pipe2 != NULL) CloseHandle(pipe2);
    if (pipe3 != NULL) CloseHandle(pipe3);
    if (pipe4 != NULL) CloseHandle(pipe4);
}

bool readFromPipe(HANDLE g_hChildStd_OUT_Rd, pfc::string8 &artwork_url)
{
    bool inputFound = false;
    const int TEMP_BUF_SIZE = 16;
    CHAR tempBuf[TEMP_BUF_SIZE];
    DWORD peekedBytes;
    const auto now = std::chrono::high_resolution_clock::now();

    // Read timeout from conf. If set to 0 use 1 day as timeout.
    const long timeout_config = config::processTimeout.GetValue();
    const auto timeout_s = std::chrono::seconds(timeout_config == 0 ? 86400 : timeout_config);

    // Try to read output from the process. for 10 seconds
    while (!inputFound)
    {
        if (PeekNamedPipe( g_hChildStd_OUT_Rd, tempBuf, TEMP_BUF_SIZE, &peekedBytes, NULL, NULL ))
        {
            if (peekedBytes > 0)
            {
                inputFound = true;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const auto td = std::chrono::high_resolution_clock::now() - now;
        if (td > timeout_s) break;
    }

    bool rSuccess = false;
    if (inputFound)
    {
        DWORD dwRead;
        // Should be good enough amount of characters
        const int BUFSIZE = 2048;
        CHAR chBuf[BUFSIZE];
        rSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL );
        artwork_url = pfc::string8(chBuf, dwRead);
        // rtrim space like characters
        artwork_url.skip_trailing_chars(" \t\n\r");
    }

    return rSuccess;
}

pfc::string8 getArtworkFilepath(const artwork_info& art, abort_callback &abort, pfc::string8 &tempFile, bool &deleteFile)
{
    abort.check();

    pfc::string8 filepath;

    // Tempfile assigned later to make sure the actual cover art is not deleted after uploading
    tempFile = "";

    deleteFile = false;

    // If cover art is not embedded just use that file as the input.
    // No need to copy it to another folder
    if (strlen(art.path) > 0)
    {
        filepath = pfc::string8(art.path);
    }
    else
    {
        auto tempDir = core_api::pathInProfile(DRP_UNDERSCORE_NAME);

        if (!filesystem::g_exists(tempDir.c_str(), abort))
        {
            filesystem::g_create_directory(tempDir.c_str(), abort);
        }

        // Get the image mime type since we cannot deduce it otherwise from embedded artwork
        const auto api = fb2k::imageLoaderLite::tryGet();
        std::string ext = "jpg";
        if (api.is_valid())
        {
            const auto info = api->getInfo(art.data->get_ptr(), art.data->get_size(), abort);
            abort.check();

            const auto mime = std::string(info.mime);
            // Use mime type extension for image/ mime types. Not perfect and will fail for some of the more exotic types like svg
            if (mime.rfind("image/", 0) == 0)
            {
                ext = mime.substr(mime.find("/") + 1);
            }
        }

        // Take the last 10 digits of current time and use that for filename.
        // Should be good enough for this purpose as the file is deleted after the operation or overwritten in the future
        const auto ts = std::to_string(std::time(NULL));
        const auto filename = pfc::string8((ts.substr(std::max(ts.size(), 10u) - 10) + "." + ext).c_str());

        tempDir.add_filename(filename.c_str());
        filepath = tempDir;
        deleteFile = true;

        #ifdef _DEBUG
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": full temp filepath " << filepath;
        #endif

        // UTF-8 might cause problems?
        tempFile = filepath;
        {
            service_ptr_t<file> file_ptr;
            // File gets released after file_ptr has been deleted
            filesystem::g_open_write_new( file_ptr, tempFile, abort );
            file_ptr->write( art.data->get_ptr(), art.data->get_size(), abort );
        }
    }

    if (filepath.has_prefix("file://"))
    {
        // Remove file:// protocol since the program expects a file path instead of a uri
        if (filepath.replace_string("file://", "") > 1)
        {
            // This should never really be reached
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Multiple instances of \"file://\" replaced during cover upload";
        }
    }

    return filepath;
}

std::wstring to_wstring(const std::string &str)
{
    // Just check size
    int convertResult = MultiByteToWideChar(
            CP_UTF8,
            0,
            str.c_str(),
            str.length(),
            NULL,
            0);

    if ( convertResult <= 0 )
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Failed to convert command to utf-16. Error code " << convertResult;
        throw std::invalid_argument( "Could not convert command to utf-16" );
    }

    std::wstring str_w;
    // https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    // The example has + 10 without any comment as to why
    str_w.resize( convertResult + 10 );

    // This one writes the bytes to str_w
    convertResult = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        str.length(),
        &str_w[0],
        (int)str_w.size() );

    if ( convertResult <= 0 )
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Failed to convert command to utf-16. Error code " << convertResult;
        throw std::invalid_argument( "Could not convert command to utf-16" );
    }

    return str_w;
}

bool uploadOpenProcess(const std::wstring &cmd_w, const char* filepath_c, pfc::string8 &artwork_url,
    STARTUPINFO &siStartInfo, PROCESS_INFORMATION &piProcInfo,
    HANDLE &g_hChildStd_OUT_Wr, HANDLE &g_hChildStd_IN_Rd, HANDLE &g_hChildStd_IN_Wr, HANDLE &g_hChildStd_OUT_Rd)
{
     // Create the child process.
     bool bSuccess = CreateProcessW(NULL,
        (LPWSTR)cmd_w.c_str(), // command line
        NULL,          // process security attributes
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited
        CREATE_NO_WINDOW, // creation flags
        NULL,          // use parent's environment
        NULL,          // use parent's current directory
        &siStartInfo,  // STARTUPINFO pointer
        &piProcInfo);  // receives PROCESS_INFORMATION

     if (bSuccess)
     {
         // Close handles to the stdin and stdout pipes no longer needed by the child process.
         // If they are not explicitly closed, there is no way to recognize that the child process has ended.
         CloseHandle( g_hChildStd_OUT_Wr );
         g_hChildStd_OUT_Wr = NULL;
         CloseHandle( g_hChildStd_IN_Rd );
         g_hChildStd_IN_Rd = NULL;

         try
         {
             DWORD dwWritten;
             bool wSuccess = WriteFile(g_hChildStd_IN_Wr, filepath_c, strlen( filepath_c ), &dwWritten, NULL );
             CloseHandle( g_hChildStd_IN_Wr );
             g_hChildStd_IN_Wr = NULL;

             bool rSuccess = readFromPipe(g_hChildStd_OUT_Rd, artwork_url);

             if ( wSuccess && rSuccess )
             {
                 WaitForSingleObject( piProcInfo.hProcess, 5000 );
             }
         }
         catch (...)
         {
             // Make sure handles are close in case of error
             CloseHandle( piProcInfo.hProcess );
             CloseHandle( piProcInfo.hThread );
             throw;
         }

         DWORD exit_code;
         GetExitCodeProcess( piProcInfo.hProcess, &exit_code );
         bool terminateProcess = exit_code == STILL_ACTIVE && artwork_url.get_length() == 0;

         // In case of a time out terminate the process
         if (terminateProcess)
         {
             TerminateProcess(&piProcInfo.hProcess, exit_code);
             WaitForSingleObject( piProcInfo.hProcess, 5000 );
         }

         CloseHandle( piProcInfo.hProcess );
         CloseHandle( piProcInfo.hThread );

         // If exit code is zero and result contains newlines assume it's an error since urls should not contains those
         const bool isError = exit_code != 0 || artwork_url.find_first('\n') != ~0;
         artwork_url =  terminateProcess ? pfc::string8("Process timed out") : artwork_url;

         FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": artwork uploader exited with status: " << exit_code <<
             " and " << ( isError ? "error" : "url" ) << ": " << artwork_url;

         if (isError)
         {
             artwork_url = "";
         }

         return true;
     }

    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Failed to run command '" << cmd_w << "'";

    return false;
}

pfc::string8 uploadArtwork(artwork_info& art, abort_callback &abort)
{
    pfc::string8 artwork_url = "";

    if ( check_artwork_hash(art, abort, artwork_url) )
    {
        return artwork_url;
    }

    const std::string commandString = config::uploadArtworkCommand.GetValue();
    if ( commandString.length() == 0 )
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": No upload command given";
        return artwork_url;
    }

    pfc::string8 tempFile;
    bool deleteFile;
    auto filepath = getArtworkFilepath(art, abort, tempFile, deleteFile);

    const auto filepath_c = filepath.c_str();

    abort.check();
   
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    
    SECURITY_ATTRIBUTES saAttr;

     try
     {
         if ( !initializeProcessVariables(g_hChildStd_IN_Rd, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr, saAttr) )
         {
             closePipes(g_hChildStd_IN_Rd, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr);
             return artwork_url;
         }

         STARTUPINFO siStartInfo;

         ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
         siStartInfo.cb = sizeof(STARTUPINFO);
         siStartInfo.hStdError = g_hChildStd_OUT_Wr;
         siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
         siStartInfo.hStdInput = g_hChildStd_IN_Rd;
         siStartInfo.dwFlags |= STARTF_USESTDHANDLES;


         PROCESS_INFORMATION piProcInfo; 
         ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

         #ifdef _DEBUG
         FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Upload command " << commandString;
         FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Cover file path " << filepath;
         #endif
         const auto cmd_w = to_wstring( commandString );

         abort.check();

         uploadOpenProcess(cmd_w, filepath_c, artwork_url,
             siStartInfo, piProcInfo,
             g_hChildStd_OUT_Wr, g_hChildStd_IN_Rd, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd);
     } catch (...)
     {
         closePipes(g_hChildStd_IN_Rd, g_hChildStd_IN_Wr, g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr);

         if ( deleteFile )
         {
             filesystem::g_remove( tempFile, abort );
         }
         
         FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": artwork uploader threw an error";
         throw;
     }

    if ( deleteFile )
    {
        filesystem::g_remove(tempFile, abort);
    }

    abort.check();

    if (artwork_url.get_length() > 0)
    {
        set_artwork_url_hash(artwork_url, art.artwork_hash, abort);
    }
    
    return artwork_url;
}

}
