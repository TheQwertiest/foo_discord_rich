#include <stdafx.h>

#include "discord_impl.h"

#include <fb2k/config.h>

#include <ctime>

namespace drp::internal
{

PresenceData::PresenceData()
{
    memset( &presence, 0, sizeof( presence ) );
    presence.state = state.c_str();
    presence.details = details.c_str();
}

PresenceData::PresenceData( const PresenceData& other )
{
    CopyData( other );
}

PresenceData& PresenceData::operator=( const PresenceData& other )
{
    if ( this != &other )
    {
        CopyData( other );
    }

    return *this;
}

bool PresenceData::operator==( const PresenceData& other )
{
    auto areStringsSame = []( const char* a, const char* b ) {
        return ( ( a == b ) || ( a && b && !strcmp( a, b ) ) );
    };

    return ( areStringsSame( presence.state, other.presence.state )
             && areStringsSame( presence.details, other.presence.details )
             && areStringsSame( presence.largeImageKey, other.presence.largeImageKey )
             && areStringsSame( presence.largeImageText, other.presence.largeImageText )
             && areStringsSame( presence.smallImageKey, other.presence.smallImageKey )
             && areStringsSame( presence.smallImageText, other.presence.smallImageText )
             && presence.startTimestamp == other.presence.startTimestamp
             && presence.endTimestamp == other.presence.endTimestamp
             && trackLength == other.trackLength );
}

bool PresenceData::operator!=( const PresenceData& other )
{
    return !operator==( other );
}

void PresenceData::CopyData( const PresenceData& other )
{
    metadb = other.metadb;
    state = other.state;
    details = other.details;
    largeImageKey = other.largeImageKey;
    smallImageKey = other.smallImageKey;
    trackLength = other.trackLength;

    memcpy( &presence, &other.presence, sizeof( presence ) );
    presence.state = state.c_str();
    presence.details = details.c_str();
    presence.largeImageKey = largeImageKey.empty() ? nullptr : largeImageKey.c_str();
    presence.smallImageKey = smallImageKey.empty() ? nullptr : smallImageKey.c_str();
}

} // namespace drp::internal

namespace drp
{

PresenceModifier::PresenceModifier( DiscordHandler& parent,
                                    const drp::internal::PresenceData& presenceData )
    : parent_( parent )
    , presenceData_( presenceData )
{
}

PresenceModifier::~PresenceModifier()
{
    const bool hasChanged = ( parent_.presenceData_ != presenceData_ );
    if ( hasChanged )
    {
        parent_.presenceData_ = presenceData_;
    }

    const bool needsToBeDisabled = ( isDisabled_
                                     || !playback_control::get()->is_playing()
                                     || ( playback_control::get()->is_paused() && config::g_disableWhenPaused ) );
    if ( needsToBeDisabled )
    {
        if ( parent_.HasPresence() )
        {
            parent_.ClearPresence();
        }
    }
    else
    {
        if ( !parent_.HasPresence() || hasChanged )
        {
            parent_.SendPresense();
        }
    }
}

void PresenceModifier::UpdateImage()
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();

    auto setImageKey = [&pd]( const std::u8string& imageKey ) {
        pd.largeImageKey = imageKey;
        pd.presence.largeImageKey = pd.largeImageKey.empty() ? nullptr : pd.largeImageKey.c_str();
    };

    switch ( config::g_largeImageSettings )
    {
    case config::ImageSetting::Light:
    {
        setImageKey( config::g_largeImageId_Light );
        break;
    }
    case config::ImageSetting::Dark:
    {
        setImageKey( config::g_largeImageId_Dark );
        break;
    }
    case config::ImageSetting::Disabled:
    {
        setImageKey( std::u8string{} );
        break;
    }
    }
}

void PresenceModifier::UpdateSmallImage()
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();

    auto setImageKey = [&pd]( const std::u8string& imageKey ) {
        pd.smallImageKey = imageKey;
        pd.presence.smallImageKey = pd.smallImageKey.empty() ? nullptr : pd.smallImageKey.c_str();
    };

    const bool usePausedImage = ( pc->is_paused() || config::g_swapSmallImages );

    switch ( config::g_smallImageSettings )
    {
    case config::ImageSetting::Light:
    {
        setImageKey( usePausedImage ? config::g_pausedImageId_Light : config::g_playingImageId_Light );
        break;
    }
    case config::ImageSetting::Dark:
    {
        setImageKey( usePausedImage ? config::g_pausedImageId_Dark : config::g_playingImageId_Dark );
        break;
    }
    case config::ImageSetting::Disabled:
    {
        setImageKey( std::u8string{} );
        break;
    }
    }
}

