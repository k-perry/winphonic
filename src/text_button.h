/******************************************************************************
text_button.h - Header file for text_button.cpp
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

#define WP_TEXT_BTN_CLASS "WPTextButton"

#define WP_BM_SETCOLOR_NORMAL	0x0601
#define WP_BM_SETCOLOR_HOVER	0x0602
#define WP_BM_SETCOLOR_PRESSED	0x0603


struct TextButtonState {
	COLORREF normal_color;
	COLORREF hover_color;
	COLORREF pressed_color;
	char* caption;
	char* font_name;
	int font_size;
	COLORREF font_color;
	bool is_hovered;
	bool is_mouse_pressed;	// Is mouse button currently down?
	HFONT font;
};

void TextButtonRegisterClass(void);
void TextButtonUnregisterClass(void);

HWND CreateTextButton(const char* caption, const char* font_name, int font_size, COLORREF font_color,
	COLORREF btn_normal_color, COLORREF btn_hover_color, COLORREF btn_pressed_color, HWND parent_hwnd,
	HMENU btn_id, HINSTANCE parent_inst, int x, int y, int width, int height);