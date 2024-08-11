#include <stdafx.h>

#include "fetcher.h"

#include <artwork/musicbrainz_fetcher.h>
#include <artwork/uploader.h>
#include <discord/discord_integration.h>

#include <component_paths.h>

#include <cpr/cpr.h>
#include <qwr/abort_callback.h>
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

qwr::u8string GenerateMusicBrainzArtPinId( const qwr::u8string& artist, const qwr::u8string& album )
{
    return artist + "|" + album;
}

std::optional<qwr::u8string> GenerateArtPinId( const drp::ArtworkFetcher::FetchRequest& request )
{
    const auto artPinId = std::visit(
        qwr::Visitor{
            []( const drp::ArtworkFetcher::MusicBrainzFetchRequest& req ) {
                const auto& [artist, album, userReleaseMbidOpt] = req;
                if ( artist.empty() || album.empty() )
                {
                    return qwr::u8string{};
                }
                return GenerateMusicBrainzArtPinId( req.artist, req.album );
            },
            []( const drp::ArtworkFetcher::UploadRequest& req ) {
                return req.artPinId;
            } },
        request );
    return ( artPinId.empty() ? std::optional<qwr::u8string>{} : artPinId );
}

bool IsRequestExecutable( const drp::ArtworkFetcher::FetchRequest& request )
{
    return std::visit(
        qwr::Visitor{
            []( const drp::ArtworkFetcher::MusicBrainzFetchRequest& req ) {
                return true;
            },
            []( const drp::ArtworkFetcher::UploadRequest& req ) {
                return !req.uploadCommand.empty();
            } },
        request );
}

} // namespace

namespace drp
{

drp::ArtworkFetcher& ArtworkFetcher::Get()
{
    static ArtworkFetcher instance;
    return instance;
}

void ArtworkFetcher::Initialize()
{
    LoadCache();
    StartThread();
}

void ArtworkFetcher::Finalize()
{
    StopThread();
    SaveCache();
}

std::optional<qwr::u8string> ArtworkFetcher::GetArtUrl( const FetchRequest& request )
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

    if ( !IsRequestExecutable( request ) )
    {
        return std::nullopt;
    }

    {
        std::unique_lock lock( mutex_ );

        currentRequestOpt_ = request;
        cv_.notify_all();
    }

    return std::nullopt;
}

void ArtworkFetcher::LoadCache( bool throwOnError )
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
        if ( throwOnError )
        {
            throw;
        }
    }
    catch ( const json::exception& e )
    {
        LogError( fmt::format( "Failed to load cache: {}", e.what() ) );
        if ( throwOnError )
        {
            throw;
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        LogError( fmt::format( "Failed to load cache: {}", e.what() ) );
        if ( throwOnError )
        {
            throw;
        }
    }
}

void ArtworkFetcher::SaveCache()
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

std::filesystem::path ArtworkFetcher::GetCacheFilePath()
{
    static const auto cachePath = drp::path::ImageDir() / "art_urls.v2.0.1.json";
    return cachePath;
}

void ArtworkFetcher::StartThread()
{
    pThread_ = std::make_unique<std::jthread>( &ArtworkFetcher::ThreadMain, this );
    qwr::SetThreadName( *pThread_, "DRP ArtFetcher" );
}

void ArtworkFetcher::StopThread()
{
    if ( pThread_ )
    {
        pThread_->request_stop();
        pThread_.reset();
    }
}

void ArtworkFetcher::ThreadMain()
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
        if ( !artUrlOpt && ( qwr::GlobalAbortCallback::GetInstance().is_aborting() || token.stop_requested() ) )
        { // do not save nullopt if interrupted, because it might actually had the image
            return;
        }

        if ( artUrlOpt && qwr::unicode::ToWide( *artUrlOpt ).length() > 254 )
        { // Discord API max image key size
            LogError( fmt::format( "Failed to process art url `{}`:\nlength is bigger than 254", *artUrlOpt ) );
            artUrlOpt.reset();
        }

        const auto artPinIdOpt = GenerateArtPinId( *lastRequest );
        assert( artPinIdOpt );
        {
            std::unique_lock lock( mutex_ );

            artPinIdToArtUrl_.try_emplace( *artPinIdOpt, artUrlOpt );

            if ( lastRequest == currentRequestOpt_ )
            {
                currentRequestOpt_.reset();

                if ( artUrlOpt )
                {
                    fb2k::inMainThread( [] {
                        auto pm = DiscordAdapter::GetInstance().GetPresenceModifier();
                        pm.UpdateImage();
                    } );
                }
            }
            lastRequest.reset();
        }
    }
}

std::optional<qwr::u8string> ArtworkFetcher::ProcessFetchRequest( const MusicBrainzFetchRequest& request )
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

std::optional<qwr::u8string> ArtworkFetcher::ProcessFetchRequest( const UploadRequest& request )
{
    try
    {
        return UploadArt( request.handle, request.uploadCommand );
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
