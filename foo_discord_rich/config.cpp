#include <stdafx.h>
#include "config.h"

#include <discord_impl.h>

namespace drp::config
{

utils::CfgWrapBool g_isEnabled( g_guid_drp_conf_is_enabled, true );
utils::CfgWrapUint8 g_largeImageSettings( g_guid_drp_conf_large_image_settings, static_cast<uint8_t>( ImageSetting::Light ) );
utils::CfgWrapUint8 g_smallImageSettings( g_guid_drp_conf_small_image_settings, static_cast<uint8_t>( ImageSetting::Light ) );
utils::CfgWrapUint8 g_timeSettings( g_guid_drp_conf_time_settings, static_cast<uint8_t>( TimeSetting::Elapsed ) );
utils::CfgWrapString8 g_stateQuery( g_guid_drp_conf_state_query, "[%title%]" );
utils::CfgWrapString8 g_detailsQuery( g_guid_drp_conf_details_query, "[%album artist%[: %album%]]" );

utils::CfgWrapString8 g_discordAppToken( g_guid_drp_conf_app_token, "507982587416018945" );
utils::CfgWrapString8 g_largeImageId_Light( g_guid_drp_conf_large_image_id_light, "foobar2000" );
utils::CfgWrapString8 g_largeImageId_Dark( g_guid_drp_conf_large_image_id_dark, "foobar2000-dark" );
utils::CfgWrapString8 g_playingImageId_Light( g_guid_drp_conf_playing_image_id_light, "playing" );
utils::CfgWrapString8 g_playingImageId_Dark( g_guid_drp_conf_playing_image_id_dark, "playing-dark" );
utils::CfgWrapString8 g_pausedImageId_Light( g_guid_drp_conf_paused_image_id_light, "paused" );
utils::CfgWrapString8 g_pausedImageId_Dark( g_guid_drp_conf_paused_image_id_dark, "paused-dark" );

void InitializeConfig()
{
    g_isEnabled.Reread();
    g_largeImageSettings.Reread();
    g_smallImageSettings.Reread();
    g_timeSettings.Reread();
    g_stateQuery.Reread();
    g_detailsQuery.Reread();

    g_discordAppToken.Reread();
    g_largeImageId_Light.Reread();
    g_largeImageId_Dark.Reread();
    g_playingImageId_Light.Reread();
    g_playingImageId_Dark.Reread();
    g_pausedImageId_Light.Reread();
    g_pausedImageId_Dark.Reread();
}

} // namespace drp::config
