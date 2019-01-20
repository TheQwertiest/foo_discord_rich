#include <stdafx.h>
#include "ui_pref_tab_advanced.h"

#include <ui/ui_pref_tab_manager.h>
#include <discord_impl.h>
#include <config.h>

namespace drp::ui
{

using namespace config;

PreferenceTabAdvanced::PreferenceTabAdvanced( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , configs_( { CreateUiCfgWrap( config::g_discordAppToken, IDC_TEXTBOX_APP_TOKEN ),
                  CreateUiCfgWrap( config::g_largeImageId_Light, IDC_TEXTBOX_LARGE_LIGHT_ID ),
                  CreateUiCfgWrap( config::g_largeImageId_Dark, IDC_TEXTBOX_LARGE_DARK_ID ),
                  CreateUiCfgWrap( config::g_playingImageId_Light, IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID ),
                  CreateUiCfgWrap( config::g_playingImageId_Dark, IDC_TEXTBOX_SMALL_PLAYING_DARK_ID ),
                  CreateUiCfgWrap( config::g_pausedImageId_Light, IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID ),
                  CreateUiCfgWrap( config::g_pausedImageId_Dark, IDC_TEXTBOX_SMALL_PAUSED_DARK_ID ) } )
{
}

PreferenceTabAdvanced::~PreferenceTabAdvanced()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().Revert();
    }
}

HWND PreferenceTabAdvanced::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabAdvanced::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabAdvanced::Name() const
{
    return L"Advanced";
}

t_uint32 PreferenceTabAdvanced::get_state()
{
    const bool hasChanged =
        configs_.cend() != std::find_if( configs_.cbegin(), configs_.cend(), []( const auto& config ) {
            return config->GetCfg().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabAdvanced::apply()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().Apply();
    }
}

void PreferenceTabAdvanced::reset()
{
    for ( auto& config : configs_ )
    {
        config->GetCfg().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabAdvanced::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& config : configs_ )
    {
        config->SetHwnd( m_hWnd );
    }
    UpdateUiFromCfg();

    return TRUE; // set focus to default control
}

void PreferenceTabAdvanced::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
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

void PreferenceTabAdvanced::OnChanged()
{
    pParent_->OnDataChanged();
}

void PreferenceTabAdvanced::UpdateUiFromCfg()
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
