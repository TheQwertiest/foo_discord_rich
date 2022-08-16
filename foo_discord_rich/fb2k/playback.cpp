#include <stdafx.h>

#include <discord/discord_impl.h>

namespace
{

using namespace drp;

class PlaybackCallback : public play_callback_static
{
public:
    unsigned get_flags() override;
    void on_playback_starting( play_control::t_track_command cmd, bool paused ) override
    { // ignore
    }
    void on_playback_new_track( metadb_handle_ptr track ) override;
    void on_playback_stop( play_control::t_stop_reason reason ) override;
    void on_playback_seek( double time ) override;
    void on_playback_pause( bool state ) override;
    void on_playback_edited( metadb_handle_ptr track ) override;
    void on_playback_dynamic_info( const file_info& info ) override
    { // ignore
    }
    void on_playback_dynamic_info_track( const file_info& info ) override;
    void on_playback_time( double time ) override;
    void on_volume_change( float p_new_val ) override
    { // ignore
    }

private:
    void on_playback_changed( metadb_handle_ptr track = metadb_handle_ptr() );

private:
    bool needPresenceRefresh_ = false;
};

} // namespace

namespace
{

unsigned PlaybackCallback::get_flags()
{
    return flag_on_playback_all;
}

void PlaybackCallback::on_playback_new_track( metadb_handle_ptr track )
{
    //on_playback_changed( track );
    auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
    pm.UpdateTrack( track );
    pm.UpdateSmallImage();
    pm.UpdateImage();
}

void PlaybackCallback::on_playback_stop( play_control::t_stop_reason reason )
{
    if ( play_control::t_stop_reason::stop_reason_starting_another != reason )
    {
        auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
        pm.Disable();
    }
}

void PlaybackCallback::on_playback_seek( double time )
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

void PlaybackCallback::on_playback_pause( bool state )
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

void PlaybackCallback::on_playback_edited( metadb_handle_ptr track )
{
    on_playback_changed( track );
}

void PlaybackCallback::on_playback_dynamic_info_track( const file_info& info )
{
    on_playback_changed();
}

void PlaybackCallback::on_playback_time( double time )
{
    if ( needPresenceRefresh_ )
    {
        auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
        pm.UpdateTrack();
        pm.UpdateSmallImage();

        needPresenceRefresh_ = false;
    }
}

void PlaybackCallback::on_playback_changed( metadb_handle_ptr track )
{
    auto pm = DiscordHandler::GetInstance().GetPresenceModifier();
    pm.UpdateTrack( track );
}

} // namespace

namespace
{

play_callback_static_factory_t<PlaybackCallback> playbackCallback;

} // namespace
