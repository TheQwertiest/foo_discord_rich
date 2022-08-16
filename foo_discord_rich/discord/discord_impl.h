#pragma once

#include <discord_rpc.h>

namespace drp::internal
{

struct PresenceData
{
    PresenceData();
    PresenceData( const PresenceData& other );

    PresenceData& operator=( const PresenceData& other );
    bool operator==( const PresenceData& other );
    bool operator!=( const PresenceData& other );

private:
    void CopyData( const PresenceData& other );

public:
    DiscordRichPresence presence;
    metadb_handle_ptr metadb;
    std::u8string state;
    std::u8string details;
    std::u8string largeImageKey;
    std::u8string smallImageKey;
    double trackLength = 0;
};

} // namespace drp::internal

namespace drp
{

class DiscordHandler;

/// @details Destructor updates presence data in parent and sends it to Discord
class PresenceModifier
{
    friend class drp::DiscordHandler;

public:
    ~PresenceModifier();

    void UpdateImage();
    void UpdateSmallImage();
    void UpdateTrack( metadb_handle_ptr metadb = metadb_handle_ptr() );
    void UpdateDuration( double time );
    void DisableDuration();

    /// @brief Disables Discord Rich Presence
    /// @details The updated data will still be transferred to parent
    void Disable();

private:
    PresenceModifier( DiscordHandler& parent,
                      const std::shared_ptr<internal::PresenceData> presenceData );

private:
    DiscordHandler& parent_;

    bool isDisabled_ = false;
    std::shared_ptr<internal::PresenceData> presenceData_;
};

class DiscordHandler
{
    friend class drp::PresenceModifier;

public:
    static DiscordHandler& GetInstance();

    void Initialize();
    void Finalize();
    void OnSettingsChanged();
    /* Update presence if the given presence data pointer is the same as the current one */
    void MaybeUpdatePresence(std::shared_ptr<internal::PresenceData> pd);

    drp::PresenceModifier GetPresenceModifier();

private:
    bool HasPresence() const;
    void SendPresence();
    void ClearPresence();

private:
    DiscordHandler() = default;

    static void OnReady( const DiscordUser* request );
    static void OnDisconnected( int errorCode, const char* message );
    static void OnErrored( int errorCode, const char* message );

private:
    bool hasPresence_ = true;
    std::u8string appToken_;
    std::shared_ptr<internal::PresenceData> presenceData_ = std::make_shared<internal::PresenceData>();
};

} // namespace drp
