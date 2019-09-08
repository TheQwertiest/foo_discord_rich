#include <stdafx.h>
#include "unicode.h"

namespace drp::unicode
{

std::wstring ToWide( const std::u8string_view& src )
{
    size_t stringLen = MultiByteToWideChar( CP_UTF8, 0, src.data(), src.size(), nullptr, 0 );
    std::wstring strVal;
    strVal.resize( stringLen );

    stringLen = MultiByteToWideChar( CP_UTF8, 0, src.data(), src.size(), strVal.data(), strVal.size() );
    strVal.resize( stringLen );

	return strVal;
}

std::u8string ToU8( const std::wstring_view& src )
{
    size_t stringLen = WideCharToMultiByte( CP_UTF8, 0, src.data(), src.size(), nullptr, 0, nullptr, nullptr );
    std::u8string strVal;
    strVal.resize( stringLen );

    stringLen = WideCharToMultiByte( CP_UTF8, 0, src.data(), src.size(), strVal.data(), strVal.size(), nullptr, nullptr );
    strVal.resize( stringLen );

	return strVal;
}

} // namespace drp::unicode
