// This file is part of Wagic Syntax Plugin.
// 
// Copyright (C)2023 Vitty85 <https://github.com/Vitty85>
// 
// Wagic Syntax Plugin is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <WindowsX.h>
#include "PluginInterface.h"
#include "resource.h"
#include "Hyperlinks.h"
#include "Version.h"

#ifdef _WIN64
#define BITNESS L"(64 bit)"
#else
#define BITNESS L"(32 bit)"
#endif

INT_PTR CALLBACK abtDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			ConvertStaticToHyperlink(hwndDlg, IDC_GITHUB);
			ConvertStaticToHyperlink(hwndDlg, IDC_README);
			Edit_SetText(GetDlgItem(hwndDlg, IDC_VERSION), L"WagicSyntaxPlugin v" VERSION_TEXT L" " BITNESS);
			return true;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					DestroyWindow(hwndDlg);
					return true;
				case IDC_GITHUB:
					ShellExecute(hwndDlg, L"open", L"https://github.com/Vitty85/WagicSyntaxPlugin/", NULL, NULL, SW_SHOWNORMAL);
					return true;
			}
		case WM_DESTROY:
			DestroyWindow(hwndDlg);
			return true;
		}
	return false;
}

void ShowAboutDialog(HINSTANCE hInstance, const wchar_t *lpTemplateName, HWND hWndParent) {
	HWND hSelf = CreateDialogParam((HINSTANCE)hInstance, lpTemplateName, hWndParent, abtDlgProc, NULL);

	// Go to center
	RECT rc;
	GetClientRect(hWndParent, &rc);
	POINT center;
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	center.x = rc.left + w / 2;
	center.y = rc.top + h / 2;
	ClientToScreen(hWndParent, &center);

	RECT dlgRect;
	GetClientRect(hSelf, &dlgRect);
	int x = center.x - (dlgRect.right - dlgRect.left) / 2;
	int y = center.y - (dlgRect.bottom - dlgRect.top) / 2;

	SetWindowPos(hSelf, HWND_TOP, x, y, (dlgRect.right - dlgRect.left), (dlgRect.bottom - dlgRect.top), SWP_SHOWWINDOW);
}
