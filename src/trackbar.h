/******************************************************************************
trackbar.h - Header file for trackbar.cpp
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

#pragma once
#include <windows.h>
#include <math.h>
#include "resource.h"
#include "image.h"
#include "util.h"

#define BTN_RES_NAME IDB_CIRCLE
#define THUMB_BTN_SIZE 12
#define CHANNEL_SIZE 3

enum TrackbarType { TB_HORIZONTAL, TB_VERTICAL };

struct TrackbarData {
	COLORREF background_color;
	COLORREF channel_color;
	COLORREF sel_channel_color;
	TrackbarType type;
	int min;
	int max;
	int pos;
	HBITMAP thumb_btn_img;
	HBITMAP thumb_btn_mask;
	RECT channel_rect;
	HWND parent;
	bool is_mouse_down;
};

#define WP_TRACKBAR_CLASS "WPTrackbar"
#define TRANSPARENT_COLOR	RGB(40, 40, 40)

// Macros for extracting X and Y coordinates from mouse position in WM_XBUTTONXXX.
// These are taken from windowsx.h so that I didn't have to include entire header file.
#define GET_X_LPARAM(lp)	((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))


// Messages
// User-defined messages must being at 0x0400 (WM_USER)
#define WP_TBM_SETPOS		0x0401
#define WP_TBM_GETPOS		0x0402
#define WP_TBM_SETMAX		0x0403
#define WP_TBM_GETMAX		0x0404
#define WP_TBM_SETMIN		0x0405
#define WP_TBM_GETMIN		0x0406
#define WP_TBM_POSCHANGED	0x0407		// Notify parent that position has changed (user is done dragging)
#define WP_TBM_DRAGGING		0x0408		// Notify parent of pending change to the position



// Function declarations
static bool IsValidPos(TrackbarData* trackbar_data, int pos);
static RECT CalcChannelRect(HWND hwnd, TrackbarData* trackbar_data);
static int CalcPos(HWND hwnd, TrackbarData* trackbar_data, int x);
static void PaintTrackbar(HWND hwnd, TrackbarData* trackbar_data);
static void SetPos(HWND hwnd, TrackbarData* trackbar_data, int pos);

void TrackbarRegisterClass(void);
void TrackbarUnregisterClass(void);
HWND CreateTrackbar(TrackbarType type, COLORREF background_color, COLORREF channel_color,
	COLORREF sel_channel_color, int thumb_btn_img_res_id, int min, int max, int pos,
	HWND parent_hwnd, HINSTANCE parent_inst, int x, int y, int width, int height);