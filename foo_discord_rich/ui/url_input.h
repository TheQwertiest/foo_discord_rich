#pragma once

#include "stdafx.h"
#include "resource.h"

#include <memory>

namespace drp::ui {
	class InputDialog;

	// This is kept a separate shared_ptr'd struct because it may outlive InputDialog instance.
	struct sharedData_t {
		pfc::string8 url;
		metadb_handle_list items;

		InputDialog * owner; // weak reference to the owning dialog; can be only used after checking the validity by other means.
	};

	class InputDialog : public CDialogImpl<InputDialog> {
	public:
		enum { IDD = IDD_CTX_MENU_INPUT };

		InputDialog(metadb_handle_list_cref data);

		static void OpenDialog(metadb_handle_list_cref data);

		BEGIN_MSG_MAP_EX(InputDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
			MSG_WM_DESTROY(OnDestroy)
		END_MSG_MAP()

	private:
		BOOL OnInitDialog(CWindow, LPARAM);

		void OnDestroy();

		void OnCancel(UINT, int, CWindow);

		void OnOK(UINT, int, CWindow);

		void startTask();

		// Run operation in main thread
		static void mainThreadOp(std::shared_ptr<abort_callback_impl> aborter, std::function<void ()> op );

		static void work( std::shared_ptr<sharedData_t> shared, std::shared_ptr<abort_callback_impl> aborter );

		bool cancelTask();

        std::shared_ptr<abort_callback_impl> m_aborter;

		// Data shared with the worker thread. It is created only once per dialog lifetime.
		std::shared_ptr< sharedData_t > m_sharedData;
	};
}
