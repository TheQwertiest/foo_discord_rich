#include <stdafx.h>

#include "uploader.h"

#include <fb2k/config.h>

#include <ctime>
#include <nlohmann/json.hpp>

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

pfc::string8 get_image_hash_file(abort_callback &abort)
{
    static const auto path = core_api::pathInProfile(DRP_UNDERSCORE_NAME);
    auto filename = pfc::string8(path);
    filename.add_filename("image_hashes.json");

    if (!filesystem::g_exists(path.c_str(), abort))
    {
        filesystem::g_create_directory(path.c_str(), abort);
    }

    return filename;
}

bool restore_old_hash_json(abort_callback &abort)
{
    const auto &filename = get_image_hash_file(abort);
    auto filename_old = filename;
    filename_old += ".old";

    if (filesystem::g_exists(filename_old.c_str(), abort))
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Restoring old artwork hash json file because the actual file one was not found or it was corrupted";
        filesystem::g_copy(filename_old, filename, abort);
        return true;
    }

    return false;
}

/* Parses image hash json into a unique ptr */
bool read_hash_json(std::unique_ptr<nlohmann::json> &json, abort_callback &abort)
{
    const auto &filename = get_image_hash_file(abort);
    if ( !filesystem::g_exists(filename.c_str(), abort) && !restore_old_hash_json(abort) )
    {
        return false;
    }

    {
        pfc::string8 json_string;
        // Files needs to be scoped like this so it gets released immediately after reading
        {
            service_ptr_t<file> file;
            filesystem::g_open_read( file, filename.c_str(), abort );
            
            file->read_string_raw( json_string, abort );
        }

        try
        {
            json = std::make_unique<nlohmann::json>( nlohmann::json::parse(std::string( json_string.c_str() )));
        }
        catch (const nlohmann::detail::parse_error&)
        {
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Image hash json parsing failed. Trying to restore old file";
            if ( restore_old_hash_json( abort ) )
            {
                service_ptr_t<file> file;
                filesystem::g_open_read( file, filename.c_str(), abort );
                file->read_string_raw( json_string, abort );
                json = std::make_unique<nlohmann::json>( nlohmann::json::parse( std::string( json_string.c_str() ) ) );
            }
        }
    }

    return true;
}

static std::unique_ptr<nlohmann::json> g_hashJson;
bool prepare_static_hash_json( abort_callback& abort )
{
    static bool loaded = false;
    if (!loaded)
    {
        if ( read_hash_json( g_hashJson, abort ) )
        {
            loaded = true;
        }
    }

    return loaded;
}


bool check_artwork_hash(artwork_info &artwork, abort_callback &abort, pfc::string8 &artwork_url)
{
    const auto md5 = static_api_ptr_t<hasher_md5>()->process_single(artwork.data->get_ptr(), artwork.data->get_size()).asString();
    artwork.artwork_hash = md5;

    if ( !prepare_static_hash_json( abort ) )
    {
        return false;
    }

    if (g_hashJson->contains(md5.c_str()))
    {
        artwork_url = pfc::string8(
            g_hashJson->at(md5.toString()).get<std::string>().c_str()
        );
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Found url " << artwork_url << " with image hash " << md5;

        return true;
    }

    return false;
}

/**
 * Saves the hash json file making sure existing data is not lost in case of an error
 */
bool save_hash_json(abort_callback &abort)
{
    if ( !prepare_static_hash_json( abort) )
    {
        return false;
    }

    const auto &filename = get_image_hash_file(abort);
    auto filename_new = pfc::string8(filename);
    filename_new += ".new";

    auto filename_old = pfc::string8(filename);
    filename_old += ".old";

    // Write current json to file
    {
        service_ptr_t<file> file_ptr;
        filesystem::g_open_write_new(file_ptr, filename_new, abort);
        file_ptr->write_string_raw(g_hashJson->dump(4).c_str(), abort);
    }

    // Copy the old file to backup file
    filesystem::g_copy(filename, filename_old, abort);

    // Copy the new file to the main file
    filesystem::g_copy(filename_new, filename, abort);

    return true;
}


