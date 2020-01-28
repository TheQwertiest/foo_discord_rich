#pragma once

#include <string>
#include <string_view>

namespace drp::unicode
{

std::wstring ToWide( const std::u8string_view& src );
std::u8string ToU8( const std::wstring_view& src );

} // namespace drp::unicode
