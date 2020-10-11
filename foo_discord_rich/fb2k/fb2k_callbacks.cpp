#include <stdafx.h>

#include <discord/discord_impl.h>

namespace
{

using namespace drp;

class PlaybackCallback : public play_callback_static
{
public:
    unsigned get_flags() override
    {
        return flag_on_playback_all;
    }
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override
    { // ignore
    }
    void on_playback_new_track( metadb_handle_ptr track ) override
    {
        //on_playback_changed( track );
        auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
        pm.UpdateTrack( track );
        pm.UpdateSmallImage();
    }
    void on_playback_stop( play_control::t_stop_reason reason ) override
    {
        if ( play_control::t_stop_reason::stop_reason_starting_another != reason )
        {
            auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
            pm.Disable();
        }
    }
    void on_playback_seek( double time ) override
    {
        if ( playback_control::get()->is_playing() )
        { // track seeking might take some time, thus on_playback_time is needed
            needPresenceRefresh_ = true;
        }
        else
        {
            auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
            pm.UpdateDuration( time );
        }
    }
    void on_playback_pause( bool state ) override
    {
        if ( state )
        {
            auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
            pm.UpdateTrack();
            pm.UpdateSmallImage();
        }
        else
        { // resuming playback may take some time, thus on_playback_time is needed
            needPresenceRefresh_ = true;
        }
    }
    void on_playback_edited( metadb_handle_ptr track ) override
    {
        on_playback_changed( track );
    }
    void on_playback_dynamic_info( const file_info& info ) override
    { // ignore
    }
    void on_playback_dynamic_info_track( const file_info& info ) override
    {
        on_playback_changed();
    }
    void on_playback_time( double time ) override
    {
        if ( needPresenceRefresh_ )
        {
            auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
            pm.UpdateTrack();
            pm.UpdateSmallImage();

            needPresenceRefresh_ = false;
        }
    }
    void on_volume_change( float p_new_val ) override
    { // ignore
    }

private:
    void on_playback_changed( metadb_handle_ptr track = metadb_handle_ptr() )
    {
        auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
        pm.UpdateTrack( track );
    }

private:
    bool needPresenceRefresh_ = false;
};

play_callback_static_factory_t<PlaybackCallback> playbackCallback;

} // namespace
