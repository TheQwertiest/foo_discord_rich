#include "stdafx.h"

#include <discord_impl.h>

#include <discord_rpc.h>

#include <ctime>

DECLARE_COMPONENT_VERSION(
    DRP_NAME,
    DRP_VERSION,
    DRP_NAME_WITH_VERSION " by TheQwertiest" );

VALIDATE_COMPONENT_FILENAME( DRP_DLL_NAME );

namespace
{

class ComponentInitQuit : public initquit
{
public:
    void on_init() override
    {
        discord::InitializeDiscord();
    }

    void on_quit() override
    {
        discord::FinalizeDiscord();
    }
};

initquit_factory_t<ComponentInitQuit> g_initquit;

} // namespace

extern "C" BOOL WINAPI DllMain( [[maybe_unused]] HINSTANCE ins, DWORD reason, [[maybe_unused]] LPVOID lp )
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