bool set_artwork_url_hash(const pfc::string8 &artwork_url, const pfc::string8 &artwork_hash, abort_callback &abort)
{
    if ( !prepare_static_hash_json( abort) )
    {
        return false;
    }

    (*g_hashJson)[artwork_hash.c_str()] = artwork_url.c_str();

    // Should be fine to call this for every new entry as it's only reached from upload at the moment and that
    // has a concurrency lock
    return save_hash_json(abort);
}


metadb_index_client_impl::metadb_index_client_impl( const char * pinTo ) {
    static_api_ptr_t<titleformat_compiler>()->compile_force(m_keyObj, pinTo);
}

metadb_index_hash metadb_index_client_impl::transform(const file_info & info, const playable_location & location) {
    pfc::string_formatter str;
    m_keyObj->run_simple( location, &info, str );
    // Make MD5 hash of the string, then reduce it to 64-bit metadb_index_hash
    return static_api_ptr_t<hasher_md5>()->process_single_string( str ).xorHalve();
}

metadb_index_client_impl * clientByGUID( const GUID & guid ) {
    // Static instances, never destroyed (deallocated with the process), created first time we get here
    // Using service_impl_single_t, reference counting disabled
    // This is somewhat ugly, operating on raw pointers instead of service_ptr, but OK for this purpose
    static metadb_index_client_impl* g_clientTrack = new service_impl_single_t<metadb_index_client_impl>( config::artworkMetadbKey.GetValue().c_str() );

    PFC_ASSERT( guid == guid::artwork_url_index );
    return g_clientTrack;
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
        artwork_url = uploadArtwork( artwork, abort );
        if (artwork_url.get_length() > 0)
        {
            artwork_url_set( hash, artwork_url );
            return true;
        }
    }

    return false;
}

