#pragma once

// Spider Monkey ESR60 supports only Win7+
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

// Fix std min max conflicts
#define NOMINMAX
#include <algorithm>

#include <WinSock2.h>
#include <Windows.h>

// ATL/WTL
#include <atlstr.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atltheme.h>
#include <atltypes.h>

// foobar2000 SDK
#pragma warning( push, 0 )
#   include <foobar2000/SDK/foobar2000.h>
#pragma warning( pop ) 

// Some macros defined by windowsx.h should be removed
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif

#include "component_defines.h"
