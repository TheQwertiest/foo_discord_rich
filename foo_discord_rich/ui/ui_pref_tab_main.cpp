#include <stdafx.h>

#include "ui_pref_tab_main.h"

#include <discord/discord_integration.h>
#include <ui/ui_pref_tab_manager.h>

#include <qwr/fb2k_config_ui_option.h>

namespace drp::ui
{

using namespace config;

PreferenceTabMain::PreferenceTabMain( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , isEnabled_( config::isEnabled )
    , topTextQuery_( config::topTextQuery )
    , middleTextQuery_( config::middleTextQuery )
    , bottomTextQuery_( config::bottomTextQuery )
    , enableAlbumArtFetch_( config::enableAlbumArtFetch )
    , largeImageSettings_( config::largeImageSettings, { { ImageSetting::Light, IDC_RADIO_IMG_LIGHT }, { ImageSetting::Dark, IDC_RADIO_IMG_DARK }, { ImageSetting::Disabled, IDC_RADIO_IMG_DISABLED } } )
    , smallImageSettings_( config::smallImageSettings, { { ImageSetting::Light, IDC_RADIO_PLAYBACK_IMG_LIGHT }, { ImageSetting::Dark, IDC_RADIO_PLAYBACK_IMG_DARK }, { ImageSetting::Disabled, IDC_RADIO_PLAYBACK_IMG_DISABLED } } )
    , timeSettings_( config::timeSettings, { { TimeSetting::Elapsed, IDC_RADIO_TIME_ELAPSED }, { TimeSetting::Remaining, IDC_RADIO_TIME_REMAINING }, { TimeSetting::Disabled, IDC_RADIO_TIME_DISABLED } } )
    , disableWhenPaused_( config::disableWhenPaused )
    , swapSmallImages_( config::swapSmallImages )
    , ddxOptions_( {
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( isEnabled_, IDC_CHECK_IS_ENABLED ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( topTextQuery_, IDC_EDIT_TOP_TEXT ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( middleTextQuery_, IDC_EDIT_MIDDLE_TEXT ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( bottomTextQuery_, IDC_EDIT_BOTTOM_TEXT ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( enableAlbumArtFetch_, IDC_CHECK_FETCH_ALBUM_ART ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( largeImageSettings_, std::initializer_list<int>{ IDC_RADIO_IMG_LIGHT, IDC_RADIO_IMG_DARK, IDC_RADIO_IMG_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( smallImageSettings_, std::initializer_list<int>{ IDC_RADIO_PLAYBACK_IMG_LIGHT, IDC_RADIO_PLAYBACK_IMG_DARK, IDC_RADIO_PLAYBACK_IMG_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( timeSettings_, std::initializer_list<int>{ IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_REMAINING, IDC_RADIO_TIME_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( disableWhenPaused_, IDC_CHECK_DISABLE_WHEN_PAUSED ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( swapSmallImages_, IDC_CHECK_SWAP_STATUS ),
      } )
{
    isAlbumArtFetchOverriden_ = config::enableArtUpload;
}

PreferenceTabMain::~PreferenceTabMain()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Revert();
    }
}

HWND PreferenceTabMain::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabMain::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabMain::Name() const
{
    return L"Main";
}

void PreferenceTabMain::OnUiChangeRequest( int nID, bool enable )
{
    if ( nID == IDC_CHECK_FETCH_ALBUM_ART )
    {
        enableAlbumArtFetch_.SetValue( enable );
        isAlbumArtFetchOverriden_ = !enable;
    }
}

t_uint32 PreferenceTabMain::GetState()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | preferences_state::dark_mode_supported | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabMain::Apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }
}

void PreferenceTabMain::Reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }

    isAlbumArtFetchOverriden_ = false;
    DoFullDdxToUi();
}

BOOL PreferenceTabMain::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    darkModeHooks_.AddDialogWithControls( m_hWnd );

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    DoFullDdxToUi();

    // Disable duration options, since they are currently not implemented by Discord API
    for ( auto id: { IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_REMAINING, IDC_RADIO_TIME_DISABLED } )
    {
        CButton( GetDlgItem( id ) ).EnableWindow( false );
    }

    CButton( GetDlgItem( IDC_CHECK_FETCH_ALBUM_ART ) ).EnableWindow( !isAlbumArtFetchOverriden_ );

    helpUrl_.SetHyperLinkExtendedStyle( HLINK_UNDERLINED | HLINK_COMMANDBUTTON );
    helpUrl_.SetToolTipText( L"Title formatting help" );
    helpUrl_.SubclassWindow( GetDlgItem( IDC_LINK_FORMAT_HELP ) );

    return TRUE; // set focus to default control
}

void PreferenceTabMain::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    auto it = std::find_if( ddxOptions_.begin(), ddxOptions_.end(), [nID]( auto& val ) {
        return val->Ddx().IsMatchingId( nID );
    } );

    if ( ddxOptions_.end() != it )
    {
        ( *it )->Ddx().ReadFromUi();
    }

    OnChanged();
}

void PreferenceTabMain::OnHelpUrlClick( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    standard_commands::main_titleformat_help();
}

void PreferenceTabMain::OnChanged()
{
    pParent_->OnDataChanged();
}

void PreferenceTabMain::DoFullDdxToUi()
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
