#include <stdafx.h>
#include "ui_pref_tab_manager.h"

#include <ui/ui_pref_tab_main.h>
#include <discord_impl.h>
#include <config.h>

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
        return g_guid_drp_ui_pref;
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
}

PreferenceTabManager::~PreferenceTabManager()
{
}

void PreferenceTabManager::OnDataChanged()
{
    callback_->on_state_changed();
}

HWND PreferenceTabManager::get_wnd()
{
    return m_hWnd;
}

t_uint32 PreferenceTabManager::get_state()
{
    uint32_t state = preferences_state::resettable;
    for ( auto& tab : tabs_ )
    {
        state |= tab->get_state();
    }

    return state;
}

void PreferenceTabManager::apply()
{
    for ( auto& tab : tabs_ )
    {
        tab->apply();
    }

    OnDataChanged();
    drp::DiscordHandler::GetInstance().OnSettingsChanged();
}

void PreferenceTabManager::reset()
{
    for ( auto& tab : tabs_ )
    {
        tab->reset();
    }

    OnDataChanged();
}

BOOL PreferenceTabManager::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    hTabs_ = GetDlgItem( IDC_TAB1 );

    for ( const auto& tab : tabs_ )
    {
        uTCITEM tabs{};
        tabs.mask = TCIF_TEXT;
        tabs.pszText = const_cast<char*>( tab->Name() );
        uTabCtrl_InsertItem( hTabs_, 0, &tabs );
    }

    TabCtrl_SetCurSel( hTabs_, activeTabIdx_ );
    CreateTab();

    return TRUE; // set focus to default control
}

void PreferenceTabManager::OnParentNotify( UINT message, UINT nChildID, LPARAM lParam )
{
    if ( WM_DESTROY == message && reinterpret_cast<HWND>(lParam) == hChild_ )
    {
        hChild_ = nullptr;
    }
}

LRESULT PreferenceTabManager::OnSelectionChanged( LPNMHDR pNmhdr )
{
    activeTabIdx_ = TabCtrl_GetCurSel( GetDlgItem( IDC_TAB1 ) );
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
    else if ( lpwp->flags & SWP_SHOWWINDOW && !hChild_ )
    {
        CreateTab();
    }

    bHandled = FALSE;

    return 0;
}

void PreferenceTabManager::CreateTab()
{
    DestroyTab();

    RECT tab;

    ::GetWindowRect( hTabs_, &tab );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&tab, 2 );

    TabCtrl_AdjustRect( hTabs_, FALSE, &tab );

    if ( activeTabIdx_ >= tabs_.size() )
    {
        activeTabIdx_ = 0;
    }

    hChild_ = tabs_[activeTabIdx_]->CreateTab( m_hWnd );
    assert( hChild_ );
    
    EnableThemeDialogTexture( hChild_, ETDT_ENABLETAB );

    ::SetWindowPos( hChild_, nullptr, tab.left, tab.top, tab.right - tab.left, tab.bottom - tab.top, SWP_NOZORDER );
    ::SetWindowPos( hTabs_, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    ::ShowWindow( hChild_, SW_SHOWNORMAL );
}

void PreferenceTabManager::DestroyTab()
{
    if ( hChild_ )
    {
        ::ShowWindow( hChild_, SW_HIDE );
        ::DestroyWindow( hChild_ );
        hChild_ = nullptr;
    }
}

} // namespace drp::ui