artwork_info extractArtwork( const metadb_handle_ptr track, abort_callback &abort )
{
    static_api_ptr_t<album_art_manager_v3> aam;
    auto extractor = aam->open(pfc::list_single_ref_t(track),
                           pfc::list_single_ref_t(album_art_ids::cover_front), abort);

    try {
        abort.check();
        artwork_info info;

        const auto paths = extractor->query_paths( album_art_ids::cover_front, abort );
        if (paths->get_count() > 0)
        {
            info.path = paths->get_path(0);
            auto loc = track->get_location().get_path();
            // Artwork location same as file means artwork is embedded
            if (strcmp(info.path, loc) == 0)
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
    auto now = std::chrono::high_resolution_clock::now();
    const auto timeout_s = std::chrono::seconds(10);

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
        auto td = std::chrono::high_resolution_clock::now() - now;
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
    auto tempDir = core_api::pathInProfile(DRP_UNDERSCORE_NAME);

    if (!filesystem::g_exists(tempDir.c_str(), abort))
    {
        filesystem::g_create_directory(tempDir.c_str(), abort);
    }

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
        // Get the image mime type since we cannot deduce it otherwise from embedded artwork
        auto api = fb2k::imageLoaderLite::tryGet();
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
        auto filename = pfc::string((ts.substr(ts.size() - 10) + "." + ext).c_str());

        tempDir.add_filename(filename.c_str());
        filepath = tempDir;
        deleteFile = true;

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

         const auto cmd_w = to_wstring( commandString );

         abort.check();

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
             CloseHandle( piProcInfo.hProcess );
             CloseHandle( piProcInfo.hThread );

             FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": artwork uploader exited with status: " << exit_code <<
                 " and " << ( exit_code == 0 ? "url" : "error" ) << ": " << artwork_url;

             if (exit_code != 0)
             {
                 artwork_url = "";
             }
         }
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
    
// Static cached ptr to metadb_index_manager
// Cached because we'll be calling it a lot on per-track basis, let's not pass it everywhere to low level functions
// Obtaining the pointer from core is reasonably efficient - log(n) to the number of known service classes, but not good enough for something potentially called hundreds of times
static metadb_index_manager::ptr g_cachedAPI;
static metadb_index_manager::ptr cached_index_api() {
    auto ret = g_cachedAPI;
    if ( ret.is_empty() ) ret = metadb_index_manager::get(); // since fb2k SDK v1.4, core API interfaces have a static get() method
    return ret;
}
    
void record_set( metadb_index_hash hash, const record_t & record) {
    stream_writer_formatter_simple< /* using bing endian data? nope */ false > writer;
    writer << record.artwork_url;

    cached_index_api()->set_user_data( guid::artwork_url_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
}

    
record_t record_get( metadb_index_hash hash) {
    mem_block_container_impl temp; // this will receive our BLOB
    cached_index_api()->get_user_data( guid::artwork_url_index, hash, temp );
    if ( temp.get_size() > 0 ) {
        try {
            // Parse the BLOB using stream formatters
            stream_reader_formatter_simple_ref reader(temp.get_ptr(), temp.get_size());

            record_t ret;

            if ( reader.get_remaining() > 0 ) {
                reader >> ret.artwork_url;
            }
            
            return ret;
        } catch (exception_io_data) {
            // we get here as a result of stream formatter data error
            // fall thru to return a blank record
        }
    }
    return record_t();
}

// Our group in Properties dialog / Details tab, see track_property_provider_impl
static const char strPropertiesGroup[] = "Discord rich presence";

	// An init_stage_callback to hook ourselves into the metadb
// We need to do this properly early to prevent dispatch_global_refresh() from new fields that we added from hammering playlists etc
class init_stage_callback_impl : public init_stage_callback
{
public:
    void on_init_stage( t_uint32 stage )
    {
        if ( stage == init_stages::after_config_read )
        {
            auto api = metadb_index_manager::get();
            g_cachedAPI = api;
            // Important, handle the exceptions here!
            // This will fail if the files holding our data are somehow corrupted.
            try
            {
                api->add( clientByGUID( guid::artwork_url_index ), guid::artwork_url_index, system_time_periods::week * 4 );
            }
            catch ( std::exception const& e )
            {
                api->remove( guid::artwork_url_index );
                FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Critical initialization failure: " << e;
                return;
            }
            api->dispatch_global_refresh();
        }
    }
};
class initquit_impl : public initquit
{
public:
    void on_quit()
    {
        // Cleanly kill g_cachedAPI before reaching static object destructors or else
        g_cachedAPI.release();
        g_hashJson.release();
    }
};

static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;
static service_factory_single_t<initquit_impl> g_initquit_impl;

    
bool artwork_url_set( metadb_index_hash hash, const pfc::string8 &artwork_url )
{
    auto rec = record_get( hash );
    bool bChanged = false;
    if ( ! rec.artwork_url.equals( artwork_url ) ) {
        rec.artwork_url = artwork_url;
        record_set( hash, rec );
        bChanged = true;
    }

    return bChanged;
}

    

void generateUrls( metadb_handle_list_cref tracks ) {
    const size_t count = tracks.get_count();
    if (count == 0) return;

    auto client = clientByGUID(guid::artwork_url_index);
    
    pfc::map_t<metadb_index_hash, metadb_handle_ptr> allHashes;
    for (size_t w = 0; w < count; ++w) {
        metadb_index_hash hash;
        if (client->hashHandle(tracks[w], hash)) {
            if (allHashes.exists(hash)) continue;
            allHashes.set(hash, tracks[w]);
        }
    }

    if (allHashes.get_count() == 0) {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Could not hash any of the tracks due to unavailable metadata, bailing";
        return;
    }

    auto thread_impl = new service_impl_t<threaded_process_artwork_uploader>(allHashes);
    const std::string p_title = "Uploading artwork";

    threaded_process::g_run_modeless(
        thread_impl,
        threaded_process::flag_show_progress | threaded_process::flag_show_abort | threaded_process::flag_show_delayed,
        g_foobar2000_api->get_main_window(),
        p_title.c_str(),
        p_title.length()
    );
}

void clearUrls( metadb_handle_list_cref tracks ) {
    const size_t count = tracks.get_count();
    if (count == 0) return;

    auto client = clientByGUID(guid::artwork_url_index);
    pfc::avltree_t<metadb_index_hash> allHashes;

    for (size_t w = 0; w < count; ++w) {
        metadb_index_hash hash;
        if (client->hashHandle(tracks[w], hash)) {
            if (allHashes.exists(hash)) continue;
            allHashes += hash;
        }
    }

    if (allHashes.get_count() == 0) {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Could not hash any of the tracks due to unavailable metadata, bailing";
        return;
    }

    pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed

    for (auto iter = allHashes.first(); iter.is_valid(); ++iter)
    {
        const metadb_index_hash hash = *iter;
        if (artwork_url_set( hash, "" ))
        {
            lstChanged += hash;
        }
    }

    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": " << lstChanged.get_count() << " entries cleared";
    if (lstChanged.get_count() > 0) {
        // This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
        cached_index_api()->dispatch_refresh(guid::artwork_url_index, lstChanged);
    }
}
    
class contextmenu_artwork_url : public contextmenu_item_simple {
public:
	GUID get_parent() {
		return guid::context_menu_group;
	}

	unsigned get_num_items() {
		return 2;
	}

	void get_item_name(unsigned p_index, pfc::string_base & p_out) {
		PFC_ASSERT( p_index < get_num_items() );
		switch(p_index) {
		    case 0:
		        p_out = "Generate artwork url"; break;
		    case 1:
		        p_out = "Clear artwork"; break;
		}
	}

	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) {
		PFC_ASSERT( p_index < get_num_items() );

        switch (p_index)
        {
        case 0:
            generateUrls( p_data );
            break;
        case 1:
            clearUrls( p_data );
            break;
        default:
            uBugCheck();    
        }
	}

	GUID get_item_guid(unsigned p_index) {
		switch(p_index) {
		    case 0:	return guid::context_menu_item_generate_url;
		    case 1:	return guid::context_menu_item_clear_url;
		    default: uBugCheck();
		}
	}

	bool get_item_description(unsigned p_index, pfc::string_base & p_out) {
		PFC_ASSERT( p_index < get_num_items() );
		switch( p_index ) {
		case 0:
			p_out = "Generate urls to be used with discord rich presence";
			return true;
		case 1:
		    // Currently clears based on album. Can be changed after a good way of storing image hashes is found
		    p_out = "Clear artwork urls of selected albums";
		    return true;
		default:
			PFC_ASSERT(!"Should not get here");
			return false;
		}
	}
};

static contextmenu_group_popup_factory g_mygroup( guid::context_menu_group, contextmenu_groups::root, "Discord rich presence", 0 );
static contextmenu_item_factory_t< contextmenu_artwork_url > g_contextmenu_rating;


class track_property_provider_impl : public track_property_provider_v2 {
public:
	void workThisIndex(GUID const & whichID, double priorityBase, metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
		auto client = clientByGUID( whichID );
		pfc::avltree_t<metadb_index_hash> hashes;
		const size_t trackCount = p_tracks.get_count();
		for (size_t trackWalk = 0; trackWalk < trackCount; ++trackWalk) {
			metadb_index_hash hash;
			if (client->hashHandle(p_tracks[trackWalk], hash)) {
				hashes += hash;
			}
		}

		pfc::string8 strComment;

		{
			bool bFirst = true;
			bool bVarComments = false;
			for (auto i = hashes.first(); i.is_valid(); ++i) {
				auto rec = record_get( *i );


				if ( bFirst ) {
					strComment = rec.artwork_url;
				} else if ( ! bVarComments ) {
					if ( strComment != rec.artwork_url ) {
						bVarComments = true;
						strComment = "<various>";
					}
				}

				bFirst = false;
			}
		}

		p_out.set_property(strPropertiesGroup, priorityBase, PFC_string_formatter() << "Artwork Url", strComment);
	}
	void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
		workThisIndex( guid::artwork_url_index, 0, p_tracks, p_out );
	}
	void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out) {
		if ( p_out.is_group_wanted( strPropertiesGroup ) ) {
			enumerate_properties( p_tracks, p_out );
		}
	}
	
	bool is_our_tech_info(const char * p_name) { 
		// If we do stuff with tech infos read from the file itself (see file_info::info_* methods), signal whether this field belongs to us
		// We don't do any of this, hence false
		return false; 
	}
};

static service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;
}
