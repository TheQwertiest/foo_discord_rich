#include <stdafx.h>

#include "config.h"

#include <discord/discord_impl.h>

namespace drp::config
{

qwr::fb2k::ConfigBool isEnabled( guid::conf_is_enabled, true );
qwr::fb2k::ConfigUint8Enum<ImageSetting> largeImageSettings( guid::conf_large_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<ImageSetting> smallImageSettings( guid::conf_small_image_settings, ImageSetting::Light );
qwr::fb2k::ConfigUint8Enum<TimeSetting> timeSettings( guid::conf_time_settings, TimeSetting::Elapsed );

qwr::fb2k::ConfigString stateQuery( guid::conf_state_query, "[%title%]" );
qwr::fb2k::ConfigString detailsQuery( guid::conf_details_query, "[%album artist%[: %album%]]" );

qwr::fb2k::ConfigString discordAppToken( guid::conf_app_token, "507982587416018945" );
qwr::fb2k::ConfigString largeImageId_Light( guid::conf_large_image_id_light, "foobar2000" );
qwr::fb2k::ConfigString largeImageId_Dark( guid::conf_large_image_id_dark, "foobar2000-dark" );
qwr::fb2k::ConfigString playingImageId_Light( guid::conf_playing_image_id_light, "playing" );
qwr::fb2k::ConfigString playingImageId_Dark( guid::conf_playing_image_id_dark, "playing-dark" );
qwr::fb2k::ConfigString pausedImageId_Light( guid::conf_paused_image_id_light, "paused" );
qwr::fb2k::ConfigString pausedImageId_Dark( guid::conf_paused_image_id_dark, "paused-dark" );
qwr::fb2k::ConfigString uploadArtworkCommand( guid::conf_upload_artwork_command, "" );
qwr::fb2k::ConfigString artworkMetadbKey( guid::conf_artwork_metadb_key, "%album artist% - $if2([%album%],%title%) [ - %discnumber%]" );
qwr::fb2k::ConfigUint32 processTimeout( guid::conf_process_timeout, 10 );

qwr::fb2k::ConfigBool disableWhenPaused( guid::conf_disable_when_paused, false );
qwr::fb2k::ConfigBool swapSmallImages( guid::conf_swap_small_images, false );
qwr::fb2k::ConfigBool uploadArtwork( guid::conf_upload_artwork, false );

} // namespace drp::config
