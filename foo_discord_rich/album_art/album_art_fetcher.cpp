#include <stdafx.h>

#include "album_art_fetcher.h"

#include <discord/discord_integration.h>
#include <fb2k/current_track_titleformat.h>

#include <component_paths.h>

#include <cpr/cpr.h>
#include <qwr/algorithm.h>
#include <qwr/file_helpers.h>
#include <qwr/thread_name_setter.h>

namespace fs = std::filesystem;

namespace
{

const std::chrono::seconds kRequestProcessingDelay{ 2 };

}

namespace
{

const fs::path& GetCacheFilePath()
{
    static const auto cachePath = drp::path::ImageDir() / "album_art_urls.json";
    return cachePath;
}

qwr::u8string GenerateAlbumId( const qwr::u8string& artist, const qwr::u8string& album )
{
    return artist + "|" + album;
}

/// @throw qwr::qwrException
std::optional<qwr::u8string> FetchReleaseMbid( const qwr::u8string& album, const qwr::u8string& artist )
{
    using json = nlohmann::json;

    auto releaseGroupResp = cpr::Get(
        cpr::Url{ "https://www.musicbrainz.org/ws/2/release-group" },
        cpr::Parameters{
            { "query", fmt::format( "artist:\"{}\"+releasegroup:\"{}\"", artist, album ) },
            { "fmt", "json" },
        }
    );
    if ( releaseGroupResp.status_code == 404 )
    {
        return std::nullopt;
    }
    else if ( releaseGroupResp.status_code != 200 )
    {
        throw qwr::QwrException( "Failed to fetch MB release group\nCode: {}\nError: {}", releaseGroupResp.status_code, releaseGroupResp.reason );
    }

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
                cpr::Header{ { "Accept", "application/json" } }
            );
            if ( releaseResp.status_code != 200 )
            {
                throw qwr::QwrException( "Failed to fetch MB release\nCode: {}\nError: {}", releaseResp.status_code, releaseResp.reason );
            }

            auto jRelease = nlohmann::json::parse( releaseResp.text );
            if ( jRelease.at( "cover-art-archive" ).at( "artwork" ).get<bool>() )
            {
                return releaseId;
            }
        }

        return std::nullopt;
    }
    catch ( const nlohmann::json::parse_error& e )
    {
        throw qwr::QwrException( "Failed to parse musicbrainz response: {}", e.what() );
    }
}

/// @throw qwr::qwrException
std::optional<qwr::u8string> FetchAlbumArtUrl( const qwr::u8string& mbid )
{
    auto resp = cpr::Get(
        cpr::Url{ fmt::format( "http://coverartarchive.org/release/{}/front-1200", mbid ) }
    );
    if ( resp.status_code == 404 )
    {
        return std::nullopt;
    }
    else if ( resp.status_code != 200 )
    {
        throw qwr::QwrException( "Failed to fetch album art url\nCode: {}\nError: {}", resp.status_code, resp.reason );
    }

    return resp.url.str();
}

} // namespace

