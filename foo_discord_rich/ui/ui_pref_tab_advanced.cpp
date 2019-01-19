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
    , configs_( {
          config::g_discordAppToken,
          config::g_largeImageId_Light,
          config::g_largeImageId_Dark,
          config::g_playingImageId_Light,
          config::g_playingImageId_Dark,
          config::g_pausedImageId_Light,
          config::g_pausedImageId_Dark,
      } )
{
}

PreferenceTabAdvanced::~PreferenceTabAdvanced()
{
    for ( auto& config : configs_ )
    {
        config.get().Revert();
    }
}

HWND PreferenceTabAdvanced::CreateTab( HWND hParent )
{
    return Create(hParent);
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
            return config.get().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabAdvanced::apply()
{
    for ( auto& config : configs_ )
    {
        config.get().Apply();
    }
}

void PreferenceTabAdvanced::reset()
{
    for ( auto& config : configs_ )
    {
        config.get().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabAdvanced::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    UpdateUiFromCfg();

    return TRUE; // set focus to default control
}

void PreferenceTabAdvanced::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto getDlgItemText = [&wndCtl]() {
        CString tmp;
        if ( !wndCtl.GetWindowText( tmp ) )
        {
            tmp = "";
        }
        return pfc::string8_fast{ pfc::stringcvt::string_utf8_from_wide( tmp.GetBuffer() ) };
    };

    switch ( nID )
    {
    case IDC_TEXTBOX_APP_TOKEN:
    {
        g_discordAppToken = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_LARGE_LIGHT_ID:
    {
        g_largeImageId_Light = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_LARGE_DARK_ID:
    {
        g_largeImageId_Dark = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID:
    {
        g_playingImageId_Light = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_SMALL_PLAYING_DARK_ID:
    {
        g_playingImageId_Dark = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID:
    {
        g_pausedImageId_Light = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_SMALL_PAUSED_DARK_ID:
    {
        g_pausedImageId_Dark = getDlgItemText();
        break;
    }
    default:
        break;
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

    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_APP_TOKEN, g_discordAppToken.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_LARGE_LIGHT_ID, g_largeImageId_Light.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_LARGE_DARK_ID, g_largeImageId_Dark.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID, g_playingImageId_Light.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_SMALL_PLAYING_DARK_ID, g_playingImageId_Dark.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID, g_pausedImageId_Light.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_SMALL_PAUSED_DARK_ID, g_pausedImageId_Dark.GetCurrentValue() );
}

} // namespace drp::ui
