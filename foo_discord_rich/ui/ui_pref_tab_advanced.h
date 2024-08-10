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
        COMMAND_HANDLER_EX( IDC_EDIT_APP_TOKEN, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_LARGE_LIGHT_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_LARGE_DARK_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_SMALL_PLAYING_LIGHT_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_SMALL_PLAYING_DARK_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_SMALL_PAUSED_LIGHT_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_SMALL_PAUSED_DARK_ID, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_LINK_ART_UPLOADER_HELP, BN_CLICKED, OnHelpUrlClick )
        COMMAND_HANDLER_EX( IDC_CHECK_UPLOAD_ART, BN_CLICKED, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_UPLOAD_COMMAND, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_EDIT_UPLOAD_ART_PIN_QUERY, EN_CHANGE, OnDdxUiChange )
        COMMAND_HANDLER_EX( IDC_BUTTON_LOAD_CACHE, BN_CLICKED, OnLoadCacheClick )
        COMMAND_HANDLER_EX( IDC_BUTTON_SAVE_CACHE, BN_CLICKED, OnSaveCacheClick )
        COMMAND_HANDLER_EX( IDC_BUTTON_OPEN_CACHE_FOLDER, BN_CLICKED, OnOpenCacheFolderClick )
    END_MSG_MAP()

public:
    PreferenceTabAdvanced( PreferenceTabManager* pParent );
    ~PreferenceTabAdvanced() override;

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
    void OnLoadCacheClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnSaveCacheClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnOpenCacheFolderClick( UINT uNotifyCode, int nID, CWindow wndCtl );
    void OnChanged();
    void DoFullDdxToUi();

private:
    PreferenceTabManager* pParent_ = nullptr;

#define SPTF_DEFINE_UI_OPTION( name ) \
    qwr::ui::UiOption<decltype( config::name )> name##_;

#define SPTF_DEFINE_UI_OPTIONS( ... ) \
    QWR_EXPAND( QWR_PASTE( SPTF_DEFINE_UI_OPTION, __VA_ARGS__ ) )

    SPTF_DEFINE_UI_OPTIONS( discordAppToken,
                            largeImageId_Light,
                            largeImageId_Dark,
                            playingImageId_Light,
                            playingImageId_Dark,
                            pausedImageId_Light,
                            pausedImageId_Dark,
                            enableArtUpload,
                            artUploadCmd,
                            artUploadPinQuery )

#undef SPTF_DEFINE_OPTIONS
#undef SPTF_DEFINE_OPTION

    std::array<std::unique_ptr<qwr::ui::IUiDdxOption>, 10> ddxOptions_;

    CHyperLink helpUrl_;

    fb2k::CCoreDarkModeHooks darkModeHooks_;
};

} // namespace drp::ui
