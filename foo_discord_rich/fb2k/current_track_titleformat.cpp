#include <stdafx.h>

#include "current_track_titleformat.h"

namespace drp
{

qwr::u8string EvaluateQueryForCurrentTrack( const metadb_handle_ptr& handle, const qwr::u8string& query )
{
    auto pc = playback_control::get();
    titleformat_object::ptr tf;
    titleformat_compiler::get()->compile_safe( tf, query.c_str() );

    pfc::string8_fast result;
    if ( pc->is_playing() )
    {
        metadb_handle_ptr dummyHandle;
        pc->playback_format_title_ex( dummyHandle, nullptr, result, tf, nullptr, playback_control::display_level_all );
    }
    else if ( handle.is_valid() )
    {
        handle->format_title( nullptr, result, tf, nullptr );
    }

    return result.c_str();
}

} // namespace drp
