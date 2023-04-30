#include "stdafx.h"

#include <memory>

#include "fb2k/artwork_metadb.h"
#include "fb2k/metadb_helpers.h"
#include "url_input.h"

#include "qwr/string_helpers.h"


namespace drp::ui {
InputDialog::InputDialog(metadb_handle_list_cref data) {
	m_sharedData = std::make_shared< sharedData_t > ();
	m_sharedData->items = data;
	m_sharedData->url = "";
	m_sharedData->owner = this;
}

void InputDialog::OpenDialog(metadb_handle_list_cref data) {
	InputDialog(data).DoModal(core_api::get_main_window());
}

BOOL InputDialog::OnInitDialog(CWindow, LPARAM) {
	uSetWindowText(*this, PFC_string_formatter() << "Selected " << m_sharedData->items.get_size() << " tracks.");
	// 256 characters (32 bytes) is the limit for largeImageKey according to discord_rpc.h
	SendMessage(GetDlgItem(IDC_TEXTBOX_URL), EM_SETLIMITTEXT, 256, 0);
	GotoDlgCtrl(GetDlgItem(IDC_TEXTBOX_URL));

	// Dialog box centering
	// https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes?redirectedfrom=MSDN#initializing-a-dialog-box
	const auto parent = GetParent();
	RECT pRect;
	RECT cRect;
	RECT rc;
	parent.GetWindowRect(&pRect);
	GetWindowRect(&cRect);
	CopyRect(&rc, &pRect);

	OffsetRect(&cRect, -cRect.left, -cRect.top);
	OffsetRect(&rc, -rc.left, -rc.top);
	OffsetRect(&rc, -cRect.right, -cRect.bottom);

	SetWindowPos(HWND_TOP,
	 pRect.left + (rc.right / 2),
	 pRect.top + (rc.bottom / 2),
	 0, 0,          // Ignores size arguments.
	 SWP_NOSIZE);

	ShowWindow(SW_SHOW);

	return TRUE; // system should set focus
}

void InputDialog::OnDestroy() {
	cancelTask();
}

void InputDialog::OnCancel(UINT, int, CWindow) {
	EndDialog(IDOK);
}

void InputDialog::OnOK(UINT, int, CWindow) {
	startTask();
}

void InputDialog::startTask() {
	auto aborter = pfc::replace_null_t(m_aborter);
	if (aborter) return;

	auto shared = m_sharedData;
	aborter = std::make_shared<abort_callback_impl>();
	m_aborter = aborter;
	shared->url = uGetDlgItemText(*shared->owner, IDC_TEXTBOX_URL).c_str();

	fb2k::splitTask( [aborter, shared] {
		try {
			work( shared, aborter );
			mainThreadOp(std::make_shared<abort_callback_impl>(), [shared] { shared->owner->EndDialog(IDOK); });
		} catch(exception_aborted) {
			mainThreadOp(std::make_shared<abort_callback_impl>(), [shared] { shared->owner->EndDialog(IDOK); });
		} catch(std::exception const & e) {
			// should not really get here
            FB2K_console_formatter() << "Failed to set album url from manual input. " << e;
			mainThreadOp(std::make_shared<abort_callback_impl>(), [shared] { shared->owner->EndDialog(-1); });
		}
	} );
}

void InputDialog::mainThreadOp(std::shared_ptr<abort_callback_impl> aborter, std::function<void ()> op ) {
	aborter->check(); // are we getting aborted?
	fb2k::inMainThread( [=] {
		if ( aborter->is_set() ) return;
		op();
	} );
}

void InputDialog::work( std::shared_ptr<sharedData_t> shared, std::shared_ptr<abort_callback_impl> aborter ) {
	mainThreadOp(aborter, [shared] {
		const auto tracks = shared->items;
		const pfc::avltree_t<metadb_index_hash> allHashes = getHashes(tracks);
		if (allHashes.get_count() == 0) {
			return;
		}

		if (allHashes.get_count() == 0) {
			FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Could not hash any of the tracks due to unavailable metadata, bailing";
			return;
		}

		pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
		auto url = shared->url;
		url.skip_trailing_chars(" \t");

		for (auto iter = allHashes.first(); iter.is_valid(); ++iter)
		{
			const metadb_index_hash hash = *iter;

			if (artwork_url_set( hash, url ))
			{
				lstChanged += hash;
			}
		}

		FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": " << lstChanged.get_count() << " entries updated";
		if (lstChanged.get_count() > 0) {
			// This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
			cached_index_api()->dispatch_refresh(guid::artwork_url_index, lstChanged);
		}
	} );
}

bool InputDialog::cancelTask() {
	bool ret = false;
	auto aborter = pfc::replace_null_t(m_aborter);
	if (aborter) {
		ret = true;
		aborter->set();
	}
	return ret;
}
}
