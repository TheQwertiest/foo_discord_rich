#include "stdafx.h"

#include <discord_rpc.h>

#include <ctime>

using namespace discord;

namespace
{

class DiscordDataHandler
{
public:
    DiscordDataHandler()
    {
        memset( &presence_, 0, sizeof( presence_ ) );
        presence_.largeImageKey = "foobar2000";
    }

    void Initialize()
    {
        DiscordEventHandlers handlers;
        memset( &handlers, 0, sizeof( handlers ) );

        Discord_Initialize( "507982587416018945", &handlers, 1, nullptr );
    }

    void Finalize()
    {
        Discord_Shutdown();
    }

public:
    void UpdateTrackData( metadb_handle_ptr metadb )
    {
        state_.reset();
        details_.reset();
        partyIdQuery_.reset();
        trackLength_ = 0;
        needToRefreshTime_ = true;

        auto pc = playback_control::get();
        const bool isPlaying = pc->is_playing();

        titleformat_object::ptr tfState;
        titleformat_compiler::get()->compile_safe( tfState, stateQuery_ );
        titleformat_object::ptr tfDetails;
        titleformat_compiler::get()->compile_safe( tfDetails, detailsQuery_ );
        titleformat_object::ptr tfPartyId;
        titleformat_compiler::get()->compile_safe( tfPartyId, partyIdQuery_ );
        titleformat_object::ptr tfLength;
        titleformat_compiler::get()->compile_safe( tfLength, "[%length_seconds%]" );
        pfc::string8_fast lengthStr;
        titleformat_object::ptr tfDuration;
        titleformat_compiler::get()->compile_safe( tfDuration, "[%playback_time_seconds%]" );
        pfc::string8_fast durationStr;

        if ( isPlaying )
        {
            metadb_handle_ptr dummyHandle;
            pc->playback_format_title_ex( dummyHandle, nullptr, state_, tfState, nullptr, playback_control::display_level_all );
            pc->playback_format_title_ex( dummyHandle, nullptr, details_, tfDetails, nullptr, playback_control::display_level_all );
            pc->playback_format_title_ex( dummyHandle, nullptr, partyId_, tfPartyId, nullptr, playback_control::display_level_all );
            pc->playback_format_title_ex( dummyHandle, nullptr, lengthStr, tfLength, nullptr, playback_control::display_level_all );
            pc->playback_format_title_ex( dummyHandle, nullptr, durationStr, tfDuration, nullptr, playback_control::display_level_all );
        }
        else if ( metadb.is_valid() )
        {
            metadb->format_title( nullptr, state_, tfState, nullptr );
            metadb->format_title( nullptr, details_, tfDetails, nullptr );
            metadb->format_title( nullptr, partyId_, tfPartyId, nullptr );
            metadb->format_title( nullptr, lengthStr, tfLength, nullptr );
            metadb->format_title( nullptr, durationStr, tfDuration, nullptr );
        }

        trackLength_ = ( lengthStr.is_empty() ? 0 : stoll( std::string( lengthStr ) ) );
        uint64_t trackDuration = ( durationStr.is_empty() ? 0 : stoll( std::string( durationStr ) ) );

        presence_.state = state_.c_str();
        presence_.details = details_.c_str();
        presence_.partyId = partyId_.c_str();
        presence_.startTimestamp = 0;
        presence_.endTimestamp = ( trackLength_ ? std::time( nullptr ) + std::max<uint64_t>( 0, ( trackLength_ - trackDuration ) ) : 0 );

        Discord_UpdatePresence( &presence_ );
    }

    void DisableTrackData()
    {
        Discord_ClearPresence();
    }

    void UpdateDuration( double time, bool force = false )
    {
        if ( !needToRefreshTime_ && !force )
        {
            return;
        }

        if ( !trackLength_ )
        {
            return;
        }

        presence_.endTimestamp = std::time( nullptr ) + std::max<uint64_t>( 0, ( trackLength_ - std::llround( time ) ) );

        Discord_UpdatePresence( &presence_ );
    }

    void DisableDuration()
    {
        presence_.endTimestamp = 0;

        Discord_UpdatePresence( &presence_ );
    }

    void RequestTimeRefresh()
    {
        needToRefreshTime_ = true;
    }

private:
    DiscordRichPresence presence_;

    pfc::string8_fast stateQuery_ = "[%title%]";
    pfc::string8_fast detailsQuery_ = "[%album artist%[: %album%]]";
    pfc::string8_fast partyIdQuery_;

    bool needToRefreshTime_ = false;
    pfc::string8_fast state_;
    pfc::string8_fast details_;
    pfc::string8_fast partyId_;
    uint64_t trackLength_ = 0;
};

DiscordDataHandler g_discordHandler;

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
        g_discordHandler.UpdateTrackData( metadb_handle_ptr() );
    }
    void on_playback_edited( metadb_handle_ptr track ) override
    {
        g_discordHandler.UpdateTrackData( track );
    }
    void on_playback_new_track( metadb_handle_ptr track ) override
    {
        g_discordHandler.UpdateTrackData( track );
    }
    void on_playback_pause( bool state ) override
    {
        if ( state )
        {
            g_discordHandler.DisableDuration();
        }
        else
        {
            g_discordHandler.RequestTimeRefresh();
        }
    }
    void on_playback_seek( double time ) override
    {
        g_discordHandler.UpdateDuration( time, true );
        g_discordHandler.RequestTimeRefresh(); ///< track seeking might take some time, thus on_playback_time is needed
    }
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override
    {
    }
    void on_playback_stop( play_control::t_stop_reason reason ) override
    {
        if ( play_control::t_stop_reason::stop_reason_starting_another != reason )
        {
            g_discordHandler.DisableTrackData();
        }
    }
    void on_playback_time( double time ) override
    {
        g_discordHandler.UpdateDuration( time );
    }
    void on_volume_change( float p_new_val ) override
    {
    }
};

play_callback_static_factory_t<PlaybackCallback> playbackCallback;

} // namespace

namespace discord
{

void InitializeDiscord()
{
    g_discordHandler.Initialize();
}

void FinalizeDiscord()
{
    g_discordHandler.Finalize();
}

} // namespace discord
