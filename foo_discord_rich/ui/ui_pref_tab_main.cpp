#include <stdafx.h>
#include "ui_pref_tab_main.h"

#include <ui/ui_pref_tab_manager.h>
#include <discord_impl.h>
#include <config.h>

namespace drp::ui
{

using namespace config;

PreferenceTabMain::PreferenceTabMain( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , configs_( {
          config::g_isEnabled,
          config::g_largeImageSettings,
          config::g_smallImageSettings,
          config::g_timeSettings,
          config::g_stateQuery,
          config::g_detailsQuery,
          
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

PreferenceTabMain::~PreferenceTabMain()
{
    for ( auto& config : configs_ )
    {
        config.get().Revert();
    }
}

HWND PreferenceTabMain::CreateTab( HWND hParent )
{
    return Create(hParent);
}

const char* PreferenceTabMain::Name() const
{
    return "Main";
}

t_uint32 PreferenceTabMain::get_state()
{
    const bool hasChanged =
        configs_.cend() != std::find_if( configs_.cbegin(), configs_.cend(), []( const auto& config ) {
            return config.get().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabMain::apply()
{
    for ( auto& config : configs_ )
    {
        config.get().Apply();
    }
}

void PreferenceTabMain::reset()
{
    for ( auto& config : configs_ )
    {
        config.get().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabMain::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    UpdateUiFromCfg();

    return TRUE; // set focus to default control
}

void PreferenceTabMain::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto getDlgItemText = [&wndCtl]() {
        CString tmp;
        if ( !wndCtl.GetWindowText( tmp ) )
        {
            tmp = "";
        }
        const pfc::string8_fast str8 = pfc::stringcvt::string_utf8_from_wide( tmp.GetBuffer() );
        return str8;
    };

    switch ( nID )
    {
    case IDC_CHECK_IS_ENABLED:
    {
        g_isEnabled = uButton_GetCheck( this->m_hWnd, IDC_CHECK_IS_ENABLED );
        break;
    }
    case IDC_TEXTBOX_STATE:
    {
        g_stateQuery = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_DETAILS:
    {
        g_detailsQuery = getDlgItemText();
        break;
    }
    case IDC_RADIO_IMG_LIGHT:
    case IDC_RADIO_IMG_DARK:
    case IDC_RADIO_IMG_DISABLED:
    {
        if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_LIGHT ) )
        {
            g_largeImageSettings = static_cast<uint8_t>( ImageSetting::Light );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_DARK ) )
        {
            g_largeImageSettings = static_cast<uint8_t>( ImageSetting::Dark );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_DISABLED ) )
        {
            g_largeImageSettings = static_cast<uint8_t>( ImageSetting::Disabled );
        }
        break;
    }
    case IDC_RADIO_TIME_ELAPSED:
    case IDC_RADIO_TIME_REMAINING:
    case IDC_RADIO_TIME_DISABLED:
    {
        if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_ELAPSED ) )
        {
            g_timeSettings = static_cast<uint8_t>( TimeSetting::Elapsed );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_REMAINING ) )
        {
            g_timeSettings = static_cast<uint8_t>( TimeSetting::Remaining );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_DISABLED ) )
        {
            g_timeSettings = static_cast<uint8_t>( TimeSetting::Disabled );
        }
        break;
    }
    default:
        break;
    }

    OnChanged();
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

    uButton_SetCheck( this->m_hWnd, IDC_CHECK_IS_ENABLED, g_isEnabled.GetCurrentValue() );

    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_STATE, g_stateQuery.GetCurrentValue() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_DETAILS, g_detailsQuery.GetCurrentValue() );

    const auto imageSettings = static_cast<ImageSetting>( g_largeImageSettings.GetCurrentValue() );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_LIGHT, ImageSetting::Light == imageSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_DARK, ImageSetting::Dark == imageSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_DISABLED, ImageSetting::Disabled == imageSettings );

    const auto timeSettings = static_cast<TimeSetting>( g_timeSettings.GetCurrentValue() );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_ELAPSED, TimeSetting::Elapsed == timeSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_REMAINING, TimeSetting::Remaining == timeSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_DISABLED, TimeSetting::Disabled == timeSettings );
}

} // namespace drp::ui
