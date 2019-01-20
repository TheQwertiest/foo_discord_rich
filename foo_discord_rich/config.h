#pragma once

#include <utils/cfg_wrap.h>

namespace drp::config
{

enum class ImageSetting : uint8_t
{
    Light = 0,
    Dark,
    Disabled
};
enum class TimeSetting : uint8_t
{
    Elapsed = 0,
    Remaining,
    Disabled
};

extern utils::CfgWrapBool g_isEnabled;
extern utils::CfgWrapUint8 g_largeImageSettings;
extern utils::CfgWrapUint8 g_smallImageSettings;
extern utils::CfgWrapUint8 g_timeSettings;
extern utils::CfgWrapString8 g_stateQuery;
extern utils::CfgWrapString8 g_detailsQuery;

extern utils::CfgWrapString8 g_discordAppToken;
extern utils::CfgWrapString8 g_largeImageId_Light;
extern utils::CfgWrapString8 g_largeImageId_Dark;
extern utils::CfgWrapString8 g_playingImageId_Light;
extern utils::CfgWrapString8 g_playingImageId_Dark;
extern utils::CfgWrapString8 g_pausedImageId_Dark;
extern utils::CfgWrapString8 g_pausedImageId_Light;

void InitializeConfig();

}; // namespace drp::config