void PresenceModifier::UpdateTrack( metadb_handle_ptr metadb )
{
    auto& pd = presenceData_;

    pd.state.clear();
    pd.details.clear();
    pd.trackLength = 0;

    if ( metadb.is_valid() )
    { // Need to save, since refresh might be required when settings are changed
        pd.metadb = metadb;
    }

    auto pc = playback_control::get();
    const auto queryData = [&pc, metadb = pd.metadb]( const std::u8string& query ) -> std::u8string {
        titleformat_object::ptr tf;
        titleformat_compiler::get()->compile_safe( tf, query.c_str() );
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

        return result.c_str();
    };
    const auto fixStringLength = []( std::u8string& str ) {
        if ( str.length() == 1 )
        { // minimum allowed non-zero string length is 2, so we need to pad it
            str += ' ';
        }
        else if ( str.length() > 127 )
        { // maximum allowed length is 127
            str.resize( 127 );
        }
    };

    pd.state = queryData( config::g_stateQuery );
    fixStringLength( pd.state );
    pd.details = queryData( config::g_detailsQuery );
    fixStringLength( pd.details );

    const std::u8string lengthStr = queryData( "[%length_seconds_fp%]" );
    pd.trackLength = ( lengthStr.empty() ? 0 : stold( lengthStr ) );

    const std::u8string durationStr = queryData( "[%playback_time_seconds%]" );

    pd.presence.state = pd.state.c_str();
    pd.presence.details = pd.details.c_str();
    UpdateDuration( durationStr.empty() ? 0 : stold( durationStr ) );
}

void PresenceModifier::UpdateDuration( double time )
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();
    const config::TimeSetting timeSetting = ( ( pd.trackLength && pc->is_playing() && !pc->is_paused() )
                                                  ? config::g_timeSettings
                                                  : config::TimeSetting::Disabled );
    switch ( timeSetting )
    {
    case config::TimeSetting::Elapsed:
    {
        pd.presence.startTimestamp = std::time( nullptr ) - std::llround( time );
        pd.presence.endTimestamp = 0;

        break;
    }
    case config::TimeSetting::Remaining:
    {
        pd.presence.startTimestamp = 0;
        pd.presence.endTimestamp = std::time( nullptr ) + std::max<uint64_t>( 0, std::llround( pd.trackLength - time ) );

        break;
    }
    case config::TimeSetting::Disabled:
    {
        pd.presence.startTimestamp = 0;
        pd.presence.endTimestamp = 0;

        break;
    }
    }
}

void PresenceModifier::DisableDuration()
{
    auto& pd = presenceData_;
    pd.presence.startTimestamp = 0;
    pd.presence.endTimestamp = 0;
}

void PresenceModifier::Disable()
{
    isDisabled_ = true;
}

DiscordHandler& DiscordHandler::GetInstance()
{
    static DiscordHandler discordHandler;
    return discordHandler;
}

void DiscordHandler::Initialize()
{
    appToken_ = config::g_discordAppToken;

    DiscordEventHandlers handlers;
    memset( &handlers, 0, sizeof( handlers ) );

    handlers.ready = OnReady;
    handlers.disconnected = OnDisconnected;
    handlers.errored = OnErrored;

    Discord_Initialize( appToken_.c_str(), &handlers, 1, nullptr );
    Discord_RunCallbacks();

    hasPresence_ = true; ///< Discord may use default app handler, which we need to override

    auto pm = GetPresenceModifier();
    pm.UpdateImage();
    pm.Disable(); ///< we don't want to activate presence yet
}

void DiscordHandler::Finalize()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordHandler::OnSettingsChanged()
{
    if ( appToken_ != static_cast<std::string>( config::g_discordAppToken ) )
    {
        Finalize();
        Initialize();
    }

    auto pm = GetPresenceModifier();
    pm.UpdateImage();
    pm.UpdateSmallImage();
    pm.UpdateTrack();
    if ( !config::g_isEnabled )
    {
        pm.Disable();
    }
}

bool DiscordHandler::HasPresence() const
{
    return hasPresence_;
}

void DiscordHandler::SendPresense()
{
    if ( config::g_isEnabled )
    {
        Discord_UpdatePresence( &presenceData_.presence );
        hasPresence_ = true;
    }
    else
    {
        Discord_ClearPresence();
        hasPresence_ = false;
    }
    Discord_RunCallbacks();
}

void DiscordHandler::ClearPresence()
{
    Discord_ClearPresence();
    hasPresence_ = false;

    Discord_RunCallbacks();
}

PresenceModifier DiscordHandler::GetPresenceModifier()
{
    return PresenceModifier( *this, presenceData_ );
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

} // namespace drp
