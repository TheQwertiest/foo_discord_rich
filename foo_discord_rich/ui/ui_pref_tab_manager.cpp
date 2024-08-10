#include <stdafx.h>

#include "ui_pref_tab_manager.h"

#include <discord/discord_integration.h>
#include <fb2k/config.h>
#include <ui/ui_pref_tab_advanced.h>
#include <ui/ui_pref_tab_main.h>

namespace
{

class preferences_page_impl
    : public preferences_page_v3
{
public:
    const char* get_name() override
    {
        return DRP_NAME;
    }

    GUID get_guid() override
    {
        return drp::guid::ui_pref;
    }

    GUID get_parent_guid() override
    {
        return preferences_page::guid_tools;
    }

    bool get_help_url( pfc::string_base& p_out ) override
    {
        p_out = "https://github.com/TheQwertiest/foo_discord_rich";
        return true;
    }

    preferences_page_instance::ptr instantiate( HWND parent, preferences_page_callback::ptr callback ) override
    {
        auto p = fb2k::service_new<drp::ui::PreferenceTabManager>( callback );
        p->Create( parent );
        return p;
    }
};

preferences_page_factory_t<preferences_page_impl> g_pref;

} // namespace

namespace drp::ui
{

using namespace config;

PreferenceTabManager::PreferenceTabManager( preferences_page_callback::ptr callback )
    : callback_( callback )
{
    tabs_.emplace_back( std::make_unique<PreferenceTabMain>( this ) );
    tabs_.emplace_back( std::make_unique<PreferenceTabAdvanced>( this ) );
}

PreferenceTabManager::~PreferenceTabManager()
{
}

void PreferenceTabManager::OnDataChanged()
{
    callback_->on_state_changed();
}

void PreferenceTabManager::RequestUiChange( int nId, bool enable )
{
    for ( auto& tab: tabs_ )
    {
        tab->OnUiChangeRequest( nId, enable );
    }
}

HWND PreferenceTabManager::get_wnd()
{
    return m_hWnd;
}

t_uint32 PreferenceTabManager::get_state()
{
    uint32_t state = preferences_state::resettable | preferences_state::dark_mode_supported;
    for ( auto& tab: tabs_ )
    {
        state |= tab->GetState();
    }

    return state;
}

void PreferenceTabManager::apply()
{
    for ( auto& tab: tabs_ )
    {
        tab->Apply();
    }

    OnDataChanged();
    drp::DiscordAdapter::GetInstance().OnSettingsChanged();
}

void PreferenceTabManager::reset()
{
    for ( auto& tab: tabs_ )
    {
        tab->Reset();
    }

    OnDataChanged();
}

BOOL PreferenceTabManager::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    darkModeHooks_.AddDialogWithControls( m_hWnd );

    cTabs_ = GetDlgItem( IDC_TAB_PREFS_CURRENT );

    for ( size_t i = 0; i < tabs_.size(); ++i )
    {
        cTabs_.InsertItem( static_cast<int>( i ), tabs_[i]->Name() );
    }

    cTabs_.SetCurSel( static_cast<int>( activeTabIdx_ ) );
    CreateTab();

    return TRUE; // set focus to default control
}

void PreferenceTabManager::OnParentNotify( UINT message, UINT nChildID, LPARAM lParam )
{
    if ( WM_DESTROY == message && pcCurTab_ && reinterpret_cast<HWND>( lParam ) == static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_ = nullptr;
    }
}

LRESULT PreferenceTabManager::OnSelectionChanged( LPNMHDR pNmhdr )
{
    activeTabIdx_ = TabCtrl_GetCurSel( GetDlgItem( IDC_TAB_PREFS_CURRENT ) );
    CreateTab();

    return 0;
}

LRESULT PreferenceTabManager::OnWindowPosChanged( UINT, WPARAM, LPARAM lp, BOOL& bHandled )
{
    auto lpwp = reinterpret_cast<LPWINDOWPOS>( lp );
    // Temporary workaround for various bugs that occur due to foobar2000 1.0+
    // having a dislike for destroying preference pages
    if ( lpwp->flags & SWP_HIDEWINDOW )
    {
        DestroyTab();
    }
    else if ( lpwp->flags & SWP_SHOWWINDOW && !pcCurTab_ )
    {
        CreateTab();
    }

    bHandled = FALSE;

    return 0;
}

void PreferenceTabManager::CreateTab()
{
    DestroyTab();

    RECT tabRc;

    cTabs_.GetWindowRect( &tabRc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&tabRc, 2 );

    cTabs_.AdjustRect( FALSE, &tabRc );

    if ( activeTabIdx_ >= tabs_.size() )
    {
        activeTabIdx_ = 0;
    }

    auto& pCurTab = tabs_[activeTabIdx_];
    pcCurTab_ = &pCurTab->Dialog();
    pCurTab->CreateTab( m_hWnd );

    EnableThemeDialogTexture( static_cast<HWND>( *pcCurTab_ ), ETDT_ENABLETAB );

    pcCurTab_->SetWindowPos( nullptr, tabRc.left, tabRc.top, tabRc.right - tabRc.left, tabRc.bottom - tabRc.top, SWP_NOZORDER );
    cTabs_.SetWindowPos( HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    pcCurTab_->ShowWindow( SW_SHOWNORMAL );
}

void PreferenceTabManager::DestroyTab()
{
    if ( pcCurTab_ && static_cast<HWND>( *pcCurTab_ ) )
    {
        pcCurTab_->ShowWindow( SW_HIDE );
        pcCurTab_->DestroyWindow();
    }
}

} // namespace drp::ui
