/******************************************************************************
util.h - Header file for util.cpp
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
#include <Windows.h>

HFONT GetFont(const char* font_name, int font_size, bool is_bold, bool is_italic, bool is_underline);
void WINAPIV DebugOut(const TCHAR *fmt, ...);
const char* GetFilenameExt(const char *filename);
void InvalidateWindow(HWND hwnd);
char* DuplicateString(const char* str);
void FreeMemory(void* ptr);
HBITMAP CreateBitmapMask(HBITMAP bitmap, COLORREF transparent_color);
void PaintTransparentBitmap(HDC dc, HBITMAP bitmap, HBITMAP mask, COLORREF bg_color, int x, int y);
void RemoveFilenameFromPath(char* file_name, size_t len);
void GetFilenameFromPath(char* path, size_t path_buffer_len, char* file_name,
	size_t file_name_buffer_len);