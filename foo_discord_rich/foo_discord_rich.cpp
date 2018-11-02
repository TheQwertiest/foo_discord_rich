#include "stdafx.h"
//#include "thread_pool.h"
//#include "popup_msg.h"
//#include "panel_manager.h"
//#include "user_message.h"

DECLARE_COMPONENT_VERSION(
    DRP_NAME,
    DRP_VERSION,
    DRP_NAME_WITH_VERSION " by TheQwertiest" );

VALIDATE_COMPONENT_FILENAME( DRP_DLL_NAME );

namespace
{

class js_initquit : public initquit
{
public:
    void on_init() override
    {
    }

    void on_quit() override
    {
    }
};

initquit_factory_t<js_initquit> g_initquit;

} // namespace

extern "C" BOOL WINAPI DllMain( HINSTANCE ins, DWORD reason, [[maybe_unused]] LPVOID lp )
{
    switch ( reason )
    {
    case DLL_PROCESS_ATTACH:
    {
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        break;
    }
    }

    return TRUE;
}
