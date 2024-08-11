#pragma once

#include <qwr/fb2k_adv_config.h>

namespace drp::config::advanced
{

extern qwr::fb2k::AdvConfigBool_MT enableDebugLog;
extern qwr::fb2k::AdvConfigBool_MT logWebRequests;
extern qwr::fb2k::AdvConfigBool_MT logWebResponses;
extern qwr::fb2k::AdvConfigBool_MT logUploaderCmds;
extern qwr::fb2k::AdvConfigBool_MT logUploaderOutput;

} // namespace drp::config::advanced
