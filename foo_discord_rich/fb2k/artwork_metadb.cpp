#include "stdafx.h"

#include "artwork_metadb.h"

#include "config.h"

namespace drp
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
    static metadb_index_client_impl* g_clientTrack = new service_impl_single_t<metadb_index_client_impl>( config::artworkMetadbKey.GetValue().c_str() );

    PFC_ASSERT( guid == guid::artwork_url_index );
    return g_clientTrack;
}


// Static cached ptr to metadb_index_manager
// Cached because we'll be calling it a lot on per-track basis, let's not pass it everywhere to low level functions
// Obtaining the pointer from core is reasonably efficient - log(n) to the number of known service classes, but not good enough for something potentially called hundreds of times
static metadb_index_manager::ptr g_cachedAPI;
metadb_index_manager::ptr cached_index_api() {
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
    }
};

static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;
static service_factory_single_t<initquit_impl> g_initquit_impl;
}
