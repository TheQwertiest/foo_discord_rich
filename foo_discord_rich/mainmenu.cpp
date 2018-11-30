#include <stdafx.h>

#include <discord_impl.h>
#include <config.h>

namespace
{

class MainMenuCommandsImpl : public mainmenu_commands
{
public:
    t_uint32 get_command_count() override
    {
        return 1;
    }
    GUID get_command( t_uint32 p_index ) override
    {
        switch ( p_index )
        {
        case 0:
            return g_guid_drp_mainmenu_cmd_enable;
        default:
            uBugCheck();
        }
    }
    void get_name( t_uint32 p_index, pfc::string_base& p_out ) override
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
    bool get_description( t_uint32 /* p_index */, pfc::string_base& p_out ) override
    {
        p_out = "Toggles Discord Rich Presence";
        return true;
    }
    GUID get_parent() override
    {
        return mainmenu_groups::view;
    }
    void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback ) override
    {
        drp::config::g_isEnabled = !drp::config::g_isEnabled;
        drp::config::g_isEnabled.Apply();
        drp::DiscordHandler::GetInstance().OnSettingsChanged();
    }
    bool get_display( t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags ) override
    {
        get_name( p_index, p_out );
        p_flags = sort_priority_dontcare | ( drp::config::g_isEnabled ? mainmenu_commands::flag_checked : 0 );
        return true;
    }
};

mainmenu_commands_factory_t<MainMenuCommandsImpl> g_mainmenuCommands;

} // namespace
