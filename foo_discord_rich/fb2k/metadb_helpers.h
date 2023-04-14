#pragma once

namespace drp
{
/**
 * Gets all of the unique hashes in the index artwork_url_index
 */
pfc::avltree_t<metadb_index_hash> getHashes( metadb_handle_list_cref tracks );

}
