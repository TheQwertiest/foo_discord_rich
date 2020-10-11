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

class PreferenceTabAdvanced
    : public CDialogImpl<PreferenceTabAdvanced>
    , public CWinDataExchange<PreferenceTabAdvanced>
    , public ITab
{
public:
    enum
    {
        IDD = IDD_PREFS_ADVANCED_TAB
    };

    BEGIN_MSG_MAP( PreferenceTabAdvanced )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_APP_TOKEN, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_LARGE_LIGHT_ID, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_LARGE_DARK_ID, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_SMALL_PLAYING_DARK_ID, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_SMALL_PAUSED_DARK_ID, EN_CHANGE, OnEditChange )
    END_MSG_MAP()

public:
    PreferenceTabAdvanced( PreferenceTabManager* pParent );
    ~PreferenceTabAdvanced() override;

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

    qwr::ui::UiOptionTuple<
        decltype( config::g_discordAppToken ),
        decltype( config::g_largeImageId_Light ),
        decltype( config::g_largeImageId_Dark ),
        decltype( config::g_playingImageId_Light ),
        decltype( config::g_playingImageId_Dark ),
        decltype( config::g_pausedImageId_Light ),
        decltype( config::g_pausedImageId_Dark )>
        options_;
    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 7> ddxOptions_;
};

} // namespace drp::ui