namespace drp
{

drp::AlbumArtFetcher& AlbumArtFetcher::Get()
{
    static AlbumArtFetcher instance;
    return instance;
}

void AlbumArtFetcher::Initialize()
{
    LoadCache();
    StartThread();
}

void AlbumArtFetcher::Finalize()
{
    StopThread();
    SaveCache();
}

std::optional<qwr::u8string> AlbumArtFetcher::GetArtUrl( const FetchRequest& request )
{
    const auto& [artist, album, userReleaseMbidOpt] = request;
    if ( artist.empty() || album.empty() )
    {
        return std::nullopt;
    }

    if ( const auto artUrlOpt = qwr::FindOrDefault( albumIdToArtUrl_, GenerateAlbumId( artist, album ), std::nullopt );
         artUrlOpt )
    {
        return artUrlOpt;
    }

    {
        std::unique_lock lock( mutex_ );

        currentRequestOpt_ = request;
        cv_.notify_all();
    }

    return std::nullopt;
}

void AlbumArtFetcher::LoadCache()
{
    using json = nlohmann::json;

    try
    {
        const auto cachePath = GetCacheFilePath();
        if ( !fs::exists( cachePath ) )
        {
            return;
        }

        const auto content = qwr::file::ReadFile( cachePath, CP_UTF8 );
        json::parse( content ).get_to( albumIdToArtUrl_ );
    }
    catch ( const qwr::QwrException& e )
    {
        LogError( fmt::format( "Failed to load cache: {}", e.what() ) );
    }
    catch ( const json::exception& e )
    {
        LogError( fmt::format( "Failed to load cache: {}", e.what() ) );
    }
    catch ( const fs::filesystem_error& e )
    {
        LogError( fmt::format( "Failed to load cache: {}", e.what() ) );
    }
}

void AlbumArtFetcher::SaveCache()
{
    using json = nlohmann::json;

    try
    {
        const auto cachePath = GetCacheFilePath();
        fs::create_directories( cachePath.parent_path() );

        const auto content = json( albumIdToArtUrl_ ).dump( 2 );
        qwr::file::WriteFile( cachePath, content, false );
    }
    catch ( const qwr::QwrException& e )
    {
        LogError( fmt::format( "Failed to save cache: {}", e.what() ) );
    }
    catch ( const json::exception& e )
    {
        LogError( fmt::format( "Failed to save cache: {}", e.what() ) );
    }
    catch ( const fs::filesystem_error& e )
    {
        LogError( fmt::format( "Failed to save cache: {}", e.what() ) );
    }
}

void AlbumArtFetcher::StartThread()
{
    pThread_ = std::make_unique<std::jthread>( &AlbumArtFetcher::ThreadMain, this );
    qwr::SetThreadName( *pThread_, "DRP AlbumArtFetcher" );
}

void AlbumArtFetcher::StopThread()
{
    if ( pThread_ )
    {
        pThread_->request_stop();
        pThread_.reset();
    }
}

void AlbumArtFetcher::ThreadMain()
{
    auto token = pThread_->get_stop_token();
    std::optional<FetchRequest> lastRequest;

    while ( !token.stop_requested() )
    {
        std::optional<qwr::u8string> mbidOpt;
        {
            std::unique_lock lock( mutex_ );
            cv_.wait_for( lock, kRequestProcessingDelay, [&] {
                return token.stop_requested() || lastRequest != currentRequestOpt_;
            } );

            if ( currentRequestOpt_ )
            {
                if ( const auto albumId = GenerateAlbumId( currentRequestOpt_->artist, currentRequestOpt_->album );
                     albumIdToArtUrl_.contains( albumId ) )
                {
                    currentRequestOpt_.reset();
                    lastRequest.reset();
                    continue;
                }
            }

            if ( lastRequest != currentRequestOpt_ )
            { // user requested new art, wait again
                lastRequest = currentRequestOpt_;
                continue;
            }

            if ( !lastRequest )
            {
                continue;
            }
        }

        const auto artUrlOpt = ProcessFetchRequest( *lastRequest );

        {
            std::unique_lock lock( mutex_ );

            const auto albumId = GenerateAlbumId( currentRequestOpt_->artist, currentRequestOpt_->album );
            albumIdToArtUrl_.try_emplace( albumId, artUrlOpt );

            if ( lastRequest == currentRequestOpt_ )
            {
                currentRequestOpt_.reset();
                lastRequest.reset();

                if ( artUrlOpt )
                {
                    fb2k::inMainThread( [] {
                        auto pm = DiscordAdapter::GetInstance().GetPresenceModifier();
                        pm.UpdateImage();
                    } );
                }
            }
        }
    }
}

std::optional<qwr::u8string> AlbumArtFetcher::ProcessFetchRequest( const FetchRequest& request )
{
    try
    {
        if ( request.userReleaseMbidOpt )
        {
            const auto urlOpt = FetchAlbumArtUrl( *request.userReleaseMbidOpt );
            return urlOpt;
        }

        const auto releaseMbidOpt = FetchReleaseMbid( request.album, request.artist );
        if ( !releaseMbidOpt )
        {
            return std::nullopt;
        }

        return FetchAlbumArtUrl( *releaseMbidOpt );
    }
    catch ( const qwr::QwrException& e )
    {
        LogError( e.what() );
        return std::nullopt;
    }
}

} // namespace drp
