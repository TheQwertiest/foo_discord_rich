#pragma once

#include <qwr/fb2k_config.h>

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

extern qwr::fb2k::ConfigBool g_isEnabled;
extern qwr::fb2k::ConfigUint8Enum<ImageSetting> g_largeImageSettings;
extern qwr::fb2k::ConfigUint8Enum<ImageSetting> g_smallImageSettings;
extern qwr::fb2k::ConfigUint8Enum<TimeSetting> g_timeSettings;
extern qwr::fb2k::ConfigString g_stateQuery;
extern qwr::fb2k::ConfigString g_detailsQuery;

extern qwr::fb2k::ConfigString g_discordAppToken;
extern qwr::fb2k::ConfigString g_largeImageId_Light;
extern qwr::fb2k::ConfigString g_largeImageId_Dark;
extern qwr::fb2k::ConfigString g_playingImageId_Light;
extern qwr::fb2k::ConfigString g_playingImageId_Dark;
extern qwr::fb2k::ConfigString g_pausedImageId_Dark;
extern qwr::fb2k::ConfigString g_pausedImageId_Light;

extern qwr::fb2k::ConfigBool g_disableWhenPaused;
extern qwr::fb2k::ConfigBool g_swapSmallImages;

}; // namespace drp::config
