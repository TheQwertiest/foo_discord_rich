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
    struct FetchRequest
    {
        qwr::u8string artist;
        qwr::u8string album;
        std::optional<qwr::u8string> userReleaseMbidOpt;

        auto operator<=>( const FetchRequest& other ) const = default;
    };

public:
    static AlbumArtFetcher& Get();

    void Initialize();
    void Finalize();

    std::optional<qwr::u8string> GetArtUrl( const FetchRequest& request );

private:
    void LoadCache();
    void SaveCache();

    void StartThread();
    void StopThread();

    void ThreadMain();

    std::optional<qwr::u8string> ProcessFetchRequest( const FetchRequest& request );

private:
    AlbumArtFetcher() = default;

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::unique_ptr<std::jthread> pThread_;

    std::optional<FetchRequest> currentRequestOpt_;
    std::unordered_map<qwr::u8string, std::optional<qwr::u8string>> albumIdToArtUrl_;
};

} // namespace drp
