#include <stdafx.h>

#include "uploader.h"

#include <fb2k/config.h>

#include <ctime>

namespace drp::uploader
{
    

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
    static metadb_index_client_impl * g_clientTrack = new service_impl_single_t<metadb_index_client_impl>("%album artist% - $if2([%album%],%title%) [ - %discnumber%]");

    PFC_ASSERT( guid == guid::cover_art_url_index );
    return g_clientTrack;
}

album_art_info extractAlbumArt( const metadb_handle_ptr track, std::shared_ptr<abort_callback> abort )
{
    static_api_ptr_t<album_art_manager_v3> aam;
    auto extractor = aam->open(pfc::list_single_ref_t(track),
                           pfc::list_single_ref_t(album_art_ids::cover_front), *abort);

    try {
        abort->check();
        album_art_info info;
        
        info.data = extractor->query(album_art_ids::cover_front, *abort);
        info.success = true;

        return info;

    } catch (const exception_album_art_not_found&) {
        return album_art_info();
    } catch (const exception_aborted&) {
        throw;
    } catch (...) {
        return album_art_info();
    }
}

pfc::string8 uploadAlbumArt(const album_art_info& art, std::shared_ptr<abort_callback> abort)
{
    auto tempDir = core_api::pathInProfile(DRP_UNDERSCORE_NAME);

    if (!filesystem::g_exists(tempDir.c_str(), *abort))
    {
        filesystem::g_create_directory(tempDir.c_str(), *abort);
    }

    abort->check();

    // Get the image mime type since we cannot deduce it otherwise from embedded cover art
    auto api = fb2k::imageLoaderLite::tryGet();
    std::string ext = "jpg";
    if (api.is_valid())
    {
        const auto info = api->getInfo(art.data->get_ptr(), art.data->get_size(), *abort);
        abort->check();

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
    pfc::string filename = pfc::string((ts.substr(ts.size() - 10) + "." + ext).c_str());

    tempDir.add_filename(filename.c_str());
    if (tempDir.has_prefix("file://"))
    {
        // Remove file:// protocol since a normal path is easier to process
        if (tempDir.replace_string("file://", "") > 1)
        {
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Multiple instances of \"file://\" replaced during cover upload";
        }
    }

    // UTF-8 might cause problems?
    const auto tempFile = tempDir.c_str();
 
    {
        service_ptr_t<file> file_ptr;
        // File gets released after file_ptr has been deleted
        filesystem::g_open_write_new(file_ptr, tempFile, *abort);
        file_ptr->write(art.data->get_ptr(), art.data->get_size(), *abort);
    }

    abort->check();
   
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    pfc::string8 cover_url = "";

     try
     {
         if ( !CreatePipe( &g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0 ) )
             return cover_url;

         // Ensure the read handle to the pipe for STDOUT is not inherited.

         if ( !SetHandleInformation( g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) )
             return cover_url;

         // Create a pipe for the child process's STDIN.

         if ( !CreatePipe( &g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0 ) )
             return cover_url;

         // Ensure the write handle to the pipe for STDIN is not inherited.

         if ( !SetHandleInformation( g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0 ) )
             return cover_url;

         STARTUPINFOA siStartInfo;
    
         ZeroMemory( &siStartInfo, sizeof(STARTUPINFOA) );
         siStartInfo.cb = sizeof(STARTUPINFOA); 
         siStartInfo.hStdError = g_hChildStd_OUT_Wr;
         siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
         siStartInfo.hStdInput = g_hChildStd_IN_Rd;
         siStartInfo.dwFlags |= STARTF_USESTDHANDLES;


         PROCESS_INFORMATION piProcInfo; 
         ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
         auto commandString = config::uploadCoverArtCommand.GetValue();
         
         // TODO Unicode will make this break completely
         LPSTR command = const_cast<char*>( commandString.c_str() );

         abort->check();

         // Create the child process. Not unicode friendly and converting from u8string to w_chart* is a pain
         bool bSuccess = CreateProcessA(NULL, 
            command,     // command line 
            NULL,          // process security attributes 
            NULL,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            CREATE_NO_WINDOW,             // creation flags 
            NULL,          // use parent's environment 
            NULL,          // use parent's current directory 
            &siStartInfo,  // STARTUPINFO pointer 
            &piProcInfo);  // receives PROCESS_INFORMATION

         if (bSuccess)
         {
             // Close handles to the stdin and stdout pipes no longer needed by the child process.
             // If they are not explicitly closed, there is no way to recognize that the child process has ended.
             CloseHandle( g_hChildStd_OUT_Wr );
             CloseHandle( g_hChildStd_IN_Rd );

             try
             {
                 DWORD dwWritten;
                 bool wSuccess = WriteFile(g_hChildStd_IN_Wr, tempFile, strlen( tempFile ), &dwWritten, NULL );
                 CloseHandle( g_hChildStd_IN_Wr );


                 bool inputFound = false;
                 const int TEMP_BUF_SIZE = 16;
                 CHAR tempBuf[TEMP_BUF_SIZE];
                 DWORD peekedBytes;
                 auto now = std::chrono::high_resolution_clock::now();
                 const auto timeout_s = std::chrono::seconds(5);

                 // Try to read output from the process. for 5 seconds
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
                     cover_url = pfc::string8(chBuf, dwRead);
                 }

                 if ( wSuccess && rSuccess )
                 {
                     WaitForSingleObject( piProcInfo.hProcess, 5000 );
                 }
             }
             catch (...)
             {
                 // Make sure handles are close in case of error
                 CloseHandle( g_hChildStd_IN_Wr );
                 CloseHandle( piProcInfo.hProcess );
                 CloseHandle( piProcInfo.hThread );
                 throw;
             }
            
             
             DWORD exit_code;
             GetExitCodeProcess( piProcInfo.hProcess, &exit_code );
             CloseHandle( piProcInfo.hProcess );
             CloseHandle( piProcInfo.hThread );

             FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": cover art uploader exited with status: " << exit_code << " and url: " << cover_url;
         }
     } catch (...)
     {
         filesystem::g_remove(tempFile, *abort);
         FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": cover art uploader threw an error";
         throw;
     }

    filesystem::g_remove(tempFile, *abort);

    abort->check();
    
    return cover_url;
}
    
