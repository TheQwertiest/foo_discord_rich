#pragma once

#include <string>

namespace drp::pfc_x
{

template <typename T>
std::basic_string<T> uGetWindowText( HWND wnd )
{
    static_assert( std::is_same_v<T, wchar_t> || std::is_same_v<T, char8_t> || std::is_same_v<T, char> );

    auto size = ::GetWindowTextLength( wnd );
    if ( !size )
    {
        return std::basic_string<T>{};
    }

    std::wstring text;
    text.resize( size + 1 );
    (void)::GetWindowText( wnd, text.data(), text.size() );
    text.resize( wcslen( text.c_str() ) );

    if constexpr ( std::is_same_v<T, wchar_t> )
    {
        return text;
    }
    else
    {
        return drp::unicode::ToU8( text );
    }
}

template <typename T>
std::basic_string<T> uGetDlgItemText( HWND wnd, UINT id )
{
    const auto hControl = ::GetDlgItem( wnd, id );
    if ( !hControl )
    {
        return std::basic_string<T>{};
    }

    return drp::pfc_x::uGetWindowText<T>( hControl );
}

} // namespace drp::pfc_x