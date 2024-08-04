#include <stdafx.h>

#include "presence_data.h"

#include <album_art/album_art_fetcher.h>
#include <discord/discord_integration.h>
#include <fb2k/config.h>

#include <qwr/algorithm.h>

namespace
{

qwr::u8string EvaluateQueryForPlayingTrack( const metadb_handle_ptr& handle, const qwr::u8string& query )
{
    static std::unordered_map<qwr::u8string, titleformat_object::ptr> queryToTitleFormat;

    auto pc = playback_control::get();
    auto tf = qwr::FindOrDefault( queryToTitleFormat, query, titleformat_object::ptr{} );
    if ( tf.is_empty() )
    {
        titleformat_compiler::get()->compile_safe( tf, query.c_str() );
        queryToTitleFormat.try_emplace( query, tf );
    }

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

} // namespace

namespace drp::internal
{

PresenceData::PresenceData()
{
    memset( &presence, 0, sizeof( presence ) );
    presence.activityType = DiscordActivityType::LISTENING;
    UpdateTextFieldPointers();
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

    return areStringsSame( presence.state, other.presence.state )
           && areStringsSame( presence.details, other.presence.details )
           && areStringsSame( presence.largeImageKey, other.presence.largeImageKey )
           && areStringsSame( presence.largeImageText, other.presence.largeImageText )
           && areStringsSame( presence.smallImageKey, other.presence.smallImageKey )
           && areStringsSame( presence.smallImageText, other.presence.smallImageText )
           && presence.startTimestamp == other.presence.startTimestamp
           && presence.endTimestamp == other.presence.endTimestamp
           && trackLength == other.trackLength;
}

bool PresenceData::operator!=( const PresenceData& other )
{
    return !operator==( other );
}

void PresenceData::CopyData( const PresenceData& other )
{
    metadb = other.metadb;
    topText = other.topText;
    middleText = other.middleText;
    bottomText = other.bottomText;
    largeImageKey = other.largeImageKey;
    smallImageKey = other.smallImageKey;
    trackLength = other.trackLength;

    memcpy( &presence, &other.presence, sizeof( presence ) );
    UpdateTextFieldPointers();
    presence.activityType = DiscordActivityType::LISTENING;
    presence.largeImageKey = ( largeImageKey.empty() ? nullptr : largeImageKey.c_str() );
    presence.smallImageKey = ( smallImageKey.empty() ? nullptr : smallImageKey.c_str() );
}

void PresenceData::UpdateTextFieldPointers()
{
    presence.details = topText.c_str();
    presence.state = middleText.c_str();
    presence.largeImageText = bottomText.c_str();
}

} // namespace drp::internal

namespace drp
{

PresenceModifier::PresenceModifier( DiscordAdapter& parent, const drp::internal::PresenceData& presenceData )
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

    const bool needsToBeDisabled = ( isDisabled_ || !playback_control::get()->is_playing() || ( playback_control::get()->is_paused() && config::disableWhenPaused ) );
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
            parent_.SendPresence();
        }
    }
}

void PresenceModifier::UpdateImage( metadb_handle_ptr metadb )
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();

    auto setImageKey = [&pd]( const qwr::u8string& imageKey ) {
        pd.largeImageKey = imageKey;
        pd.presence.largeImageKey = pd.largeImageKey.empty() ? nullptr : pd.largeImageKey.c_str();
    };

    const auto artUrlOpt = [&]() -> std::optional<qwr::u8string> {
        if ( !config::fetchAlbumArt )
        {
            return std::nullopt;
        }

        const auto userReleaseMbid = EvaluateQueryForPlayingTrack( metadb, "$if3($meta(MUSICBRAINZ_ALBUMID),$meta(MUSICBRAINZ ALBUM ID))" );
        const AlbumArtFetcher::FetchRequest request{
            .artist = EvaluateQueryForPlayingTrack( metadb, "%artist%" ),
            .album = EvaluateQueryForPlayingTrack( metadb, "%album%" ),
            .userReleaseMbidOpt = userReleaseMbid.empty() ? std::optional<qwr::u8string>{} : userReleaseMbid
        };

        return AlbumArtFetcher::Get().GetArtUrl( request );
    }();
    if ( artUrlOpt )
    {
        setImageKey( *artUrlOpt );
    }
    else
    {
        switch ( config::largeImageSettings )
        {
        case config::ImageSetting::Light:
        {
            setImageKey( config::largeImageId_Light );
            break;
        }
        case config::ImageSetting::Dark:
        {
            setImageKey( config::largeImageId_Dark );
            break;
        }
        case config::ImageSetting::Disabled:
        {
            setImageKey( qwr::u8string{} );
            break;
        }
        }
    }
}

