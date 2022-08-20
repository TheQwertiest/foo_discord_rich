﻿#pragma once

namespace drp::uploader
{


class threaded_process_artwork_uploader : public threaded_process_callback
{
public:
    threaded_process_artwork_uploader( const pfc::map_t<metadb_index_hash, metadb_handle_ptr>& hashes );
    void on_init(ctx_t p_wnd) override;
    void run(threaded_process_status & p_status,abort_callback & p_abort) override;
    void on_done(ctx_t p_wnd,bool p_was_aborted) override;
private:
    pfc::map_t<metadb_index_hash, metadb_handle_ptr> hashes_{};
};
    
// A class that turns metadata + location info into hashes to which our data gets pinned by the backend.
class metadb_index_client_impl : public metadb_index_client
{
public:
    metadb_index_client_impl( const char * pinTo );
    metadb_index_hash transform(const file_info & info, const playable_location & location);
private:
    titleformat_object::ptr m_keyObj;
};

/* metadb record */
struct record_t {
    pfc::string8 artwork_url;
};

struct artwork_info
{
    album_art_data_ptr data;
    const char* path{};
    bool success = false;
};

metadb_index_client_impl * clientByGUID( const GUID & guid );
bool extractAndUploadArtwork(const metadb_handle_ptr track, abort_callback &abort, pfc::string8 &artwork_url, metadb_index_hash hash);
artwork_info extractArtwork( const metadb_handle_ptr track, abort_callback &abort );
pfc::string8 uploadArtwork(const artwork_info& art, abort_callback &abort);
metadb_index_manager::ptr cached_index_api();
void record_set( metadb_index_hash hash, const record_t & record);
record_t record_get( metadb_index_hash hash);
bool artwork_url_set( metadb_index_hash hash, const pfc::string8 &artwork_url );
    
} // namespace drp::uploader