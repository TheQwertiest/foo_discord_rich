#pragma once

#include <optional>

namespace drp::musicbrainz
{

/// @throw qwr::QwrException
/// @throw exception_aborted
std::optional<qwr::u8string> FetchArt( const qwr::u8string& album, const qwr::u8string& artist, const std::optional<qwr::u8string>& userReleaseMbidOpt );

} // namespace drp::musicbrainz
