#pragma once

#include <ui/ui_itab.h>
#include <ui/internal/ui_cfg_wrap.h>
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
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_IMG_LIGHT, IDC_RADIO_IMG_DISABLED, BN_CLICKED, OnEditChange )
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_PLAYBACK_IMG_LIGHT, IDC_RADIO_PLAYBACK_IMG_DISABLED, BN_CLICKED, OnEditChange )
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_DISABLED, BN_CLICKED, OnEditChange )
        COMMAND_HANDLER_EX( IDC_CHECK_DISABLE_WHEN_PAUSED, BN_CLICKED, OnEditChange )
        COMMAND_HANDLER_EX( IDC_CHECK_SWAP_STATUS, BN_CLICKED, OnEditChange )
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
    std::array<std::unique_ptr<IUiCfgWrap>, 8> configs_;
};

} // namespace drp::ui
