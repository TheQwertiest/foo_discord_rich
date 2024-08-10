#include <stdafx.h>

#include "config.h"

#include <discord/discord_integration.h>

namespace drp::config
{

qwr::fb2k::ConfigBool isEnabled( guid::conf_is_enabled, true );
qwr::fb2k::ConfigUint8Enum<ImageSetting> largeImageSettings( guid::conf_large_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<ImageSetting> smallImageSettings( guid::conf_small_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<TimeSetting> timeSettings( guid::conf_time_settings, TimeSetting::Disabled );
qwr::fb2k::ConfigBool enableAlbumArtFetch( guid::conf_enable_album_art_fetch, true );
qwr::fb2k::ConfigBool enableArtUpload( guid::conf_enable_art_upload, false );
qwr::fb2k::ConfigString artUploadCmd( guid::conf_art_upload_cmd, "" );
qwr::fb2k::ConfigString artUploadPinQuery( guid::conf_art_upload_pin_query, "%artist%|%album%" );

qwr::fb2k::ConfigString topTextQuery( guid::conf_top_text_query, "[%title%]" );
qwr::fb2k::ConfigString middleTextQuery( guid::conf_middle_text_query, "[by %album artist%]" );
qwr::fb2k::ConfigString bottomTextQuery( guid::conf_bottom_text_query_v2, "[on %album%]" );
qwr::fb2k::ConfigString bottomTextQuery_v1_deprecated( guid::conf_bottom_text_query_v1_deprecated, "[%album artist%[: %album%]]" );

qwr::fb2k::ConfigString discordAppToken( guid::conf_app_token, "507982587416018945" );
qwr::fb2k::ConfigString largeImageId_Light( guid::conf_large_image_id_light, "foobar2000" );
qwr::fb2k::ConfigString largeImageId_Dark( guid::conf_large_image_id_dark, "foobar2000-dark" );
qwr::fb2k::ConfigString playingImageId_Light( guid::conf_playing_image_id_light, "playing" );
qwr::fb2k::ConfigString playingImageId_Dark( guid::conf_playing_image_id_dark, "playing-dark" );
qwr::fb2k::ConfigString pausedImageId_Light( guid::conf_paused_image_id_light, "paused" );
qwr::fb2k::ConfigString pausedImageId_Dark( guid::conf_paused_image_id_dark, "paused-dark" );

qwr::fb2k::ConfigBool disableWhenPaused( guid::conf_disable_when_paused, false );
qwr::fb2k::ConfigBool swapSmallImages( guid::conf_swap_small_images, false );

} // namespace drp::config
