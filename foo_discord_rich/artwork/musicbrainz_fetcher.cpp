#include <stdafx.h>

#include "musicbrainz_fetcher.h"

#include <cpr/cpr.h>
#include <qwr/abort_callback.h>

namespace
{

using namespace drp;

bool IsValidGuid( const qwr::u8string& str )
{
    GUID guid;
    HRESULT hr = IIDFromString( qwr::unicode::ToWide( fmt::format( "{{{}}}", str ) ).c_str(), &guid );
    return SUCCEEDED( hr );
}

void LogRequest( const cpr::Response& resp )
{
    if ( config::advanced::logWebRequests )
    {
        LogDebug( "Request: {}", resp.url.str() );
    }
    if ( config::advanced::logWebResponses )
    {
        LogDebug(
            "Response:\n"
            "  Code: {}\n"
            "  Body:\n"
            "{}\n",
            resp.status_code,
            resp.text );
    }
}

/// @throw qwr::QwrException
/// @throw exception_aborted
std::optional<qwr::u8string> FetchReleaseMbid( const qwr::u8string& artist, const qwr::u8string& album, abort_callback& aborter )
{
    using json = nlohmann::json;

    auto releaseGroupResp = cpr::Get(
        cpr::Url{ "https://www.musicbrainz.org/ws/2/release-group" },
        cpr::Parameters{
            { "query", fmt::format( "artist:\"{}\"+releasegroup:\"{}\"", artist, album ) },
            { "fmt", "json" },
        } );
    LogRequest( releaseGroupResp );
    if ( releaseGroupResp.status_code == 404 )
    {
        return std::nullopt;
    }
    if ( releaseGroupResp.status_code != 200 )
    {
        throw qwr::QwrException( "Failed to fetch MB release group\nCode: {}\nError: {}", releaseGroupResp.status_code, releaseGroupResp.reason );
    }

    aborter.check();

    try
    {
        auto j = nlohmann::json::parse( releaseGroupResp.text );
        const auto& releaseGroups = j.at( "release-groups" );
        if ( releaseGroups.empty() )
        {
            return std::nullopt;
        }

        for ( const auto& release: releaseGroups.front().at( "releases" ) )
        {
            const auto releaseId = release.at( "id" ).get<qwr::u8string>();
            auto releaseResp = cpr::Get(
                cpr::Url{ fmt::format( "https://www.musicbrainz.org/ws/2/release/{}", releaseId ) },
                cpr::Header{ { "Accept", "application/json" } } );
            LogRequest( releaseResp );
            if ( releaseResp.status_code != 200 )
            {
                throw qwr::QwrException( "Failed to fetch MB release\nCode: {}\nError: {}", releaseResp.status_code, releaseResp.reason );
            }

            auto jRelease = nlohmann::json::parse( releaseResp.text );
            if ( jRelease.at( "cover-art-archive" ).at( "artwork" ).get<bool>() )
            {
                return releaseId;
            }

            aborter.check();
        }

        return std::nullopt;
    }
    catch ( const nlohmann::json::parse_error& e )
    {
        throw qwr::QwrException( "Failed to parse musicbrainz response: {}", e.what() );
    }
}

/// @throw qwr::QwrException
std::optional<qwr::u8string> FetchAlbumArtUrl( const qwr::u8string& mbid )
{
    auto resp = cpr::Get(
        cpr::Url{ fmt::format( "http://coverartarchive.org/release/{}/front-1200", mbid ) } );
    LogRequest( resp );
    if ( resp.status_code == 404 )
    {
        return std::nullopt;
    }
    if ( resp.status_code != 200 )
    {
        throw qwr::QwrException( "Failed to fetch album art url\nCode: {}\nError: {}", resp.status_code, resp.reason );
    }

    return resp.url.str();
}
} // namespace

namespace drp::musicbrainz
{

std::optional<qwr::u8string> FetchArt( const qwr::u8string& artist, const qwr::u8string& album, const std::optional<qwr::u8string>& userReleaseMbidOpt )
{
    auto& aborter = qwr::GlobalAbortCallback::GetInstance();

    if ( userReleaseMbidOpt )
    {
        if ( !IsValidGuid( *userReleaseMbidOpt ) )
        {
            LogWarning( fmt::format( "Invalid MBID detected: `{}`. Skipping...", *userReleaseMbidOpt ) );
        }
        else
        {
            const auto urlOpt = FetchAlbumArtUrl( *userReleaseMbidOpt );
            return urlOpt;
        }
    }

    aborter.check();

    const auto releaseMbidOpt = FetchReleaseMbid( artist, album, aborter );
    if ( !releaseMbidOpt )
    {
        return std::nullopt;
    }

    aborter.check();

    return FetchAlbumArtUrl( *releaseMbidOpt );
}

} // namespace drp::musicbrainz
