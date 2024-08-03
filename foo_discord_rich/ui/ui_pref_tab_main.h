#pragma once

#include <fb2k/config.h>
#include <ui/ui_itab.h>

#include <resource.h>

#include <qwr/fb2k_config_ui_option.h>
#include <qwr/macros.h>
#include <qwr/ui_ddx_option.h>

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
        COMMAND_HANDLER_EX( IDC_TEXTBOX_TOP_TEXT, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_MIDDLE_TEXT, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_TEXTBOX_BOTTOM_TEXT, EN_CHANGE, OnEditChange )
        COMMAND_HANDLER_EX( IDC_CHECK_FETCH_ALBUM_ART, BN_CLICKED, OnEditChange )
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

#define SPTF_DEFINE_UI_OPTION( name ) \
    qwr::ui::UiOption<decltype( config::name )> name##_;

#define SPTF_DEFINE_UI_OPTIONS( ... ) \
    QWR_EXPAND( QWR_PASTE( SPTF_DEFINE_UI_OPTION, __VA_ARGS__ ) )

    // clang-format off
    SPTF_DEFINE_UI_OPTIONS( isEnabled,
                            topTextQuery,
                            middleTextQuery,
                            bottomTextQuery,
                            fetchAlbumArt,
                            largeImageSettings,
                            smallImageSettings,
                            timeSettings,
                            disableWhenPaused,
                            swapSmallImages )
    // clang-format on

#undef SPTF_DEFINE_OPTIONS
#undef SPTF_DEFINE_OPTION

    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 10> ddxOptions_;

    CHyperLink helpUrl_;
};

} // namespace drp::ui
