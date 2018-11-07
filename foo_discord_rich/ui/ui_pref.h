#pragma once

#include <utils/cfg_wrap.h>
#include <component_defines.h>
#include <resource.h>

#include <array>

namespace drp::ui
{

class CDialogPref
    : public CDialogImpl<CDialogPref>
    , public CWinDataExchange<CDialogPref>
    , public preferences_page_instance
{
public:
    enum
    {
        IDD = IDD_DIALOG_PREFERENCE
    };

    BEGIN_MSG_MAP( CDialogPref )
    MSG_WM_INITDIALOG( OnInitDialog )
    COMMAND_HANDLER_EX( IDC_CHECK_IS_ENABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_STATE, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_DETAILS, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_LIGHT, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DARK, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DISABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_ELAPSED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_REMAINING, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_DISABLED, BN_CLICKED, OnEditChange )
    END_MSG_MAP()

public:
    CDialogPref( preferences_page_callback::ptr callback );
    ~CDialogPref() override;

    // preferences_page_instance
    HWND get_wnd() override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void UpdateUiFromCfg();

private:
    preferences_page_callback::ptr callback_;
    std::array<std::reference_wrapper<utils::ICfgWrap>, 5> configs_;
};

} // namespace drp::ui
