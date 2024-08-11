#include <stdafx.h>

#include "advanced_config.h"

namespace
{

advconfig_branch_factory branchDrp(
    DRP_NAME, drp::guid::adv_branch, advconfig_branch::guid_branch_tools, 0 );

advconfig_branch_factory branchLog(
    "Logging", drp::guid::adv_branch_log, drp::guid::adv_branch, 0 );

} // namespace

namespace drp::config::advanced
{

qwr::fb2k::AdvConfigBool_MT enableDebugLog(
    "Enable",
    drp::guid::adv_var_enable_debug_log, drp::guid::adv_branch_log, 0, false );

qwr::fb2k::AdvConfigBool_MT logWebRequests(
    "Log web requests",
    drp::guid::adv_var_log_web_requests, drp::guid::adv_branch_log, 1, false );

qwr::fb2k::AdvConfigBool_MT logWebResponses(
    "Log web responses",
    drp::guid::adv_var_log_web_responses, drp::guid::adv_branch_log, 2, false );

qwr::fb2k::AdvConfigBool_MT logUploaderCmds(
    "Log uploader commands",
    drp::guid::adv_var_log_uploader_cmd, drp::guid::adv_branch_log, 3, false );

qwr::fb2k::AdvConfigBool_MT logUploaderOutput(
    "Log uploader returned urls",
    drp::guid::adv_var_log_uploader_output, drp::guid::adv_branch_log, 4, false );

} // namespace drp::config::advanced
