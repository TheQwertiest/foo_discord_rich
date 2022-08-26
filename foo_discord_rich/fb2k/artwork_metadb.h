#pragma once

namespace drp
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
    pfc::string8 artwork_url;
};

metadb_index_client_impl * clientByGUID( const GUID & guid );
metadb_index_manager::ptr cached_index_api();
void record_set( metadb_index_hash hash, const record_t & record);
record_t record_get( metadb_index_hash hash);
bool artwork_url_set( metadb_index_hash hash, const pfc::string8 &artwork_url );

}
