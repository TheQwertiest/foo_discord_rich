#pragma once

#include <fb2k/config.h>
#include <qwr/fb2k_config_ui_option.h>
#include <qwr/ui_ddx_option.h>
#include <ui/ui_itab.h>

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
        COMMAND_HANDLER_EX( IDC_LINK_FORMAT_HELP, BN_CLICKED, OnHelpUrlClick )
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
    void OnHelpUrlClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void UpdateUiFromCfg();

private:
    PreferenceTabManager* pParent_ = nullptr;

    qwr::ui::UiOption<decltype( config::g_isEnabled )> opt0_;
    qwr::ui::UiOption<decltype( config::g_stateQuery )> opt1_;
    qwr::ui::UiOption<decltype( config::g_detailsQuery )> opt2_;
    qwr::ui::UiOption<decltype( config::g_largeImageSettings )> opt3_;
    qwr::ui::UiOption<decltype( config::g_smallImageSettings )> opt4_;
    qwr::ui::UiOption<decltype( config::g_timeSettings )> opt5_;
    qwr::ui::UiOption<decltype( config::g_disableWhenPaused )> opt6_;
    qwr::ui::UiOption<decltype( config::g_swapSmallImages )> opt7_;
    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 8> ddxOptions_;

    CHyperLink helpUrl_;
};

} // namespace drp::ui
