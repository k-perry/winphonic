/******************************************************************************
img_button.cpp - Button GUI control that dispays BMP images with transparency
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

#include "img_button.h"

static void PaintButton(HWND hwnd, ImageButtonState* button_data)
{
	PAINTSTRUCT ps = {};
	HDC dc = 0;
	COLORREF color = 0;

	dc = BeginPaint(hwnd, &ps);

	// Special handling for case where we click mouse and then drag off the button.
	// Require mouse button to be pressed and hovering over button in order to draw pressed color.
	if (button_data->is_mouse_pressed && button_data->is_hovered)
		color = button_data->pressed_color;
	else if (button_data->is_hovered)
		color = button_data->hover_color;
	else   // Normal button color
		color = button_data->normal_color;

	if (!color)
		color = RGB(0, 0, 0);

	// If image hasn't already been loaded, load it.
	if (!(button_data->image))
	{
		button_data->image = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(button_data->img_res_id));
		button_data->mask = CreateBitmapMask(button_data->image, TRANSPARENT_COLOR);
	}

	PaintTransparentBitmap(dc, button_data->image, button_data->mask, color, -1, -1);
	EndPaint(hwnd, &ps);
}



static LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImageButtonState* button_data = {};
	if (msg == WM_NCCREATE) {
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		// Get CREATESTRUCT by casting lParam
		CREATESTRUCT* createstruct_ptr = reinterpret_cast<CREATESTRUCT*>(lParam);

		// CREATESTRUCT.lpCreateParams is the void pointer specified as CreateWindow() parameter
		// Get pointer to ImageButtonState by casting lpCreateParams.
		button_data = static_cast<ImageButtonState*>(createstruct_ptr->lpCreateParams);

		// Store ImageButtonState in instance data for this window handle.  This enables us to retrieve it later
		// using GetWindowLongPtr()
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)button_data);
	}
	else {
		button_data = reinterpret_cast<ImageButtonState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (msg) {

	case WM_DESTROY:
	{
		if (!button_data)
			break;

		if (button_data->image)
			DeleteObject(button_data->image);
		if (button_data->mask)
			DeleteObject(button_data->mask);
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

	case WP_BM_SETIMAGE:
	{
		// User passes resource ID for new HBITMAP as wParam
		if (!button_data)
			break;

		if (button_data->image)
			DeleteObject(button_data->image);
		if (button_data->mask)
			DeleteObject(button_data->mask);

		button_data->img_res_id = wParam;
		button_data->image = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(button_data->img_res_id));
		button_data->mask = CreateBitmapMask(button_data->image, TRANSPARENT_COLOR);
		PaintButton(hwnd, button_data);
		InvalidateWindow(hwnd);
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

	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ImageButtonRegisterClass(void)
{
	WNDCLASS wc = {};
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ButtonProc;
	wc.lpszClassName = WP_IMG_BTN_CLASS;
	RegisterClass(&wc);
}

void ImageButtonUnregisterClass(void)
{
	UnregisterClass(WP_IMG_BTN_CLASS, NULL);
}


HWND CreateImageButton(int img_res_id, COLORREF normal_color, COLORREF hover_color, COLORREF pressed_color,
	HWND parent_hwnd, HMENU btn_id, HINSTANCE parent_inst, int x, int y, int width, int height)
{
	HWND btn_hwnd = 0;
	ImageButtonState* btn_data = (ImageButtonState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ImageButtonState));
	if (btn_data)
	{
		btn_data->img_res_id = img_res_id;
		btn_data->normal_color = normal_color;
		btn_data->hover_color = hover_color;
		btn_data->pressed_color = pressed_color;
		btn_hwnd = CreateWindow(WP_IMG_BTN_CLASS, NULL, WS_VISIBLE | WS_CHILD, x, y, width, height,
			parent_hwnd, btn_id, parent_inst, btn_data);
	}
	return btn_hwnd;
}
