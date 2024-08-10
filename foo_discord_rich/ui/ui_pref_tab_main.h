#pragma once

#include <fb2k/config.h>
#include <ui/ui_itab.h>

#include <resource.h>

#include <foobar2000/SDK/coreDarkMode.h>
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
        COMMAND_HANDLER_EX( IDC_LINK_FORMAT_HELP, BN_CLICKED, OnHelpUrlClick )
        COMMAND_HANDLER_EX( IDC_CHECK_IS_ENABLED, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_TOP_TEXT, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_MIDDLE_TEXT, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_BOTTOM_TEXT, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_FETCH_ALBUM_ART, BN_CLICKED, OnDdxUiChange )
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_IMG_LIGHT, IDC_RADIO_IMG_DISABLED, BN_CLICKED, OnDdxUiChange )
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_PLAYBACK_IMG_LIGHT, IDC_RADIO_PLAYBACK_IMG_DISABLED, BN_CLICKED, OnDdxUiChange )
        COMMAND_RANGE_CODE_HANDLER_EX( IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_DISABLED, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_DISABLE_WHEN_PAUSED, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_CHECK_SWAP_STATUS, BN_CLICKED, OnDdxUiChange )
    END_MSG_MAP()

public:
    PreferenceTabMain( PreferenceTabManager* pParent );
    ~PreferenceTabMain() override;

    // IUiTab
    HWND CreateTab( HWND hParent ) override;
    CDialogImplBase& Dialog() override;
    const wchar_t* Name() const override;
    void OnUiChangeRequest( int nID, bool enable ) override;
    t_uint32 GetState() override;
    void Apply() override;
    void Reset() override;

private:
    BOOL OnInitDialog( HWND hwndFocus, LPARAM lParam );
    void OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnHelpUrlClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void DoFullDdxToUi();

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
                            enableAlbumArtFetch,
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

    bool isAlbumArtFetchOverriden_ = false;

    fb2k::CCoreDarkModeHooks darkModeHooks_;
};

} // namespace drp::ui
