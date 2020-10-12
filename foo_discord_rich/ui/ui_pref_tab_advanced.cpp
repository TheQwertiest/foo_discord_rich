#include <stdafx.h>

#include "ui_pref_tab_advanced.h"

#include <discord/discord_impl.h>
#include <fb2k/config.h>
#include <qwr/fb2k_config_ui_option.h>
#include <ui/ui_pref_tab_manager.h>

namespace drp::ui
{

using namespace config;

PreferenceTabAdvanced::PreferenceTabAdvanced( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , opt0_( config::g_discordAppToken )
    , opt1_( config::g_largeImageId_Light )
    , opt2_( config::g_largeImageId_Dark )
    , opt3_( config::g_playingImageId_Light )
    , opt4_( config::g_playingImageId_Dark )
    , opt5_( config::g_pausedImageId_Light )
    , opt6_( config::g_pausedImageId_Dark )
    , ddxOptions_( {
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt0_, IDC_TEXTBOX_APP_TOKEN ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt1_, IDC_TEXTBOX_LARGE_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt2_, IDC_TEXTBOX_LARGE_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt3_, IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt4_, IDC_TEXTBOX_SMALL_PLAYING_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt5_, IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( opt6_, IDC_TEXTBOX_SMALL_PAUSED_DARK_ID ),
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

t_uint32 PreferenceTabAdvanced::get_state()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabAdvanced::apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }
}

void PreferenceTabAdvanced::reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }

    UpdateUiFromCfg();
}

BOOL PreferenceTabAdvanced::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    UpdateUiFromCfg();

    return TRUE; // set focus to default control
}

void PreferenceTabAdvanced::OnEditChange( UINT uNotifyCode, int nID, CWindow wndCtl )
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

void PreferenceTabAdvanced::OnChanged()
{
    pParent_->OnDataChanged();
}

void PreferenceTabAdvanced::UpdateUiFromCfg()
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
