/******************************************************************************
about_dialog.cpp - Callback function for the "About" dialog window
*******************************************************************************
Winphonic
By Kevin Perry
https://k-perry.github.io
-------------------------------------------------------------------------------
MIT License

Copyright (c) 2018, Kevin Perry

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "about_dialog.h"

#define BTN_CLOSE_ID	1

static HFONT g_version_font;
static HWND g_btn_close;


INT_PTR CALLBACK AboutDlgProc(HWND hwnd_dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	AboutDlgState* about_state = {};
	if (msg == WM_INITDIALOG)
	{
		about_state = reinterpret_cast<AboutDlgState*>(lParam);
		SetWindowLongPtr(hwnd_dlg, DWLP_USER, (LONG_PTR)about_state);
	}
	else
	{
		about_state = reinterpret_cast<AboutDlgState*>(GetWindowLongPtr(hwnd_dlg, DWLP_USER));
	}

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			if (about_state)
			{
				HINSTANCE instance = GetModuleHandle(0);
				const int btn_width = 80;

				// Compute the center of the dialog
				RECT dlg_rect;
				GetClientRect(hwnd_dlg, &dlg_rect);
				const int dlg_width = dlg_rect.right - dlg_rect.left;
				int btn_left = (dlg_width - btn_width) / 2;

				g_version_font = GetFont("Segoe UI", 12, true, false, false);
				SendDlgItemMessage(hwnd_dlg, IDC_VERSION, WM_SETFONT, (WPARAM)g_version_font, true);

				g_btn_close = CreateTextButton("Close", "Segoe UI", 12, about_state->text_color,
					about_state->btn_normal_color, about_state->btn_hover_color, about_state->btn_pressed_color, 
					hwnd_dlg, (HMENU)BTN_CLOSE_ID, instance, btn_left, 165, btn_width, 35);

			}

			return TRUE;
		} break;

		case WM_CTLCOLORDLG:
		{
			if (about_state)
				return (INT_PTR)about_state->bg_brush;
			else
				return 0;
		} break;

		case WM_CTLCOLORSTATIC:
		{
			if (about_state)
			{
				HDC dc = (HDC)wParam;
				SetTextColor(dc, about_state->text_color);
				SetBkMode(dc, TRANSPARENT);
				return (INT_PTR)about_state->bg_brush;
			}
			return 0;
		} break;

		case WM_CLOSE:
		{
			DestroyWindow(hwnd_dlg);
		} break;

		case WM_DESTROY:
		{
			FreeMemory(about_state);
			if (g_version_font)
				DeleteObject(g_version_font);
			return FALSE;
		} break;

		case WM_NOTIFY:
		{
			const LPNMHDR msg_info = (LPNMHDR)lParam;
			if (msg_info->code == NM_CLICK || msg_info->code == NM_RETURN)
			{
				if (msg_info->idFrom == IDC_HOMEPAGE)
				{
					ShellExecute(hwnd_dlg, "open", "https://k-perry.github.io", 0, 0, SW_SHOWNORMAL);
				}
				else if (msg_info->idFrom == IDC_BASS)
				{
					ShellExecute(hwnd_dlg, "open", "http://www.un4seen.com/bass.html", 0, 0, SW_SHOWNORMAL);
				}
				else if (msg_info->idFrom == IDC_SOURCECODE)
				{
					ShellExecute(hwnd_dlg, "open", "https://github.com/k-perry/winphonic", 0, 0, SW_SHOWNORMAL);
				}
				else if (msg_info->idFrom == IDC_BUGREPORTS)
				{
					ShellExecute(hwnd_dlg, "open", "mailto:winphonic@gmail.com", 0, 0, SW_SHOWNORMAL);
				}
				return TRUE;
			}
		} break;

		case WM_COMMAND:
		{
			// Dialogs don't get WM_KEYDOWN events, so use this hack to detect when user hits
			// ESCAPE key.  See https://stackoverflow.com/a/7231407
			if ((GetAsyncKeyState(VK_ESCAPE) & SHRT_MIN) || (LOWORD(wParam) == BTN_CLOSE_ID))
			{
				DestroyWindow(hwnd_dlg);
			}
		} break;

	}
	return FALSE;		// Return FALSE for messages not processed
}