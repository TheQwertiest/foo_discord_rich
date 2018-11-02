#include "stdafx.h"

#include <discord_rpc.h>

#include <ctime>

namespace
{

bool g_needToRefreshTime = false;
pfc::string8_fast g_trackName;
pfc::string8_fast g_artistAndAlbum;
uint64_t g_trackLength = 0;
DiscordRichPresence g_discordPresence;

void UpdateTrackData( metadb_handle_ptr metadb )
{
    g_trackName.reset();
    g_artistAndAlbum.reset();
    g_trackLength = 0;
    g_needToRefreshTime = true;

    auto pc = playback_control::get();
    const bool isPlaying = pc->is_playing();

    titleformat_object::ptr tfTrack;
    titleformat_compiler::get()->compile_safe( tfTrack, "[%title%]" );
    titleformat_object::ptr tfArtistAndAlbum;
    titleformat_compiler::get()->compile_safe( tfArtistAndAlbum, "[%album artist%[: %album%]]" );
    titleformat_object::ptr tfLength;
    titleformat_compiler::get()->compile_safe( tfLength, "[%length_seconds%]" );
    pfc::string8_fast lengthStr;
    if ( isPlaying )
    {
        metadb_handle_ptr dummyHandle;
        pc->playback_format_title_ex( dummyHandle, nullptr, g_trackName, tfTrack, nullptr, playback_control::display_level_all );
        pc->playback_format_title_ex( dummyHandle, nullptr, g_artistAndAlbum, tfArtistAndAlbum, nullptr, playback_control::display_level_all );
        pc->playback_format_title_ex( dummyHandle, nullptr, lengthStr, tfLength, nullptr, playback_control::display_level_all );
    }
    else if ( metadb.is_valid() )
    {
        metadb->format_title( nullptr, g_trackName, tfTrack, nullptr );
        metadb->format_title( nullptr, g_artistAndAlbum, tfArtistAndAlbum, nullptr );
        metadb->format_title( nullptr, lengthStr, tfLength, nullptr );
    }

    g_trackLength = ( lengthStr.is_empty() ? 0 : stoll( std::string( lengthStr ) ) );

    g_discordPresence.state = g_trackName.c_str();
    g_discordPresence.details = g_artistAndAlbum.c_str();
    g_discordPresence.startTimestamp = 0;
    g_discordPresence.endTimestamp = ( g_trackLength ? std::time( nullptr ) + g_trackLength : 0 );

    Discord_UpdatePresence( &g_discordPresence );
}

void UpdateDuration( double time )
{
    if ( !g_trackLength )
    {
        return;
    }

    g_discordPresence.endTimestamp = std::time( nullptr ) + std::max<uint64_t>( 0, ( g_trackLength - std::llround( time ) ) );

    Discord_UpdatePresence( &g_discordPresence );
}

void DisableDuration()
{
    g_discordPresence.endTimestamp = 0;

    Discord_UpdatePresence( &g_discordPresence );
}

} // namespace

namespace
{

class PlaybackCallback : public play_callback_static
{
public:
    unsigned get_flags() override
    {
        return flag_on_playback_all;
    }
    void on_playback_dynamic_info( const file_info& info ) override
    {
    }
    void on_playback_dynamic_info_track( const file_info& info ) override
    {
        UpdateTrackData( metadb_handle_ptr() );
    }
    void on_playback_edited( metadb_handle_ptr track ) override
    {
        UpdateTrackData( track );
    }
    void on_playback_new_track( metadb_handle_ptr track ) override
    {
        UpdateTrackData( track );
    }
    void on_playback_pause( bool state ) override
    {
        if ( state )
        {
            DisableDuration();
        }
    }
    void on_playback_seek( double time ) override
    {
        UpdateDuration( time );
        g_needToRefreshTime = true;
    }
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override
    {
    }
    void on_playback_stop( play_control::t_stop_reason reason ) override
    {
        if ( play_control::t_stop_reason::stop_reason_user == reason
             || play_control::t_stop_reason::stop_reason_shutting_down == reason )
        {
            Discord_ClearPresence();
        }
    }
    void on_playback_time( double time ) override
    {
        if ( g_needToRefreshTime )
        {
            UpdateDuration( time );
            g_needToRefreshTime = false;
        }
    }
    void on_volume_change( float p_new_val ) override
    {
    }
};

play_callback_static_factory_t<PlaybackCallback> g_playbackCallback;

} // namespace

namespace discord
{

void InitializeDiscord()
{
    DiscordEventHandlers handlers;
    memset( &handlers, 0, sizeof( handlers ) );

    Discord_Initialize( "507982587416018945", &handlers, 1, nullptr );

    memset( &g_discordPresence, 0, sizeof( g_discordPresence ) );
    g_discordPresence.largeImageKey = "foobar2000";
}

void FinalizeDiscord()
{
    Discord_Shutdown();
}

} // namespace discord
