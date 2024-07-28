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
    qwr::u8string state;
    qwr::u8string details;
    qwr::u8string largeImageKey;
    qwr::u8string smallImageKey;
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
                      const drp::internal::PresenceData& presenceData );

private:
    DiscordHandler& parent_;

    bool isDisabled_ = false;
    drp::internal::PresenceData presenceData_;
};

class DiscordHandler
{
    friend class drp::PresenceModifier;

public:
    static DiscordHandler& GetInstance();

    void Initialize();
    void Finalize();
    void OnSettingsChanged();

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
    qwr::u8string appToken_;
    drp::internal::PresenceData presenceData_;
};

} // namespace drp
