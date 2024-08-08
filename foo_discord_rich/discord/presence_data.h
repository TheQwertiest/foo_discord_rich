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

    void UpdateTextFieldPointers();

private:
    void CopyData( const PresenceData& other );

public:
    DiscordRichPresence presence;
    metadb_handle_ptr metadb;
    qwr::u8string topText;
    qwr::u8string middleText;
    qwr::u8string bottomText;
    qwr::u8string largeImageKey;
    qwr::u8string smallImageKey;
    double trackLength = 0;
};

} // namespace drp::internal

namespace drp
{

class DiscordAdapter;

/// @details Destructor updates presence data in parent and sends it to Discord
class PresenceModifier
{
    friend class drp::DiscordAdapter;

public:
    ~PresenceModifier();

    void UpdateImage();
    void UpdateSmallImage();
    void UpdateTrack( metadb_handle_ptr metadb = metadb_handle_ptr() );
    void UpdateDuration( double currentTime );
    void UpdateDuration( double currentTime, double totalLength );
    void DisableDuration();

    bool HasChanged() const;
    void Rollback();

    /// @brief Disables Discord Rich Presence
    /// @details The updated data will still be transferred to parent
    void Disable();

private:
    PresenceModifier( DiscordAdapter& parent, const drp::internal::PresenceData& presenceData );

private:
    DiscordAdapter& parent_;

    bool isDisabled_ = false;
    drp::internal::PresenceData presenceData_;
};

} // namespace drp
