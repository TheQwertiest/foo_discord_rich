#include <stdafx.h>

#include "ui_pref_tab_advanced.h"

#include <artwork/fetcher.h>
#include <discord/discord_integration.h>
#include <fb2k/config.h>
#include <ui/ui_pref_tab_manager.h>

#include <qwr/fb2k_config_ui_option.h>

namespace fs = std::filesystem;

namespace drp::ui
{

using namespace config;

PreferenceTabAdvanced::PreferenceTabAdvanced( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , discordAppToken_( config::discordAppToken )
    , largeImageId_Light_( config::largeImageId_Light )
    , largeImageId_Dark_( config::largeImageId_Dark )
    , playingImageId_Light_( config::playingImageId_Light )
    , playingImageId_Dark_( config::playingImageId_Dark )
    , pausedImageId_Light_( config::pausedImageId_Light )
    , pausedImageId_Dark_( config::pausedImageId_Dark )
    , enableArtUpload_( config::enableArtUpload )
    , artUploadCmd_( config::artUploadCmd )
    , artUploadPinQuery_( config::artUploadPinQuery )
    , ddxOptions_( {
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( discordAppToken_, IDC_EDIT_APP_TOKEN ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( largeImageId_Light_, IDC_EDIT_LARGE_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( largeImageId_Dark_, IDC_EDIT_LARGE_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( playingImageId_Light_, IDC_EDIT_SMALL_PLAYING_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( playingImageId_Dark_, IDC_EDIT_SMALL_PLAYING_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( pausedImageId_Light_, IDC_EDIT_SMALL_PAUSED_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( pausedImageId_Dark_, IDC_EDIT_SMALL_PAUSED_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( enableArtUpload_, IDC_CHECK_UPLOAD_ART ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( artUploadCmd_, IDC_EDIT_UPLOAD_COMMAND ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( artUploadPinQuery_, IDC_EDIT_UPLOAD_ART_PIN_QUERY ),
      } )
{
}

PreferenceTabAdvanced::~PreferenceTabAdvanced()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Revert();
    }
}

HWND PreferenceTabAdvanced::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabAdvanced::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabAdvanced::Name() const
{
    return L"Advanced";
}

void PreferenceTabAdvanced::OnUiChangeRequest( int nID, bool enable )
{
}

t_uint32 PreferenceTabAdvanced::GetState()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | preferences_state::dark_mode_supported | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabAdvanced::Apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }
}

void PreferenceTabAdvanced::Reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }

    DoFullDdxToUi();
}

BOOL PreferenceTabAdvanced::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    darkModeHooks_.AddDialogWithControls( m_hWnd );

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    DoFullDdxToUi();

    helpUrl_.SetHyperLinkExtendedStyle( HLINK_UNDERLINED | HLINK_COMMANDBUTTON );
    helpUrl_.SetToolTipText( L"View artwork uploader documentation" );
    helpUrl_.SubclassWindow( GetDlgItem( IDC_LINK_ART_UPLOADER_HELP ) );

    return TRUE; // set focus to default control
}

void PreferenceTabAdvanced::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto it = std::find_if( ddxOptions_.begin(), ddxOptions_.end(), [nID]( auto& val ) {
        return val->Ddx().IsMatchingId( nID );
    } );

    if ( ddxOptions_.end() != it )
    {
        ( *it )->Ddx().ReadFromUi();
    }

    OnChanged();

    if ( nID == IDC_CHECK_UPLOAD_ART )
    {
        pParent_->RequestUiChange( IDC_CHECK_FETCH_ALBUM_ART, !enableArtUpload_.GetCurrentValue() );
    }
}

void PreferenceTabAdvanced::OnHelpUrlClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    ShellExecute( nullptr, L"open", L"" DRP_HOMEPAGE "/docs/advanced_users/art_upload", nullptr, nullptr, SW_SHOW );
}

void PreferenceTabAdvanced::OnLoadCacheClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    try
    {
        ArtworkFetcher::Get().LoadCache( true );
    }
    catch ( const std::exception& e )
    {
        const auto errorMsg = fmt::format( "Failed to read art cache:\n{}", e.what() );
        popup_message::g_show( errorMsg.c_str(), "Loading art cache" );
    }
}

void PreferenceTabAdvanced::OnSaveCacheClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    ArtworkFetcher::Get().SaveCache();
}

void PreferenceTabAdvanced::OnOpenCacheFolderClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    const auto& cachePath = ArtworkFetcher::GetCacheFilePath();
    if ( !fs::exists( cachePath ) )
    {
        ShellExecute( nullptr,
                      L"explore",
                      cachePath.parent_path().c_str(),
                      nullptr,
                      nullptr,
                      SW_SHOWNORMAL );
    }
    else
    {
        PIDLIST_ABSOLUTE pidl{};
        SHParseDisplayName( cachePath.c_str(), nullptr, &pidl, 0, 0 );
        SHOpenFolderAndSelectItems( pidl, 0, nullptr, 0 );
        CoTaskMemFree( pidl );
    }
}

void PreferenceTabAdvanced::OnChanged()
{
    pParent_->OnDataChanged();
}

void PreferenceTabAdvanced::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().WriteToUi();
    }
}

} // namespace drp::ui
