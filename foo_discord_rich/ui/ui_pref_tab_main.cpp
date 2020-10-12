#include <stdafx.h>

#include "ui_pref_tab_main.h"

#include <discord/discord_impl.h>
#include <qwr/fb2k_config_ui_option.h>
#include <ui/ui_pref_tab_manager.h>

namespace drp::ui
{

using namespace config;

PreferenceTabMain::PreferenceTabMain( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , opt0_( config::g_isEnabled )
    , opt1_( config::g_stateQuery )
    , opt2_( config::g_detailsQuery )
    , opt3_( config::g_largeImageSettings,
             { { ImageSetting::Light, IDC_RADIO_IMG_LIGHT },
               { ImageSetting::Dark, IDC_RADIO_IMG_DARK },
               { ImageSetting::Disabled, IDC_RADIO_IMG_DISABLED } } )
    , opt4_( config::g_smallImageSettings,
             { { ImageSetting::Light, IDC_RADIO_PLAYBACK_IMG_LIGHT },
               { ImageSetting::Dark, IDC_RADIO_PLAYBACK_IMG_DARK },
               { ImageSetting::Disabled, IDC_RADIO_PLAYBACK_IMG_DISABLED } } )
    , opt5_( config::g_timeSettings,
             { { TimeSetting::Elapsed, IDC_RADIO_TIME_ELAPSED },
               { TimeSetting::Remaining, IDC_RADIO_TIME_REMAINING },
               { TimeSetting::Disabled, IDC_RADIO_TIME_DISABLED } } )
    , opt6_( config::g_disableWhenPaused )
    , opt7_( config::g_swapSmallImages )
    , ddxOptions_( {
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( opt0_, IDC_CHECK_IS_ENABLED ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt1_, IDC_TEXTBOX_STATE ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt2_, IDC_TEXTBOX_DETAILS ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( opt3_, std::initializer_list<int>{ IDC_RADIO_IMG_LIGHT, IDC_RADIO_IMG_DARK, IDC_RADIO_IMG_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( opt4_, std::initializer_list<int>{ IDC_RADIO_PLAYBACK_IMG_LIGHT, IDC_RADIO_PLAYBACK_IMG_DARK, IDC_RADIO_PLAYBACK_IMG_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_RadioRange>( opt5_, std::initializer_list<int>{ IDC_RADIO_TIME_ELAPSED, IDC_RADIO_TIME_REMAINING, IDC_RADIO_TIME_DISABLED } ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( opt6_, IDC_CHECK_DISABLE_WHEN_PAUSED ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( opt7_, IDC_CHECK_SWAP_STATUS ),
      } )
{
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

t_uint32 PreferenceTabMain::get_state()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabMain::apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }
}

void PreferenceTabMain::reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabMain::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    UpdateUiFromCfg();

    helpUrl_.SetHyperLinkExtendedStyle( HLINK_UNDERLINED | HLINK_COMMANDBUTTON );
    helpUrl_.SetToolTipText( L"Title formatting help" );
    helpUrl_.SubclassWindow( GetDlgItem( IDC_LINK_FORMAT_HELP ) );

    return TRUE; // set focus to default control
}

void PreferenceTabMain::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
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

void PreferenceTabMain::UpdateUiFromCfg()
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
