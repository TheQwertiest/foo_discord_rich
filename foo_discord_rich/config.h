#pragma once

#include <utils/cfg_wrap.h>

namespace drp::config
{

enum class ImageSetting : uint8_t
{
    Light,
    Dark,
    Disabled
};
enum class TimeSetting : uint8_t
{
    Elapsed,
    Remaining,
    Disabled
};

extern utils::CfgWrap<cfg_bool, bool> g_isEnabled;
extern utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_largeImageSettings;
extern utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_smallImageSettings;
extern utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_timeSettings;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_stateQuery;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_detailsQuery;

extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_discordAppToken;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_largeImageId_Light;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_largeImageId_Dark;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_playingImageId_Light;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_playingImageId_Dark;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_pausedImageId_Dark;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_pausedImageId_Light;

void InitializeConfig();

}; // namespace drp::config
