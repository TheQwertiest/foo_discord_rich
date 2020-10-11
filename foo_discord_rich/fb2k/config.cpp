#include <stdafx.h>

#include "config.h"

#include <discord/discord_impl.h>

namespace drp::config
{

qwr::fb2k::ConfigBool g_isEnabled( guid::conf_is_enabled, true );
qwr::fb2k::ConfigUint8Enum<ImageSetting> g_largeImageSettings( guid::conf_large_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<ImageSetting> g_smallImageSettings( guid::conf_small_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<TimeSetting> g_timeSettings( guid::conf_time_settings, TimeSetting::Elapsed );

qwr::fb2k::ConfigString g_stateQuery( guid::conf_state_query, "[%title%]" );
qwr::fb2k::ConfigString g_detailsQuery( guid::conf_details_query, "[%album artist%[: %album%]]" );

qwr::fb2k::ConfigString g_discordAppToken( guid::conf_app_token, "507982587416018945" );
qwr::fb2k::ConfigString g_largeImageId_Light( guid::conf_large_image_id_light, "foobar2000" );
qwr::fb2k::ConfigString g_largeImageId_Dark( guid::conf_large_image_id_dark, "foobar2000-dark" );
qwr::fb2k::ConfigString g_playingImageId_Light( guid::conf_playing_image_id_light, "playing" );
qwr::fb2k::ConfigString g_playingImageId_Dark( guid::conf_playing_image_id_dark, "playing-dark" );
qwr::fb2k::ConfigString g_pausedImageId_Light( guid::conf_paused_image_id_light, "paused" );
qwr::fb2k::ConfigString g_pausedImageId_Dark( guid::conf_paused_image_id_dark, "paused-dark" );

qwr::fb2k::ConfigBool g_disableWhenPaused( guid::conf_disable_when_paused, false );
qwr::fb2k::ConfigBool g_swapSmallImages( guid::conf_swap_small_images, false );

} // namespace drp::config
