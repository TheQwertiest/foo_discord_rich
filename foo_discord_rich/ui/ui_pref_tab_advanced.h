#pragma once

#include <ui/ui_itab.h>
#include <ui/internal/ui_cfg_wrap.h>
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
    std::array<std::unique_ptr<IUiCfgWrap>, 7> configs_;
};

} // namespace drp::ui
