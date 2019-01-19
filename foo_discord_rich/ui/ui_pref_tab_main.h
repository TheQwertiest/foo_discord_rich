#pragma once

#include <utils/cfg_wrap.h>
#include <ui/ui_itab.h>
#include <component_defines.h>
#include <resource.h>

#include <array>

namespace drp::ui
{

class PreferenceTabManager;

class PreferenceTabMain
    : public CDialogImpl<PreferenceTabMain>
    , public CWinDataExchange<PreferenceTabMain>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_PREFS_MAIN_TAB
    };

    BEGIN_MSG_MAP( PreferenceTabMain )
    MSG_WM_INITDIALOG( OnInitDialog )
    COMMAND_HANDLER_EX( IDC_CHECK_IS_ENABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_STATE, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_TEXTBOX_DETAILS, EN_CHANGE, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_LIGHT, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DARK, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_IMG_DISABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_PLAYBACK_IMG_LIGHT, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_PLAYBACK_IMG_DARK, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_PLAYBACK_IMG_DISABLED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_ELAPSED, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_REMAINING, BN_CLICKED, OnEditChange )
    COMMAND_HANDLER_EX( IDC_RADIO_TIME_DISABLED, BN_CLICKED, OnEditChange )
    END_MSG_MAP()

public:
    PreferenceTabMain( PreferenceTabManager* pParent );
    ~PreferenceTabMain() override;
    
    // IUiTab
    HWND CreateTab( HWND hParent ) override;
    CDialogImplBase& Dialog() override;
    const wchar_t* Name() const override;
    t_uint32 get_state() override;
    void apply() override;
    void reset() override;

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void UpdateUiFromCfg();

private:
    PreferenceTabManager* pParent_ = nullptr;
    std::array<std::reference_wrapper<utils::ICfgWrap>, 6> configs_;
};

} // namespace drp::ui
