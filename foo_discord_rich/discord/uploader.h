#pragma once

namespace drp::uploader
{

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
    pfc::string8 cover_url;
};

struct album_art_info
{
    album_art_data_ptr data;
    bool success = false;
};

metadb_index_client_impl * clientByGUID( const GUID & guid );
album_art_info extractAlbumArt( const metadb_handle_ptr track, std::shared_ptr<abort_callback> abort );
pfc::string8 uploadAlbumArt(const album_art_info& art, std::shared_ptr<abort_callback> abort);
metadb_index_manager::ptr cover_api();
void record_set( const GUID & indexID, metadb_index_hash hash, const record_t & record);
record_t record_get(const GUID & indexID, metadb_index_hash hash);
bool cover_url_set( const GUID & indexID, metadb_index_hash hash, const char * cover_url );
    
} // namespace drp::uploader
