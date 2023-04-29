#pragma once
#include "uploader.h"


namespace drp::uploader
{

bool set_artwork_url_hash(const pfc::string8 &artwork_url, const pfc::string8 &artwork_hash, abort_callback &abort);
bool check_artwork_hash(artwork_info &artwork, abort_callback &abort, pfc::string8 &artwork_url);
bool delete_artwork_urls_from_json(const pfc::avltree_t<pfc::string8> &urls, abort_callback &abort, int& deleted);
bool clear_all_hashes(abort_callback &abort);

} // namespace drp::uploader