// Static cached ptr to metadb_index_manager
// Cached because we'll be calling it a lot on per-track basis, let's not pass it everywhere to low level functions
// Obtaining the pointer from core is reasonably efficient - log(n) to the number of known service classes, but not good enough for something potentially called hundreds of times
static metadb_index_manager::ptr g_cachedAPI;
static metadb_index_manager::ptr cover_api() {
    auto ret = g_cachedAPI;
    if ( ret.is_empty() ) ret = metadb_index_manager::get(); // since fb2k SDK v1.4, core API interfaces have a static get() method
    return ret;
}
    
void record_set( const GUID & indexID, metadb_index_hash hash, const record_t & record) {

    stream_writer_formatter_simple< /* using bing endian data? nope */ false > writer;
    writer << record.cover_url;

    cover_api()->set_user_data( indexID, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size() );
}

    
record_t record_get(const GUID & indexID, metadb_index_hash hash) {
    mem_block_container_impl temp; // this will receive our BLOB
    cover_api()->get_user_data( indexID, hash, temp );
    if ( temp.get_size() > 0 ) {
        try {
            // Parse the BLOB using stream formatters
            stream_reader_formatter_simple_ref reader(temp.get_ptr(), temp.get_size());

            record_t ret;

            if ( reader.get_remaining() > 0 ) {
                reader >> ret.cover_url;
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
        if ( stage == init_stages::before_config_read )
        {
            auto api = metadb_index_manager::get();
            g_cachedAPI = api;
            // Important, handle the exceptions here!
            // This will fail if the files holding our data are somehow corrupted.
            try
            {
                api->add( clientByGUID( guid::cover_art_url_index ), guid::cover_art_url_index, system_time_periods::week * 4 );
            }
            catch ( std::exception const& e )
            {
                api->remove( guid::cover_art_url_index );
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
    }
};

static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;
static service_factory_single_t<initquit_impl> g_initquit_impl;

    
bool cover_url_set( const GUID & indexID, metadb_index_hash hash, const char * cover_url )
{
    auto rec = record_get(indexID, hash );
    bool bChanged = false;
    if ( ! rec.cover_url.equals( cover_url ) ) {
        rec.cover_url = cover_url;
        record_set(indexID, hash, rec);
        bChanged = true;
    }

    return bChanged;
}

// TODO: Not fully implemented
static void generateUrls( const GUID & whichID, metadb_handle_list_cref tracks ) {
    const size_t count = tracks.get_count();
    if (count == 0) return;

    auto client = clientByGUID(whichID);

    pfc::string8 coverUrl;

    // Sorted/dedup'd set of all hashes of p_data items.
    // pfc::avltree_t<> is pfc equivalent of std::set<>.
    // We go the avltree_t<> route because more than one track in p_data might produce the same hash value, see metadb_index_client_impl / strPinTo
    pfc::avltree_t<metadb_index_hash> allHashes;
    for (size_t w = 0; w < count; ++w) {
        metadb_index_hash hash;
        if (client->hashHandle(tracks[w], hash)) {
            allHashes += hash;
        }
    }

    if (allHashes.get_count() == 0) {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Could not hash any of the tracks due to unavailable metadata, bailing";
        return;
    }

    pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
    for (auto iter = allHashes.first(); iter.is_valid(); ++iter) {
        const metadb_index_hash hash = *iter;

        // TODO get the first track with hash and use that?
        if ( cover_url_set(whichID, hash, "") ) {
            lstChanged += hash;
        }
    }

    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": " << lstChanged.get_count() << " entries updated";
    if (lstChanged.get_count() > 0) {
        // This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
        cover_api()->dispatch_refresh(whichID, lstChanged);
    }
}

    
class contextmenu_cover_url : public contextmenu_item_simple {
public:
	GUID get_parent() {
		return guid::context_menu_group;
	}
	unsigned get_num_items() {
		return 1;
	}
	void get_item_name(unsigned p_index, pfc::string_base & p_out) {
		PFC_ASSERT( p_index < get_num_items() );
		switch(p_index) {
		    case 0:
		        p_out = "Generate cover art url"; break;
		}
		
	}
	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) {
		PFC_ASSERT( p_index < get_num_items() );
		
		const GUID whichID = guid::cover_art_url_index;
	    
	    generateUrls( whichID, p_data );
	}
	GUID get_item_guid(unsigned p_index) {
		switch(p_index) {
		    case 0:	return guid::context_menu_item_generate_url;
		    default: uBugCheck();
		}
	}
	bool get_item_description(unsigned p_index, pfc::string_base & p_out) {
		PFC_ASSERT( p_index < get_num_items() );
		switch( p_index ) {
		case 0:
			p_out = "Generate url to be use with discord rich presence";
			return true;
		default:
			PFC_ASSERT(!"Should not get here");
			return false;
		}
	}
};

// TODO not currently implemented properly so context menu options just hidden for now 
//static contextmenu_group_popup_factory g_mygroup( guid::context_menu_group, contextmenu_groups::root, "Discord rich presence", 0 );
//static contextmenu_item_factory_t< contextmenu_cover_url > g_contextmenu_rating;


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
				auto rec = record_get(whichID, *i);


				if ( bFirst ) {
					strComment = rec.cover_url;
				} else if ( ! bVarComments ) {
					if ( strComment != rec.cover_url ) {
						bVarComments = true;
						strComment = "<various>";
					}
				}

				bFirst = false;
			}
		}

		p_out.set_property(strPropertiesGroup, priorityBase, PFC_string_formatter() << "Cover Art Url", strComment);
	}
	void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
		workThisIndex( guid::cover_art_url_index, 0, p_tracks, p_out );
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
