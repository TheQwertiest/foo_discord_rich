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

    DiscordRichPresence presence;
    metadb_handle_ptr metadb;
    pfc::string8_fast state;
    pfc::string8_fast details;
    double trackLength = 0;
};

} // namespace drp::internal

namespace drp
{

class DiscordHandler;

class PresenceModifier
{
    friend class drp::DiscordHandler;

public:
    ~PresenceModifier();

    void UpdateImage();
    void UpdateTrack( metadb_handle_ptr metadb = metadb_handle_ptr() );
    void UpdateDuration( double time );
    void DisableDuration();
    void Clear();

private:
    PresenceModifier( DiscordHandler& parent,
                      const drp::internal::PresenceData& presenceData );

private:
    DiscordHandler& parent_;

    bool isCleared_ = false;
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
    void SendPresense();
    void ClearPresence();

private:
    DiscordHandler() = default;

    static void OnReady( const DiscordUser* request );
    static void OnDisconnected( int errorCode, const char* message );
    static void OnErrored( int errorCode, const char* message );

private:
    drp::internal::PresenceData presenceData_;
};

} // namespace drp
