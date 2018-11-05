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

constexpr GUID g_guid_acfu_source = { 0x19eeccd4, 0x18c6, 0x40ae, { 0x94, 0xbc, 0xb0, 0x63, 0x1d, 0x77, 0xe3, 0x8d } };                                 
