#include <stdafx.h>

#include "album_art_fetcher.h"

#include <fb2k/current_track_titleformat.h>

#include <cpr/cpr.h>
#include <qwr/algorithm.h>
#include <qwr/thread_name_setter.h>

namespace
{

/// @throw qwr::qwrException
std::optional<qwr::u8string> FetchMbid( const qwr::u8string& artist, const qwr::u8string& album )
{
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
    // TODO: add cache load
    StartThread();
}

void AlbumArtFetcher::Finalize()
{
    // TODO: add cache save
    StopThread();
}

std::optional<qwr::u8string> AlbumArtFetcher::GetArtUrl( const metadb_handle_ptr& handle )
{
    const auto artist = EvaluateQueryForCurrentTrack( handle, "%artist%" );
    const auto album = EvaluateQueryForCurrentTrack( handle, "%album%" );
    if ( artist.empty() || album.empty() )
    {
        return std::nullopt;
    }

    TrackInfo trackInfo{ artist, album };
    const auto trackId = trackInfo.GenerateId();

    // TODO: verify syntax and tag name
    const auto tagMbid = EvaluateQueryForCurrentTrack( handle, "$meta(MUSICBRAINZ ALBUM ID)" );

    std::unique_lock lock( mutex_ );

    qwr::u8string mbid;
    if ( !tagMbid.empty() )
    {
        mbid = tagMbid;
    }
    else
    {
        const auto mbidOptOpt = qwr::FindAsOptional( trackIdToMbid_, trackId );
        if ( !mbidOptOpt )
        {
            // TODO: move to method
            // TODO: use user's mbid if present when queuing
            currentRequestedTrackOpt_ = trackInfo;
            return std::nullopt;
        }
        if ( !( *mbidOptOpt ) )
        {
            return std::nullopt;
        }

        mbid = **mbidOptOpt;
    }

    return qwr::FindOrDefault( mbidToUrl_, mbid, std::nullopt );
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
    std::optional<TrackInfo> lastRequestedTrackOpt;

    while ( !token.stop_requested() )
    {
        std::optional<qwr::u8string> mbidOpt;
        {
            std::unique_lock lock( mutex_ );
            // TODO: verify that it only waits for 2 seconds and not double the time
            cv_.wait_for( lock, std::chrono::seconds( 2 ), [&] {
                return token.stop_requested() || lastRequestedTrackOpt != currentRequestedTrackOpt_;
            } );

            if ( currentRequestedTrackOpt_ )
            {
                mbidOpt = qwr::FindOrDefault( trackIdToMbid_, currentRequestedTrackOpt_->GenerateId(), std::nullopt );
                if ( mbidOpt && qwr::FindOrDefault( mbidToUrl_, *mbidOpt, std::nullopt ) )
                {
                    currentRequestedTrackOpt_.reset();
                    lastRequestedTrackOpt.reset();
                    continue;
                }
            }

            if ( lastRequestedTrackOpt != currentRequestedTrackOpt_ )
            { // user requested new art, wait again
                lastRequestedTrackOpt = currentRequestedTrackOpt_;
                continue;
            }

            if ( !lastRequestedTrackOpt )
            {
                continue;
            }
        }

        // TODO: move to a separate method
        const auto& trackInfo = *lastRequestedTrackOpt;
        const auto trackId = trackInfo.GenerateId();
        try
        {
            if ( !mbidOpt )
            {
                mbidOpt = FetchMbid( trackInfo.artist, trackInfo.album );
            }
            {
                std::unique_lock lock( mutex_ );
                if ( !mbidOpt )
                {
                    trackIdToMbid_.try_emplace( trackId, std::nullopt );
                    continue;
                }

                trackIdToMbid_.try_emplace( trackId, *mbidOpt );
            }

            const auto& mbid = *mbidOpt;
            const auto artUrlOpt = FetchAlbumArtUrl( mbid );
            {
                std::unique_lock lock( mutex_ );
                if ( !artUrlOpt )
                {
                    mbidToUrl_.try_emplace( mbid, std::nullopt );
                    continue;
                }

                // TODO: trigger image refresh
                mbidToUrl_.try_emplace( mbid, *artUrlOpt );
            }
        }
        catch ( const qwr::QwrException& /* e */ )
        {
            // TODO: log error
            std::unique_lock lock( mutex_ );
            // TODO: fix this after handling user MBID
            trackIdToMbid_.try_emplace( trackId, std::nullopt );
        }
    }
}

qwr::u8string AlbumArtFetcher::TrackInfo::GenerateId() const
{
    return artist + "|" + album;
}

} // namespace drp
