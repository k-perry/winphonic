/******************************************************************************
text_button.cpp - Button GUI control with hover effect
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

#include "text_button.h"

static void PaintButton(HWND hwnd, TextButtonState* button_data)
{
	PAINTSTRUCT ps = {};
	HDC dc = 0;
	BITMAP bmp_info = {};
	COLORREF color = 0;

	RECT client_rect;
	GetClientRect(hwnd, &client_rect);
	const int width = client_rect.right - client_rect.left;
	const int height = client_rect.bottom - client_rect.top;

	dc = BeginPaint(hwnd, &ps);

	// Special handling for case where we click mouse and then drag off the button.
	// Require mouse button to be pressed and hovering over button in order to draw pressed color.
	if (button_data->is_mouse_pressed && button_data->is_hovered)
		color = button_data->pressed_color;
	else if (button_data->is_hovered)
		color = button_data->hover_color;
	else   // Normal button color
		color = button_data->normal_color;

	HDC mem_dc = CreateCompatibleDC(dc);		// Create memory DC
	HBITMAP mem_bmp = CreateCompatibleBitmap(dc, width, height);
	SelectObject(mem_dc, mem_bmp);

	if (!color)
		color = RGB(0, 0, 0);

	HBRUSH bg_brush = CreateSolidBrush(color);
	FillRect(mem_dc, &client_rect, bg_brush);
	DeleteObject(bg_brush);

	// Create the font, if it already hasn't been done
	if (!(button_data->font)) {
		button_data->font = GetFont(button_data->font_name, button_data->font_size, false, false, false);
	}

	SetBkMode(mem_dc, TRANSPARENT);
	SetTextColor(mem_dc, button_data->font_color);
	HFONT oldfont = (HFONT)SelectObject(mem_dc, button_data->font);
	DrawText(mem_dc, button_data->caption, -1, &client_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	SelectObject(dc, oldfont);
	BitBlt(dc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);
	DeleteDC(mem_dc);
	DeleteObject(mem_bmp);
	EndPaint(hwnd, &ps);
}


static LRESULT CALLBACK ButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TextButtonState* button_data = {};
	if (uMsg == WM_NCCREATE) {
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		// Get CREATESTRUCT by casting lParam
		CREATESTRUCT* createstruct_ptr = reinterpret_cast<CREATESTRUCT*>(lParam);

		// CREATESTRUCT.lpCreateParams is the void pointer specified as CreateWindow() parameter
		// Get pointer to TextButtonState by casting lpCreateParams.
		button_data = static_cast<TextButtonState*>(createstruct_ptr->lpCreateParams);

		// Store TextButtonState in instance data for this window handle.  This enables us to
		// retrieve it later using GetWindowLongPtr()
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)button_data);
	}
	else {
		button_data = reinterpret_cast<TextButtonState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (uMsg) {

		case WM_DESTROY:
		{
			if (!button_data)
				break;
			if (button_data->font)
				DeleteObject(button_data->font);
			FreeMemory(button_data->caption);
			FreeMemory(button_data->font_name);
			FreeMemory(button_data);
			return 0;
		} break;

		case WM_PAINT:
		{
			PaintButton(hwnd, button_data);
			return 0;
		} break;

		case WM_MOUSEMOVE:
		{
			if (!button_data)
				break;

			if (!(button_data->is_hovered)) {
				button_data->is_hovered = true;
				TRACKMOUSEEVENT mouse_event;
				mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
				mouse_event.dwFlags = TME_LEAVE | TME_HOVER;
				mouse_event.hwndTrack = hwnd;
				mouse_event.dwHoverTime = 10;
				TrackMouseEvent(&mouse_event);
			}

		} break;

		case WM_MOUSELEAVE:
		{
			if (!button_data)
				break;

			if (button_data->is_hovered) {
				button_data->is_hovered = false;
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}
			if (button_data->is_mouse_pressed) {
				//button_data->__is_pressed = false;
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}
		} break;

		case WM_MOUSEHOVER:
		{
			if (!button_data)
				break;

			if (button_data->is_hovered) {
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}

		} break;

		case WM_LBUTTONDOWN:
		{
			if (!button_data)
				break;

			button_data->is_mouse_pressed = true;
			PaintButton(hwnd, button_data);
			InvalidateWindow(hwnd);
		} break;

		case WM_LBUTTONUP:
		{
			if (!button_data)
				break;

			button_data->is_mouse_pressed = false;
			PaintButton(hwnd, button_data);
			InvalidateWindow(hwnd);
			// Send message to parent to notify of click.  Regular BUTTON controls send WM_COMMAND + BN_CLICKED.
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), BN_CLICKED), LPARAM(hwnd));
		} break;

		case WP_BM_SETCOLOR_NORMAL:
		{
			if (!button_data)
				break;

			if (wParam != NULL)
			{
				button_data->normal_color = wParam;
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}
		} break;

		case WP_BM_SETCOLOR_HOVER:
		{
			if (!button_data)
				break;

			if (wParam != NULL)
			{
				button_data->hover_color = wParam;
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}
		} break;

		case WP_BM_SETCOLOR_PRESSED:
		{
			if (!button_data)
				break;

			if (wParam != NULL)
			{
				button_data->pressed_color = wParam;
				PaintButton(hwnd, button_data);
				InvalidateWindow(hwnd);
			}
		} break;

		case WM_SETTEXT:
		{
			if (!button_data)
				break;

			FreeMemory(button_data->caption);

			char* new_caption = (char*)lParam;
			if (new_caption)
				button_data->caption = DuplicateString(new_caption);
			else
				button_data->caption = NULL;

			PaintButton(hwnd, button_data);
			InvalidateWindow(hwnd);
		}

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void TextButtonRegisterClass(void)
{
	WNDCLASS wc = {};
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ButtonProc;
	wc.lpszClassName = WP_TEXT_BTN_CLASS;
	RegisterClass(&wc);
}

void TextButtonUnregisterClass(void)
{
	UnregisterClass(WP_TEXT_BTN_CLASS, NULL);
}


HWND CreateTextButton(const char* caption, const char* font_name, int font_size, COLORREF font_color,
	COLORREF btn_normal_color, COLORREF btn_hover_color, COLORREF btn_pressed_color, HWND parent_hwnd,
	HMENU btn_id, HINSTANCE parent_inst, int x, int y, int width, int height)
{
	HWND btn_hwnd = 0;
	TextButtonState* btn_data = (TextButtonState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TextButtonState));
	if (btn_data)
	{
		if (caption)
			btn_data->caption = DuplicateString(caption);
		if (font_name)
			btn_data->font_name = DuplicateString(font_name);
		btn_data->font_size = font_size;
		btn_data->font_color = font_color;
		btn_data->normal_color = btn_normal_color;
		btn_data->hover_color = btn_hover_color;
		btn_data->pressed_color = btn_pressed_color;
		btn_hwnd = CreateWindow(WP_TEXT_BTN_CLASS, NULL, WS_VISIBLE | WS_CHILD, x, y, width, height,
			parent_hwnd, btn_id, parent_inst, btn_data);
	}
	return btn_hwnd;
}
