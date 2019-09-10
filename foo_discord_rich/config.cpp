#include <stdafx.h>
#include "config.h"

#include <discord_impl.h>

namespace drp::config
{

utils::CfgWrapBool g_isEnabled( drp::guid::conf_is_enabled, true );
utils::CfgWrapUint8 g_largeImageSettings( drp::guid::conf_large_image_settings, static_cast<uint8_t>( ImageSetting::Light ) );
utils::CfgWrapUint8 g_smallImageSettings( drp::guid::conf_small_image_settings, static_cast<uint8_t>( ImageSetting::Light ) );
utils::CfgWrapUint8 g_timeSettings( drp::guid::conf_time_settings, static_cast<uint8_t>( TimeSetting::Elapsed ) );
utils::CfgWrapString8 g_stateQuery( drp::guid::conf_state_query, "[%title%]" );
utils::CfgWrapString8 g_detailsQuery( drp::guid::conf_details_query, "[%album artist%[: %album%]]" );

utils::CfgWrapString8 g_discordAppToken( drp::guid::conf_app_token, "507982587416018945" );
utils::CfgWrapString8 g_largeImageId_Light( drp::guid::conf_large_image_id_light, "foobar2000" );
utils::CfgWrapString8 g_largeImageId_Dark( drp::guid::conf_large_image_id_dark, "foobar2000-dark" );
utils::CfgWrapString8 g_playingImageId_Light( drp::guid::conf_playing_image_id_light, "playing" );
utils::CfgWrapString8 g_playingImageId_Dark( drp::guid::conf_playing_image_id_dark, "playing-dark" );
utils::CfgWrapString8 g_pausedImageId_Light( drp::guid::conf_paused_image_id_light, "paused" );
utils::CfgWrapString8 g_pausedImageId_Dark( drp::guid::conf_paused_image_id_dark, "paused-dark" );

utils::CfgWrapBool g_disableWhenPaused( drp::guid::conf_disable_when_paused, false );
utils::CfgWrapBool g_swapSmallImages( drp::guid::conf_swap_small_images, false );

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

    g_disableWhenPaused.Reread();
    g_swapSmallImages.Reread();
}

} // namespace drp::config
