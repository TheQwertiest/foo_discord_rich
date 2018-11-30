#include "stdafx.h"

#include <discord_impl.h>
#include <config.h>

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
        drp::config::InitializeConfig();
        drp::InitializeDiscord();
    }

    void on_quit() override
    {
        drp::FinalizeDiscord();
    }
};

initquit_factory_t<ComponentInitQuit> g_initquit;

} // namespace
