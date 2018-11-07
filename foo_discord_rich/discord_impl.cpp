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

    void UpdateImage();
    void UpdateTrack( metadb_handle_ptr metadb = metadb_handle_ptr() );
    void UpdateDuration( double time );

    void SendPresense();
    void DisableDurationPresence();
    void DisableAllPresence();

private:
    static void OnReady( const DiscordUser* request );
    static void OnDisconnected( int errorCode, const char* message );
    static void OnErrored( int errorCode, const char* message );

private:
    DiscordRichPresence presence_;

    metadb_handle_ptr metadb_;

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
    Discord_RunCallbacks();

    UpdateImage();
}

void DiscordHandler::Finalize()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordHandler::OnSettingsChanged()
{
    UpdateImage();
    UpdateTrack();
    SendPresense();
}

void DiscordHandler::UpdateImage()
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

void DiscordHandler::UpdateTrack( metadb_handle_ptr metadb )
{
    state_.reset();
    details_.reset();
    trackLength_ = 0;

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
    UpdateDuration( durationStr.is_empty() ? 0 : stold( std::string( durationStr ) ) );
}

void DiscordHandler::UpdateDuration( double time )
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

void DiscordHandler::SendPresense()
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

void DiscordHandler::DisableDurationPresence()
{
    presence_.startTimestamp = 0;
    presence_.endTimestamp = 0;
    SendPresense();
}

void DiscordHandler::DisableAllPresence()
{
    Discord_ClearPresence();
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
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override
    { // ignore
    }
    void on_playback_new_track( metadb_handle_ptr track ) override
    {
        on_playback_changed( track );
    }
    void on_playback_stop( play_control::t_stop_reason reason ) override
    {
        if ( play_control::t_stop_reason::stop_reason_starting_another != reason )
        {
            g_discordHandler.DisableAllPresence();
        }
    }
    void on_playback_seek( double time ) override
    {
        g_discordHandler.UpdateDuration( time );
        if ( playback_control::get()->is_playing() )
        { // track seeking might take some time, thus on_playback_time is needed
            needTimeRefresh_ = true;
        }
        else
        {
            g_discordHandler.SendPresense();
        }
    }
    void on_playback_pause( bool state ) override
    {
        if ( state )
        {
            g_discordHandler.DisableDurationPresence();
        }
        else
        {
            needTimeRefresh_ = true;
        }
    }
    void on_playback_edited( metadb_handle_ptr track ) override
    {
        on_playback_changed( track );
    }
    void on_playback_dynamic_info( const file_info& info ) override
    { // ignore
    }
    void on_playback_dynamic_info_track( const file_info& info ) override
    {
        on_playback_changed();
    }
    void on_playback_time( double time ) override
    {
        if ( needTimeRefresh_ )
        {
            g_discordHandler.UpdateDuration( time );
            g_discordHandler.SendPresense();
            needTimeRefresh_ = false;
        }
    }
    void on_volume_change( float p_new_val ) override
    { // ignore
    }

private:
    void on_playback_changed( metadb_handle_ptr track = metadb_handle_ptr() )
    {
        g_discordHandler.UpdateTrack( track );
        g_discordHandler.SendPresense();
    }

private:
    bool needTimeRefresh_ = false;
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
