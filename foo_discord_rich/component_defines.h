#pragma once

#define DRP_NAME "Discord Rich Presense Integration"
#define DRP_DLL_NAME "foo_discord_rich.dll"

#define DRP_STRINGIFY_HELPER(x) #x
#define DRP_STRINGIFY(x) DRP_STRINGIFY_HELPER(x)

#define DRP_VERSION_MAJOR 1
#define DRP_VERSION_MINOR 0
#define DRP_VERSION_PATCH 0
//#define DRP_VERSION_PRERELEASE_TEXT "alpha"

#ifdef DRP_VERSION_PRERELEASE_TEXT
#    define DRP_VERSION_PRERELEASE "-"DRP_VERSION_PRERELEASE_TEXT
#else
#    define DRP_VERSION_PRERELEASE ""
#endif
#ifdef _DEBUG
#	define DRP_VERSION_DEBUG_SUFFIX " (Debug)"
#else
#	define DRP_VERSION_DEBUG_SUFFIX ""
#endif

#define DRP_VERSION DRP_STRINGIFY(DRP_VERSION_MAJOR)"."DRP_STRINGIFY(DRP_VERSION_MINOR)"."DRP_STRINGIFY(DRP_VERSION_PATCH)DRP_VERSION_PRERELEASE
#define DRP_NAME_WITH_VERSION DRP_NAME " v" DRP_VERSION DRP_VERSION_DEBUG_SUFFIX

constexpr GUID g_guid_drp_ui_pref = { 0xc09b193e, 0xb53d, 0x4652, { 0xa9, 0xa2, 0xf6, 0xbc, 0xe9, 0x65, 0x1f, 0xe9 } };
constexpr GUID g_guid_drp_conf_is_enabled = { 0x13ff3bbd, 0x1797, 0x42f1, { 0x96, 0xf4, 0x9d, 0xaf, 0x57, 0x31, 0x8d, 0xd2 } };
constexpr GUID g_guid_drp_conf_image_settings = { 0x5148849e, 0xc4a5, 0x4f5f, { 0xbd, 0xc1, 0x50, 0x21, 0x4, 0x1c, 0xfa, 0x8e } };
constexpr GUID g_guid_drp_conf_time_settings = { 0xca5e9b25, 0x83db, 0x424b, { 0x85, 0xf3, 0x5, 0x7b, 0x8a, 0x18, 0xcc, 0xcd } };
constexpr GUID g_guid_drp_conf_state_query = { 0xcdf5f57f, 0x1ad2, 0x4a87, { 0xa4, 0xcd, 0x29, 0x4a, 0x8f, 0x51, 0x15, 0xb0 } };
constexpr GUID g_guid_drp_conf_details_query = { 0xb678c223, 0xf312, 0x4629, { 0x89, 0xc5, 0x10, 0x30, 0x41, 0x2, 0xd7, 0x90 } };
constexpr GUID g_guid_drp_conf_partyId_query = { 0x14a7e912, 0x24e9, 0x4640, { 0x87, 0x3c, 0xdb, 0x2f, 0x42, 0xf4, 0x9f, 0xc6 } };
constexpr GUID g_guid_acfu_source = { 0x19eeccd4, 0x18c6, 0x40ae, { 0x94, 0xbc, 0xb0, 0x63, 0x1d, 0x77, 0xe3, 0x8d } };
