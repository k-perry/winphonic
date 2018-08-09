/******************************************************************************
trackbar.cpp - Trackbar GUI control
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


#include "trackbar.h"

// Is the specified position >= min and <= max?
static bool IsValidPos(TrackbarData* trackbar_data, int pos)
{
	if (pos < trackbar_data->min || pos > trackbar_data->max) {
		return false;
	}
	return true;
}

// Calculates the rectangle for the slider channel that the thumb button resides in
static RECT CalcChannelRect(HWND hwnd, TrackbarData* trackbar_data)
{
	RECT client_rect;
	GetClientRect(hwnd, &client_rect);
	int width = client_rect.right - client_rect.left;
	int height = client_rect.bottom - client_rect.top;
	RECT chan_rect;
	int channel_height, channel_width;
	int channel_left, channel_top, channel_right, channel_bottom;
	
	if (trackbar_data->type == TB_HORIZONTAL) {

		// Center the channel rectangle vertically and horizontally within the client area.
		// Leave extra room above, below, left, and right for the thumb button.
		channel_height = CHANNEL_SIZE;
		channel_width = width - THUMB_BTN_SIZE;
		channel_left = THUMB_BTN_SIZE / 2;
		channel_top = (int)round((height - channel_height) / 2.0);
		channel_right = channel_left + channel_width;
		channel_bottom = channel_top + channel_height;
	}
	else {	// trackbar_data->type == TB_VERTICAL
		channel_width = CHANNEL_SIZE;
		channel_height = height - THUMB_BTN_SIZE;
		channel_top = THUMB_BTN_SIZE / 2;
		channel_left = (int)round((width - channel_width) / 2.0);
		channel_right = channel_left + channel_width;
		channel_bottom = channel_top + channel_height;
	}

	chan_rect = { channel_left, channel_top, channel_right, channel_bottom };
	return chan_rect;
}

// Calculates a thumb slider position (>= min and <= max) based on the X or Y pixel coordinate
static int CalcPos(HWND hwnd, TrackbarData* trackbar_data, int coord)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	int range = trackbar_data->max - trackbar_data->min;
	double pos = 0.0;

	if (trackbar_data->type == TB_HORIZONTAL) {
		int channel_width = trackbar_data->channel_rect.right - trackbar_data->channel_rect.left;
		pos = round((((double)coord - trackbar_data->channel_rect.left) * range) / channel_width);
	}
	else {	// trackbar_data->type == TB_VERTICAL
		int channel_height = trackbar_data->channel_rect.bottom - trackbar_data->channel_rect.top;
		pos = round((((double)trackbar_data->channel_rect.bottom - coord) * range) / channel_height);
	}

	return (int)pos;
}


static void PaintTrackbar(HWND hwnd, TrackbarData* trackbar_data)
{
	if (!IsValidPos(trackbar_data, trackbar_data->pos)) {
		//OutputDebugStringA("Trackbar Error:  Invalid position specified.\n");
		return;
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	RECT rect;
	GetClientRect(hwnd, &rect);

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	int channel_width = trackbar_data->channel_rect.right - trackbar_data->channel_rect.left;
	int channel_height = trackbar_data->channel_rect.bottom - trackbar_data->channel_rect.top;
	int range = trackbar_data->max - trackbar_data->min;
	int slider_btn_left = 0;
	int slider_btn_top = 0;

	// Draw everything in this new device context and then blit it to screen
	HDC mem_dc = CreateCompatibleDC(hdc);
	HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(mem_dc, mem_bmp);
	HBRUSH bg_brush = CreateSolidBrush(trackbar_data->background_color);
	FillRect(mem_dc, &rect, bg_brush);
	DeleteObject(bg_brush);
	
	// Draw the channel rectangle
	HBRUSH slider_bar_brush = CreateSolidBrush(trackbar_data->channel_color);
	FillRect(mem_dc, &(trackbar_data->channel_rect), slider_bar_brush);
	DeleteObject(slider_bar_brush);

	if (range > 0) {

		int selected_left = 0, selected_top = 0, selected_right = 0, selected_bottom = 0;

		if (trackbar_data->type == TB_HORIZONTAL)
		{
			// The portion that is "selected" is based on trackbar_data->pos.
			// Ex:  If min = 0, max = 100, pos = 20, then 20% of the pixels will be selected.
			selected_left = trackbar_data->channel_rect.left;
			selected_top = trackbar_data->channel_rect.top;
			double sel_pct = ((double)trackbar_data->pos - trackbar_data->min) / range;
			selected_right = (int)(selected_left + (sel_pct * channel_width));
			selected_bottom = trackbar_data->channel_rect.bottom;

			// Draw the selected part of the slider bar rectangle
			RECT selected_rect = { selected_left, selected_top, selected_right, selected_bottom };
			HBRUSH selected_brush = CreateSolidBrush(trackbar_data->sel_channel_color);
			FillRect(mem_dc, &selected_rect, selected_brush);
			DeleteObject(selected_brush);

		}
		else if (trackbar_data->type == TB_VERTICAL)
		{

			selected_left = trackbar_data->channel_rect.left;
			selected_bottom = trackbar_data->channel_rect.bottom;
			selected_right = trackbar_data->channel_rect.right;
			double sel_pct = ((double)trackbar_data->pos - trackbar_data->min) / range;
			selected_top = (int)(selected_bottom - (sel_pct * channel_height));

			// Draw the selected part of the slider bar rectangle
			RECT selected_rect = { selected_left, selected_top, selected_right, selected_bottom };
			HBRUSH selected_brush = CreateSolidBrush(trackbar_data->sel_channel_color);
			FillRect(mem_dc, &selected_rect, selected_brush);
			DeleteObject(selected_brush);
		}

		if (trackbar_data->thumb_btn_img && trackbar_data->thumb_btn_mask)
		{
			// Draw the thumb button
			HDC bmp_dc = CreateCompatibleDC(mem_dc);
			BITMAP bmp_info;
			GetObject(trackbar_data->thumb_btn_img, sizeof(bmp_info), &bmp_info);
			
			if (trackbar_data->type == TB_HORIZONTAL)
			{
				slider_btn_left = (int)round(selected_right - (bmp_info.bmWidth / 2));
				slider_btn_top = (int)round((height - bmp_info.bmHeight) / 2);
			}
			else if (trackbar_data->type == TB_VERTICAL)
			{
				slider_btn_top = (int)round(selected_top - (bmp_info.bmWidth / 2));
				slider_btn_left = (int)round((width - bmp_info.bmHeight) / 2);
			}

			// Draw mask with SRCAND
			HBITMAP old_bmp = (HBITMAP)SelectObject(bmp_dc, trackbar_data->thumb_btn_mask);
			BitBlt(mem_dc, slider_btn_left, slider_btn_top, bmp_info.bmWidth, bmp_info.bmHeight, 
				bmp_dc, 0, 0, SRCAND);

			// Draw image with SRCPAINT
			SelectObject(bmp_dc, trackbar_data->thumb_btn_img);
			BitBlt(mem_dc, slider_btn_left, slider_btn_top, bmp_info.bmWidth, bmp_info.bmHeight, 
				bmp_dc, 0, 0, SRCPAINT);

			SelectObject(bmp_dc, old_bmp);
			DeleteDC(bmp_dc);
		}

	}

	// Copy the memory DC to the screen DC
	BitBlt(hdc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);

	DeleteDC(mem_dc);
	DeleteObject(mem_bmp);
	EndPaint(hwnd, &ps);
}


static void SetPos(HWND hwnd, TrackbarData* trackbar_data, int pos)
{
	if (!IsValidPos(trackbar_data, pos)) {
		return;
	}
	trackbar_data->pos = pos;
	PaintTrackbar(hwnd, trackbar_data);
	InvalidateWindow(hwnd);
}


static LRESULT CALLBACK TrackbarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TrackbarData* trackbar_data = {};
	if (uMsg == WM_NCCREATE) {
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		// Get CREATESTRUCT by casting lParam
		CREATESTRUCT* createstruct = reinterpret_cast<CREATESTRUCT*>(lParam);

		// CREATESTRUCT.lpCreateParams is the void pointer specified as CreateWindow() parameter.
		// Get pointer to TrackbarData by casting lpCreateParams.
		trackbar_data = reinterpret_cast<TrackbarData*>(createstruct->lpCreateParams);

		// Store TrackbarData in instance data for this window handle.  This enables us to retrieve it later
		// using GetWindowLongPtr()
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)trackbar_data);
	}
	else {
		trackbar_data = reinterpret_cast<TrackbarData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (uMsg) {

		case WM_CREATE:
		{
			if (!trackbar_data)
				break;
			trackbar_data->channel_rect = CalcChannelRect(hwnd, trackbar_data);
			SetPos(hwnd, trackbar_data, trackbar_data->pos);
			trackbar_data->parent = GetParent(hwnd);
		} break;

		case WM_DESTROY:
		{
			if (!trackbar_data)
				break;
			if (trackbar_data->thumb_btn_img)
				DeleteObject(trackbar_data->thumb_btn_img);
			if (trackbar_data->thumb_btn_mask)
				DeleteObject(trackbar_data->thumb_btn_mask);
			FreeMemory(trackbar_data);
			return 0;
		} break;
	
		case WM_PAINT:
		{
			PaintTrackbar(hwnd, trackbar_data);
			return 0;
		} break;

		case WM_LBUTTONDOWN:
		{
			int coord = 0;		// For horizontal, use X coordinate.  For vertical, use Y coordinate.
			if (trackbar_data->type == TB_HORIZONTAL)
				coord = GET_X_LPARAM(lParam);
			else
				coord = GET_Y_LPARAM(lParam);

			trackbar_data->is_mouse_down = true;
			SetCapture(hwnd);
			int pos = CalcPos(hwnd, trackbar_data, coord);
			SetPos(hwnd, trackbar_data, pos);
			SendMessage(trackbar_data->parent, WP_TBM_DRAGGING, (WPARAM)hwnd, (LPARAM)trackbar_data->pos);
		} break;

		case WM_LBUTTONUP:
		{
			trackbar_data->is_mouse_down = false;
			ReleaseCapture();
			SendMessage(trackbar_data->parent, WP_TBM_POSCHANGED, (WPARAM)hwnd, (LPARAM)trackbar_data->pos);
		} break;

		case WM_MOUSEMOVE:
		{
			if (!trackbar_data->is_mouse_down)
				break;
			int coord = 0;		// For horizontal, use X coordinate.  For vertical, use Y coordinate.
			if (trackbar_data->type == TB_HORIZONTAL)
				coord = GET_X_LPARAM(lParam);
			else
				coord = GET_Y_LPARAM(lParam);

			int pos = CalcPos(hwnd, trackbar_data, coord);
			SetPos(hwnd, trackbar_data, pos);

			SendMessage(trackbar_data->parent, WP_TBM_DRAGGING, (WPARAM)hwnd, (LPARAM)trackbar_data->pos);

		} break;

		case WP_TBM_SETPOS:
		{
			// Only process these messages if user is NOT currently dragging the thumb
			if (!trackbar_data->is_mouse_down)
				SetPos(hwnd, trackbar_data, (int)lParam);
		} break;

		case WP_TBM_GETPOS:
		{
			return trackbar_data->pos;
		} break;
		
		case WP_TBM_SETMAX:
		{
			trackbar_data->max = (int)lParam;
		} break;

		case WP_TBM_SETMIN:
		{
			trackbar_data->min = (int)lParam;
		}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void TrackbarRegisterClass(void)
{
	WNDCLASS wc = {};
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = TrackbarProc;
	wc.lpszClassName = WP_TRACKBAR_CLASS;
	RegisterClass(&wc);
}

void TrackbarUnregisterClass(void)
{
	UnregisterClass(WP_TRACKBAR_CLASS, NULL);
}


HWND CreateTrackbar(TrackbarType type, COLORREF background_color, COLORREF channel_color,
	COLORREF sel_channel_color, int thumb_btn_img_res_id, int min, int max, int pos, 
	HWND parent_hwnd, HINSTANCE parent_inst, int x, int y, int width, int height)
{
	HWND tb_hwnd = 0;
	TrackbarData* tb = (TrackbarData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TrackbarData));
	if (tb)
	{
		tb->type = type;
		tb->background_color = background_color;
		tb->channel_color = channel_color;
		tb->sel_channel_color = sel_channel_color;
		tb->min = min;
		tb->max = max;
		tb->pos = pos;
		tb->thumb_btn_img = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(thumb_btn_img_res_id));
		tb->thumb_btn_mask = CreateBitmapMask(tb->thumb_btn_img, TRANSPARENT_COLOR);
		tb_hwnd = CreateWindow(WP_TRACKBAR_CLASS, NULL, WS_VISIBLE | WS_CHILD, x, y, width, height,
			parent_hwnd, NULL, parent_inst, tb);
	}
	return tb_hwnd;
}