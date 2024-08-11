#pragma once

#include <fb2k/advanced_config.h>

namespace drp
{

namespace internal
{

void LogDebugImpl( const qwr::u8string& message );

}

template <typename... Args>
void LogDebug( qwr::u8string_view message, Args&&... messageFmtArgs )
{
    if ( config::advanced::enableDebugLog )
    {
        internal::LogDebugImpl( fmt::format( fmt::runtime( message ), std::forward<Args>( messageFmtArgs )... ) );
    }
}

void LogError( const qwr::u8string& message );
void LogWarning( const qwr::u8string& message );

} // namespace drp
