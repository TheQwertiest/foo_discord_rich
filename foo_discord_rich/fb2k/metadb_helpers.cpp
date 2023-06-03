#include "stdafx.h"
#include "metadb_helpers.h"
#include "artwork_metadb.h"

namespace drp
{
   pfc::avltree_t<metadb_index_hash> getHashes( metadb_handle_list_cref tracks ) {
      const size_t count = tracks.get_count();
      if (count == 0) return pfc::avltree_t<metadb_index_hash>();

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
      }

      return allHashes;
   }
}
