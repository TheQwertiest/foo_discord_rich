#include "stdafx.h"

#include <album_art/fetcher.h>
#include <discord/discord_integration.h>
#include <fb2k/config.h>

#include <qwr/abort_callback.h>

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
        drp::DiscordAdapter::GetInstance().Initialize();
        drp::AlbumArtFetcher::Get().Initialize();
    }

    void on_quit() override
    {
        qwr::GlobalAbortCallback::GetInstance().Abort();
        drp::AlbumArtFetcher::Get().Finalize();
        drp::DiscordAdapter::GetInstance().Finalize();
    }
};

initquit_factory_t<ComponentInitQuit> g_initquit;

} // namespace
