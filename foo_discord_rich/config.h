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
extern utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_imageSettings;
extern utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t> g_timeSettings;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_stateQuery;
extern utils::CfgWrap<cfg_string, pfc::string8_fast> g_detailsQuery;

void InitializeConfig();

}; // namespace drp::config
