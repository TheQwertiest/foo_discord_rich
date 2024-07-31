#pragma once

#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

namespace drp
{

class AlbumArtFetcher
{
public:
    static AlbumArtFetcher& Get();

    void Initialize();
    void Finalize();

    std::optional<qwr::u8string> GetArtUrl( const metadb_handle_ptr& handle );

private:
    void StartThread();
    void StopThread();

    void ThreadMain();

private:
    AlbumArtFetcher() = default;

private:
    struct TrackInfo
    {
        qwr::u8string artist;
        qwr::u8string album;

        auto operator<=>( const TrackInfo& other ) const = default;

        qwr::u8string GenerateId() const;
    };

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unique_ptr<std::jthread> pThread_;
    std::optional<TrackInfo> currentRequestedTrackOpt_;
    std::unordered_map<qwr::u8string, std::optional<qwr::u8string>> trackIdToMbid_;
    std::unordered_map<qwr::u8string, std::optional<qwr::u8string>> mbidToUrl_;
};

} // namespace drp
