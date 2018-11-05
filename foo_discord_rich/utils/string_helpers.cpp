#include <stdafx.h>
#include "string_helpers.h"

namespace
{

// TODO: remove tabs as well (string_view, isspace, manual looping?)

template <typename T>
T TrimImpl(const T& str)
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, (last - first + 1) );
}

template <>
pfc::string8_fast TrimImpl( const pfc::string8_fast& str )
{
    size_t first = 0;
    for ( first = 0; first < str.length(); ++first )
    {
        if ( str[first] != ' ' )
        {
            break;
        }
    }

    size_t last = 0;
    for ( last = str.length() - 1; last >= 0; --last )
    {
        if ( str[last] != ' ' )
        {
            break;
        }
    }

    if ( ( first == 0 ) && last == ( str.length() - 1 ) )
    {
        return str;
    }

    return pfc::string8_fast( &str[first], last - first + 1 );
}


}

namespace utils::string
{

std::string Trim( const std::string& str )
{
    return TrimImpl( str );
}

std::wstring Trim( const std::wstring& str )
{
    return TrimImpl( str );
}

pfc::string8_fast Trim( const pfc::string8_fast& str )
{
    return TrimImpl( str );
}

}
