#include <stdafx.h>
#include "discord_impl.h"

#include <config.h>

#include <discord_rpc.h>

#include <ctime>

namespace
{

using namespace drp;

class DiscordHandler
{
public:
    DiscordHandler();

    void Initialize();
    void Finalize();

public:
    void OnSettingsChanged();

    void SetImagePresence();
    void UpdateTrackData( metadb_handle_ptr metadb = metadb_handle_ptr() );
    void DisableTrackData();
    void UpdateDuration( double time, bool force = false );
    void DisableDuration();
    void RequestTimeRefresh();

private:
    void SetDurationInPresence( double time );
    void UpdatePresense();

private:
    static void OnReady( const DiscordUser* request );
    static void OnDisconnected( int errorCode, const char* message );
    static void OnErrored( int errorCode, const char* message );

private:
    DiscordRichPresence presence_;

    metadb_handle_ptr metadb_;

    bool needToRefreshTime_ = false;
    pfc::string8_fast state_;
    pfc::string8_fast details_;
    uint64_t trackLength_ = 0;
};

DiscordHandler::DiscordHandler()
{
    memset( &presence_, 0, sizeof( presence_ ) );
}

void DiscordHandler::Initialize()
{
    DiscordEventHandlers handlers;
    memset( &handlers, 0, sizeof( handlers ) );

    handlers.ready = OnReady;
    handlers.disconnected = OnDisconnected;
    handlers.errored = OnErrored;

    Discord_Initialize( "507982587416018945", &handlers, 1, nullptr );
    SetImagePresence();
}

void DiscordHandler::Finalize()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordHandler::OnSettingsChanged()
{
    SetImagePresence();
    UpdateTrackData();
}

void DiscordHandler::SetImagePresence()
{
    switch ( static_cast<config::ImageSetting>( config::g_imageSettings.GetSavedValue() ) )
    {
    case config::ImageSetting::Light:
    {
        presence_.largeImageKey = "foobar2000";
        break;
    }
    case config::ImageSetting::Dark:
    {
        presence_.largeImageKey = "foobar2000-dark";
        break;
    }
    case config::ImageSetting::Disabled:
    {
        presence_.largeImageKey = nullptr;
        break;
    }
    }
}

void DiscordHandler::UpdateTrackData( metadb_handle_ptr metadb )
{
    state_.reset();
    details_.reset();
    trackLength_ = 0;
    needToRefreshTime_ = true;

    if ( metadb.is_valid() )
    { // Need to save in case refresh requested on settings update
        metadb_ = metadb;
    }

    auto pc = playback_control::get();
    auto queryData = [&pc, metadb = metadb_]( const pfc::string8_fast& query ) {
        titleformat_object::ptr tf;
        titleformat_compiler::get()->compile_safe( tf, query );
        pfc::string8_fast result;

        if ( pc->is_playing() )
        {
            metadb_handle_ptr dummyHandle;
            pc->playback_format_title_ex( dummyHandle, nullptr, result, tf, nullptr, playback_control::display_level_all );
        }
        else if ( metadb.is_valid() )
        {
            metadb->format_title( nullptr, result, tf, nullptr );
        }

        return result;
    };

    state_ = queryData( config::g_stateQuery );
    state_.truncate( 127 );
    details_ = queryData( config::g_detailsQuery );
    details_.truncate( 127 );

    pfc::string8_fast lengthStr = queryData( "[%length_seconds%]" );
    trackLength_ = ( lengthStr.is_empty() ? 0 : stoll( std::string( lengthStr ) ) );

    pfc::string8_fast durationStr = queryData( "[%playback_time_seconds%]" );

    presence_.state = state_;
    presence_.details = details_;
    SetDurationInPresence( durationStr.is_empty() ? 0 : stold( std::string( durationStr ) ) );

    UpdatePresense();
}

void DiscordHandler::DisableTrackData()
{
    Discord_ClearPresence();
}

void DiscordHandler::UpdateDuration( double time, bool force )
{
    if ( !needToRefreshTime_ && !force )
    {
        return;
    }

    SetDurationInPresence( time );
    UpdatePresense();

    needToRefreshTime_ = false;
}

void DiscordHandler::DisableDuration()
{
    presence_.startTimestamp = 0;
    presence_.endTimestamp = 0;
    UpdatePresense();
}

void DiscordHandler::RequestTimeRefresh()
{
    needToRefreshTime_ = true;
}

void DiscordHandler::SetDurationInPresence( double time )
{
    auto pc = playback_control::get();
    const config::TimeSetting timeSetting = ( ( trackLength_ && pc->is_playing() && !pc->is_paused() )
                                                  ? static_cast<config::TimeSetting>( config::g_timeSettings.GetSavedValue() )
                                                  : config::TimeSetting::Disabled );
    switch ( timeSetting )
    {
    case config::TimeSetting::Elapsed:
    {
        presence_.startTimestamp = std::time( nullptr ) - std::llround( time );
        presence_.endTimestamp = 0;

        break;
    }
    case config::TimeSetting::Remaining:
    {
        presence_.startTimestamp = 0;
        presence_.endTimestamp = std::time( nullptr ) + std::max<uint64_t>( 0, ( trackLength_ - std::llround( time ) ) );

        break;
    }
    case config::TimeSetting::Disabled:
    {
        presence_.startTimestamp = 0;
        presence_.endTimestamp = 0;

        break;
    }
    }
}

void DiscordHandler::UpdatePresense()
{
    if ( config::g_isEnabled )
    {
        Discord_UpdatePresence( &presence_ );
    }
    else
    {
        Discord_ClearPresence();
    }
    Discord_RunCallbacks();
}

void DiscordHandler::OnReady( const DiscordUser* request )
{
    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": connected to " << ( request->username ? request->username : "<null>" );
}

void DiscordHandler::OnDisconnected( int errorCode, const char* message )
{
    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": disconnected with code " << errorCode;
    if ( message )
    {
        FB2K_console_formatter() << message;
    }
}

void DiscordHandler::OnErrored( int errorCode, const char* message )
{
    FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": error " << errorCode;
    if ( message )
    {
        FB2K_console_formatter() << message;
    }
}

DiscordHandler g_discordHandler;

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
        g_discordHandler.UpdateTrackData();
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

namespace drp
{

void InitializeDiscord()
{
    g_discordHandler.Initialize();
}

void FinalizeDiscord()
{
    g_discordHandler.Finalize();
}

void UpdateDiscordSettings()
{
    g_discordHandler.OnSettingsChanged();
}

} // namespace drp
