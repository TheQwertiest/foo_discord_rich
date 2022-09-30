#pragma once

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


struct artwork_info
{
    album_art_data_ptr data;
    // This being a char* caused problems where the contents would randomly change. pfc::string8 did not seem to have this problem
    pfc::string8 path{};
    bool success = false;
    pfc::string8 artwork_hash{};
};

bool extractAndUploadArtwork( const metadb_handle_ptr track, abort_callback &abort, pfc::string8 &artwork_url, metadb_index_hash hash );
artwork_info extractArtwork( const metadb_handle_ptr track, abort_callback &abort );
pfc::string8 uploadArtwork( artwork_info& art, abort_callback &abort );

} // namespace drp::uploader
