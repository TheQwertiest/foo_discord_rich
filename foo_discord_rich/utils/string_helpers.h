#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <charconv>

namespace drp::string
{

template <typename T>
std::basic_string_view<T> TrimView( std::basic_string_view<T> str )
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, ( last - first + 1 ) );
}

template <typename T>
std::basic_string<T> Trim( std::basic_string_view<T> str )
{
    const auto view = TrimView( str );
    return std::basic_string<T>{ view.data(), view.size() };
}

std::vector<std::u8string_view> SplitByLines( std::u8string_view str );

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, const std::basic_string<T>& separator )
{
    return ranges::view::split( str, separator )
           | ranges::view::transform( []( auto&& rng ) {
                 return std::basic_string_view<T>{ &*rng.begin(), static_cast<size_t>( ranges::distance( rng ) ) };
             } );
}

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, T separator )
{
    return Split( str, std::basic_string<T>( 1, separator ) );
}

template <typename T, typename T2>
std::optional<T> GetNumber( std::basic_string_view<T2> strView, int base = 10 )
{
    T number;
    if ( auto [pos, ec] = std::from_chars( strView.data(), strView.data() + strView.size(), number, base );
         ec == std::errc{} )
    {
        return number;
    }
    else
    {
        return std::nullopt;
    }
}

} // namespace drp::string
