#include <stdafx.h>
#include "ui_pref_tab_main.h"

#include <ui/ui_pref_tab_manager.h>
#include <discord/discord_impl.h>
#include <fb2k/config.h>

namespace drp::ui
{

using namespace config;

PreferenceTabMain::PreferenceTabMain( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , configs_( {
          CreateUiCfgWrap( config::g_isEnabled, IDC_CHECK_IS_ENABLED ),
          CreateUiCfgWrap( config::g_stateQuery, IDC_TEXTBOX_STATE ),
          CreateUiCfgWrap( config::g_detailsQuery, IDC_TEXTBOX_DETAILS ),
          CreateUiCfgWrapRange( config::g_largeImageSettings, std::initializer_list<int>{ IDC_RADIO_IMG_LIGHT, IDC_RADIO_IMG_DARK, IDC_RADIO_IMG_DISABLED } ),
          CreateUiCfgWrapRange( config::g_smallImageSettings, std::initializer_list<int>{ IDC_RADIO_PLAYBACK_IMG_LIGHT, IDC_RADIO_PLAYBACK_IMG_DARK, IDC_RADIO_PLAYBACK_IMG_DISABLED } ),
          CreateUiCfgWrapRange( config::g_timeSettings, std::initializer_list<int>{ IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_REMAINING, IDC_RADIO_TIME_DISABLED } ),
          CreateUiCfgWrap( config::g_disableWhenPaused, IDC_CHECK_DISABLE_WHEN_PAUSED ),
          CreateUiCfgWrap( config::g_swapSmallImages, IDC_CHECK_SWAP_STATUS ),
      } )
{
}

PreferenceTabMain::~PreferenceTabMain()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().Revert();
    }
}

HWND PreferenceTabMain::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabMain::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabMain::Name() const
{
    return L"Main";
}

t_uint32 PreferenceTabMain::get_state()
{
    const bool hasChanged =
        configs_.cend() != std::find_if( configs_.cbegin(), configs_.cend(), []( const auto& config ) {
            return config->GetCfg().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabMain::apply()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().Apply();
    }
}

void PreferenceTabMain::reset()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabMain::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& config : configs_ )
    {
        config->SetHwnd( m_hWnd );
    }
    UpdateUiFromCfg();

    helpUrl_.SetHyperLinkExtendedStyle( HLINK_UNDERLINED | HLINK_COMMANDBUTTON );
    helpUrl_.SetToolTipText( L"Title formatting help" );
    helpUrl_.SubclassWindow( GetDlgItem( IDC_LINK_FORMAT_HELP ) );

    return TRUE; // set focus to default control
}

void PreferenceTabMain::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto it = std::find_if( configs_.begin(), configs_.end(), [nID]( auto& val ) {
        return val->IsMatchingId( nID );
    } );

    if ( configs_.end() != it )
    {
        ( *it )->ReadFromUi();
    }

    OnChanged();
}

void PreferenceTabMain::OnHelpUrlClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    standard_commands::main_titleformat_help();
}

void PreferenceTabMain::OnChanged()
{
    pParent_->OnDataChanged();
}

void PreferenceTabMain::UpdateUiFromCfg()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& config : configs_ )
    {
        config->WriteToUi();
    }
}

} // namespace drp::ui
