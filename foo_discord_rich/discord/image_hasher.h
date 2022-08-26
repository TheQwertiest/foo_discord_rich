#pragma once
#include "uploader.h"


namespace drp::uploader
{

bool set_artwork_url_hash(const pfc::string8 &artwork_url, const pfc::string8 &artwork_hash, abort_callback &abort);
bool check_artwork_hash(artwork_info &artwork, abort_callback &abort, pfc::string8 &artwork_url);

} // namespace drp::uploader
