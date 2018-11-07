#include <stdafx.h>
#include "config.h"

#include <discord_impl.h>

namespace drp::config
{

utils::CfgWrap<cfg_bool, bool> g_isEnabled( g_guid_drp_conf_is_enabled, true );
utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_imageSettings( g_guid_drp_conf_image_settings, static_cast<uint8_t>( ImageSetting::Light ) );
utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_timeSettings( g_guid_drp_conf_time_settings, static_cast<uint8_t>( TimeSetting::Elapsed ) );
utils::CfgWrap<cfg_string, pfc::string8_fast> g_stateQuery( g_guid_drp_conf_state_query, "[%title%]" );
utils::CfgWrap<cfg_string, pfc::string8_fast> g_detailsQuery( g_guid_drp_conf_details_query, "[%album artist%[: %album%]]" );

void InitializeConfig()
{
    g_isEnabled.Reread();
    g_imageSettings.Reread();
    g_timeSettings.Reread();
    g_stateQuery.Reread();
    g_detailsQuery.Reread();
}

} // namespace drp::config
