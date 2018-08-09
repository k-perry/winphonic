/******************************************************************************
img_label.h - Header file for img_label.cpp
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
#include "image.h"
#include "util.h"

#define WP_IMG_LABEL_CLASS "WPImageLabel"

// User defined messages
#define WP_LM_SETIMAGE_FROM_RES		0x0501		// Load image from a resource ID
#define WP_LM_SETIMAGE_FROM_BUFFER	0x0502		// Load image from a buffer containing file data
#define WP_LM_CLEARIMAGE			0x0503		// Clear current image
#define WP_LM_SETIMAGEALPHA			0x0504		// Set the alpha blend level for the image.  255 = opaque.  0 = transparent.

struct ImageLabelState {
	COLORREF background_color;
	COLORREF border_color;
	HBITMAP image;
	bool stretch;
	BYTE alpha;
};

void ImageLabelRegisterClass(void);
void ImageLabelUnregisterClass(void);

HWND CreateImageLabel(int img_res_id, bool stretch, BYTE alpha, COLORREF background_color, HWND parent_hwnd,
	HMENU lbl_id, HINSTANCE parent_inst, int x, int y, int width, int height);
