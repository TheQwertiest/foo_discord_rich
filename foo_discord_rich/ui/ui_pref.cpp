#include <stdafx.h>
#include "ui_pref.h"

#include <discord_impl.h>

namespace
{

class preferences_page_impl
    : public preferences_page_v3
{
public:
    const char* get_name() override
    {
        return DRP_NAME;
    }

    GUID get_guid() override
    {
        return g_guid_drp_ui_pref;
    }

    GUID get_parent_guid() override
    {
        return preferences_page::guid_tools;
    }

    bool get_help_url( pfc::string_base& p_out ) override
    {
        p_out = "https://github.com/TheQwertiest/foo_discord_rich";
        return true;
    }

    preferences_page_instance::ptr instantiate( HWND parent, preferences_page_callback::ptr callback ) override
    {
        auto p = fb2k::service_new<drp::ui::CDialogPref>( callback );
        p->Create( parent );
        return p;
    }
};

preferences_page_factory_t<preferences_page_impl> g_pref;

} // namespace

namespace drp::ui
{

utils::CfgWrap<cfg_bool, bool>
    CDialogPref::isEnabled_( g_guid_drp_conf_is_enabled, true );

utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t>
    CDialogPref::imageSettings_( g_guid_drp_conf_image_settings, static_cast<uint8_t>( CDialogPref::ImageSetting::Light ) );

utils::CfgWrap<cfg_int_t<uint8_t>, uint8_t>
    CDialogPref::timeSettings_( g_guid_drp_conf_time_settings, static_cast<uint8_t>( CDialogPref::TimeSetting::Elapsed ) );

utils::CfgWrap<cfg_string, pfc::string8_fast>
    CDialogPref::stateQuery_( g_guid_drp_conf_state_query, "[%title%]" );

utils::CfgWrap<cfg_string, pfc::string8_fast>
    CDialogPref::detailsQuery_( g_guid_drp_conf_details_query, "[%album artist%[: %album%]]" );

CDialogPref::CDialogPref( preferences_page_callback::ptr callback )
    : m_callback( callback )
{
}

void CDialogPref::InitConfiguration()
{
    isEnabled_.Reread();
    imageSettings_.Reread();
    timeSettings_.Reread();
    stateQuery_.Reread();
    detailsQuery_.Reread();
}

HWND CDialogPref::get_wnd()
{
    return m_hWnd;
}

t_uint32 CDialogPref::get_state()
{
    const bool hasChanged =
        isEnabled_.HasChanged()
        || imageSettings_.HasChanged()
        || timeSettings_.HasChanged()
        || stateQuery_.HasChanged()
        || detailsQuery_.HasChanged();

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void CDialogPref::apply()
{
    isEnabled_.Apply();
    imageSettings_.Apply();
    timeSettings_.Apply();
    stateQuery_.Apply();
    detailsQuery_.Apply();

    OnChanged();
    drp::UpdateDiscordSettings();
}

void CDialogPref::reset()
{
    isEnabled_.Reset();
    imageSettings_.Reset();
    timeSettings_.Reset();
    stateQuery_.Reset();
    detailsQuery_.Reset();

    UpdateUiFromCfg();

    OnChanged();
}

bool CDialogPref::IsEnabled()
{
    return isEnabled_;
}

CDialogPref::ImageSetting CDialogPref::GetImageSettings()
{
    return static_cast<ImageSetting>( imageSettings_.GetSavedValue() );
}

CDialogPref::TimeSetting CDialogPref::GetTimeSetting()
{
    return static_cast<TimeSetting>( timeSettings_.GetSavedValue() );
}

const pfc::string8_fast& CDialogPref::GetStateQuery()
{
    return stateQuery_;
}

const pfc::string8_fast& CDialogPref::GetDetailsQuery()
{
    return detailsQuery_;
}

BOOL CDialogPref::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    UpdateUiFromCfg();

    return TRUE; // set focus to default control
}

void CDialogPref::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
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
        isEnabled_ = uButton_GetCheck( this->m_hWnd, IDC_CHECK_IS_ENABLED );
        break;
    }
    case IDC_TEXTBOX_STATE:
    {
        stateQuery_ = getDlgItemText();
        break;
    }
    case IDC_TEXTBOX_DETAILS:
    {
        detailsQuery_ = getDlgItemText();
        break;
    }
    case IDC_RADIO_IMG_LIGHT:
    case IDC_RADIO_IMG_DARK:
    case IDC_RADIO_IMG_DISABLED:
    {
        if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_LIGHT ) )
        {
            imageSettings_ = static_cast<uint8_t>( ImageSetting::Light );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_DARK ) )
        {
            imageSettings_ = static_cast<uint8_t>( ImageSetting::Dark );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_IMG_DISABLED ) )
        {
            imageSettings_ = static_cast<uint8_t>( ImageSetting::Disabled );
        }
        break;
    }
    case IDC_RADIO_TIME_ELAPSED:
    case IDC_RADIO_TIME_REMAINING:
    case IDC_RADIO_TIME_DISABLED:
    {
        if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_ELAPSED ) )
        {
            timeSettings_ = static_cast<uint8_t>( TimeSetting::Elapsed );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_REMAINING ) )
        {
            timeSettings_ = static_cast<uint8_t>( TimeSetting::Remaining );
        }
        else if ( uButton_GetCheck( this->m_hWnd, IDC_RADIO_TIME_DISABLED ) )
        {
            timeSettings_ = static_cast<uint8_t>( TimeSetting::Disabled );
        }
        break;
    }
    default:
        break;
    }

    OnChanged();
}

void CDialogPref::OnChanged()
{
    m_callback->on_state_changed();
}

void CDialogPref::UpdateUiFromCfg()
{
    uButton_SetCheck( this->m_hWnd, IDC_CHECK_IS_ENABLED, isEnabled_ );

    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_STATE, stateQuery_.GetSavedValue().c_str() );
    uSetDlgItemText( this->m_hWnd, IDC_TEXTBOX_DETAILS, detailsQuery_.GetSavedValue().c_str() );

    const auto imageSettings = static_cast<ImageSetting>( imageSettings_.GetSavedValue() );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_LIGHT, ImageSetting::Light == imageSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_DARK, ImageSetting::Dark == imageSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_IMG_DISABLED, ImageSetting::Disabled == imageSettings );

    const auto timeSettings = static_cast<TimeSetting>( timeSettings_.GetSavedValue() );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_ELAPSED, TimeSetting::Elapsed == timeSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_REMAINING, TimeSetting::Remaining == timeSettings );
    uButton_SetCheck( this->m_hWnd, IDC_RADIO_TIME_DISABLED, TimeSetting::Disabled == timeSettings );
}

} // namespace drp::ui
