#include <stdafx.h>

#include <discord/discord_integration.h>
#include <fb2k/config.h>

namespace
{

class MainMenuCommandsImpl : public mainmenu_commands
{
public:
    t_uint32 get_command_count() override;
    GUID get_command( t_uint32 p_index ) override;
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override;
    bool get_description( t_uint32 /* p_index */, pfc::string_base& p_out ) override;
    GUID get_parent() override;
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override;
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags ) override;
};

} // namespace

namespace
{

t_uint32 MainMenuCommandsImpl::get_command_count()
{
    return 1;
}

GUID MainMenuCommandsImpl::get_command( t_uint32 p_index )
{
    switch ( p_index )
    {
    case 0:
        return drp::guid::mainmenu_cmd_enable;
    default:
        uBugCheck();
    }
}

void MainMenuCommandsImpl::get_name( t_uint32 p_index, pfc::string_base& p_out )
{
    switch ( p_index )
    {
    case 0:
        p_out = "Display Discord Rich Presence";
        return;
    default:
        uBugCheck();
    }
}

bool MainMenuCommandsImpl::get_description( t_uint32 /* p_index */, pfc::string_base& p_out )
{
    p_out = "Toggles Discord Rich Presence";
    return true;
}

GUID MainMenuCommandsImpl::get_parent()
{
    return mainmenu_groups::view;
}

void MainMenuCommandsImpl::execute( t_uint32 p_index, service_ptr_t<service_base> p_callback )
{
    drp::config::isEnabled = !drp::config::isEnabled;
    drp::DiscordAdapter::GetInstance().OnSettingsChanged();
}

bool MainMenuCommandsImpl::get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags )
{
    get_name( p_index, p_out );
    p_flags = sort_priority_dontcare | ( drp::config::isEnabled ? mainmenu_commands::flag_checked : 0 );
    return true;
}

} // namespace

namespace
{

mainmenu_commands_factory_t<MainMenuCommandsImpl> g_mainmenuCommands;

} // namespace
