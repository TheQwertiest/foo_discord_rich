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
    , discordAppToken_( config::discordAppToken )
    , largeImageId_Light_( config::largeImageId_Light )
    , largeImageId_Dark_( config::largeImageId_Dark )
    , playingImageId_Light_( config::playingImageId_Light )
    , playingImageId_Dark_( config::playingImageId_Dark )
    , pausedImageId_Light_( config::pausedImageId_Light )
    , pausedImageId_Dark_( config::pausedImageId_Dark )
    , uploadArtwork_(config::uploadArtwork)
    , uploadArtworkCommand_(config::uploadArtworkCommand)
    , artworkMetadbKey_(config::artworkMetadbKey)
    , processTimeout_(config::processTimeout)
    , ddxOptions_( {
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( discordAppToken_, IDC_TEXTBOX_APP_TOKEN ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( largeImageId_Light_, IDC_TEXTBOX_LARGE_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( largeImageId_Dark_, IDC_TEXTBOX_LARGE_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( playingImageId_Light_, IDC_TEXTBOX_SMALL_PLAYING_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( playingImageId_Dark_, IDC_TEXTBOX_SMALL_PLAYING_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( pausedImageId_Light_, IDC_TEXTBOX_SMALL_PAUSED_LIGHT_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( pausedImageId_Dark_, IDC_TEXTBOX_SMALL_PAUSED_DARK_ID ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( uploadArtwork_, IDC_CHECK_UPLOAD_ARTWORK ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( uploadArtworkCommand_, IDC_TEXTBOX_ARTWORK_COMMAND ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEdit>( artworkMetadbKey_, IDC_TEXTBOX_METADB_KEY ),
          qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TextEditNum>( processTimeout_, IDC_TEXTBOX_PROCESS_TIMEOUT ),
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

   const bool needsRestart =
       ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged() && ddxOpt->Ddx().IsMatchingId(IDC_TEXTBOX_METADB_KEY);
        } );

    return preferences_state::resettable |
        ( hasChanged ? preferences_state::changed : 0 ) |
        ( needsRestart ? preferences_state::needs_restart : 0 );
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