void PresenceModifier::UpdateSmallImage()
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();

    auto setImageKey = [&pd]( const qwr::u8string& imageKey ) {
        pd.smallImageKey = imageKey;
        pd.presence.smallImageKey = pd.smallImageKey.empty() ? nullptr : pd.smallImageKey.c_str();
    };

    const bool usePausedImage = ( pc->is_paused() || config::swapSmallImages );

    switch ( config::smallImageSettings )
    {
    case config::ImageSetting::Light:
    {
        setImageKey( usePausedImage ? config::pausedImageId_Light : config::playingImageId_Light );
        break;
    }
    case config::ImageSetting::Dark:
    {
        setImageKey( usePausedImage ? config::pausedImageId_Dark : config::playingImageId_Dark );
        break;
    }
    case config::ImageSetting::Disabled:
    {
        setImageKey( qwr::u8string{} );
        break;
    }
    }
}

/**
 * https://stackoverflow.com/a/59691895
 * Calculate the number of characters in a utf-8 string.
 *  Some composite characters that are composed of multiple different characters, such as some emojis,
 *  are counted as multiple characters.
 */
size_t count_codepoints( const qwr::u8string& str )
{
    size_t count = 0;
    for ( auto& c: str )
        if ( ( c & 0b1100'0000 ) != 0b1000'0000 ) // Not a trailing byte
            ++count;
    return count;
}

void PresenceModifier::UpdateTrack( metadb_handle_ptr metadb )
{
    auto& pd = presenceData_;

    pd.topText.clear();
    pd.middleText.clear();
    pd.bottomText.clear();
    pd.trackLength = 0;

    if ( metadb.is_valid() )
    { // Need to save, since refresh might be required when settings are changed
        pd.metadb = metadb;
    }

    const auto queryData = [metadb = pd.metadb]( const qwr::u8string& query ) {
        return EvaluateQueryForPlayingTrack( metadb, query );
    };
    const auto fixStringLength = []( qwr::u8string& str ) {
        // Required for correct calculation of utf-8 string length
        if ( count_codepoints( str ) == 1 )
        { // minimum allowed non-zero string length is 2, so we need to pad it
            str += ' ';
        }
        else if ( str.length() > 127 )
        { // maximum allowed length is 127
            str.resize( 124 );
            str += "...";
        }
    };

    pd.topText = queryData( config::topTextQuery );
    fixStringLength( pd.topText );
    pd.middleText = queryData( config::middleTextQuery );
    fixStringLength( pd.middleText );
    pd.bottomText = queryData( config::bottomTextQuery );
    fixStringLength( pd.bottomText );
    pd.UpdateTextFieldPointers();

    const qwr::u8string lengthStr = queryData( "[%length_seconds_fp%]" );
    const qwr::u8string durationStr = queryData( "[%playback_time_seconds%]" );
    UpdateDuration( durationStr.empty() ? 0 : stold( durationStr ), lengthStr.empty() ? 0 : stold( lengthStr ) );

    UpdateImage( metadb );
}

void PresenceModifier::UpdateDuration( double currentTime )
{
    auto& pd = presenceData_;
    auto pc = playback_control::get();
    const config::TimeSetting timeSetting = ( ( pd.trackLength && pc->is_playing() && !pc->is_paused() ) ? config::timeSettings : config::TimeSetting::Disabled );
    switch ( timeSetting )
    {
    case config::TimeSetting::Elapsed:
    {
        pd.presence.startTimestamp = std::time( nullptr ) - std::llround( currentTime );
        pd.presence.endTimestamp = 0;

        break;
    }
    case config::TimeSetting::Remaining:
    {
        pd.presence.startTimestamp = 0;
        pd.presence.endTimestamp = std::time( nullptr ) + std::max<uint64_t>( 0, std::llround( pd.trackLength - currentTime ) );

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

void PresenceModifier::UpdateDuration( double currentTime, double totalLength )
{
    auto& pd = presenceData_;
    pd.trackLength = totalLength;
    UpdateDuration( currentTime );
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

} // namespace drp
