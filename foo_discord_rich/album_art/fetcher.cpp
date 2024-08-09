#include <stdafx.h>

#include "fetcher.h"

#include <album_art/musicbrainz_fetcher.h>
#include <album_art/uploader.h>
#include <discord/discord_integration.h>

#include <component_paths.h>

#include <cpr/cpr.h>
#include <qwr/algorithm.h>
#include <qwr/file_helpers.h>
#include <qwr/thread_name_setter.h>
#include <qwr/visitor.h>

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

qwr::u8string GenerateMusicBrainzArtPinId( const qwr::u8string& artist, const qwr::u8string& album )
{
    return artist + "|" + album;
}

std::optional<qwr::u8string> GenerateArtPinId( const drp::AlbumArtFetcher::FetchRequest& request )
{
    const auto artPinId = std::visit(
        qwr::Visitor{
            []( const drp::AlbumArtFetcher::MusicBrainzFetchRequest& req ) {
                const auto& [artist, album, userReleaseMbidOpt] = req;
                if ( artist.empty() || album.empty() )
                {
                    return qwr::u8string{};
                }
                return GenerateMusicBrainzArtPinId( req.artist, req.album );
            },
            []( const drp::AlbumArtFetcher::UploadRequest& req ) {
                if ( req.uploaderPath.empty() )
                {
                    return qwr::u8string{};
                }
                return req.artPinId;
            } },
        request );
    return ( artPinId.empty() ? std::optional<qwr::u8string>{} : artPinId );
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
    const auto artPinIdOpt = GenerateArtPinId( request );
    if ( !artPinIdOpt )
    {
        return std::nullopt;
    }

    if ( const auto artUrlOpt = qwr::FindOrDefault( artPinIdToArtUrl_, *artPinIdOpt, std::nullopt );
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
        json::parse( content ).get_to( artPinIdToArtUrl_ );
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

        const auto content = json( artPinIdToArtUrl_ ).dump( 2 );
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
                if ( const auto artPinIdOpt = GenerateArtPinId( *currentRequestOpt_ );
                     artPinIdOpt && artPinIdToArtUrl_.contains( *artPinIdOpt ) )
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

        auto artUrlOpt = std::visit( [&]( const auto& arg ) { return ProcessFetchRequest( arg ); }, *lastRequest );
        if ( !artUrlOpt && ( fb2k::mainAborter().is_aborting() || token.stop_requested() ) )
        { // do not save nullopt if interrupted, because it might actually had the image
            return;
        }

        if ( artUrlOpt && qwr::unicode::ToWide( *artUrlOpt ).length() > 255 )
        { // Discord API max image key size
            LogError( fmt::format( "Failed to process art url `{}`:\nlength is bigger than 255", *artUrlOpt ) );
            artUrlOpt.reset();
        }

        {
            std::unique_lock lock( mutex_ );

            const auto artPinIdOpt = GenerateArtPinId( *currentRequestOpt_ );
            assert( artPinIdOpt );

            artPinIdToArtUrl_.try_emplace( *artPinIdOpt, artUrlOpt );

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

std::optional<qwr::u8string> AlbumArtFetcher::ProcessFetchRequest( const MusicBrainzFetchRequest& request )
{
    try
    {
        return musicbrainz::FetchArt( request.artist, request.album, request.userReleaseMbidOpt );
    }
    catch ( const qwr::QwrException& e )
    {
        LogError( e.what() );
        return std::nullopt;
    }
    catch ( const exception_aborted& /*e*/ )
    {
        return std::nullopt;
    }
}

std::optional<qwr::u8string> AlbumArtFetcher::ProcessFetchRequest( const UploadRequest& request )
{
    try
    {
        return UploadArt( request.handle, request.uploaderPath );
    }
    catch ( const qwr::QwrException& e )
    {
        LogError( e.what() );
        return std::nullopt;
    }
    catch ( const exception_aborted& /*e*/ )
    {
        return std::nullopt;
    }
}

} // namespace drp
