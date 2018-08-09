/******************************************************************************
util.cpp - Utility functions
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

#include "util.h"

HFONT GetFont(const char* font_name, int font_size, bool is_bold, bool is_italic, bool is_underline)
{
	HDC font_dc = CreateCompatibleDC(0);
	int weight = is_bold ? FW_BOLD : 0;
	DWORD italic = is_italic ? 1 : 0;
	DWORD underline = is_underline ? 1 : 0;
	long font_height = -MulDiv(font_size, GetDeviceCaps(font_dc, LOGPIXELSY), 72);
	HFONT font = CreateFont(font_height, 0, 0, 0, weight, italic, underline, 0, 0, 0, 0, 0, 0, font_name);
	DeleteDC(font_dc);
	return font;
}

void WINAPIV DebugOut(const TCHAR *fmt, ...) {
	TCHAR s[1025];
	va_list args;
	ZeroMemory(s, 1025 * sizeof(s[0]));
	va_start(args, fmt);
	wvsprintf(s, fmt, args);
	va_end(args);
	s[1024] = 0;
	OutputDebugString(s);
}

const char* GetFilenameExt(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return "";
	else
		return dot + 1;
}

void InvalidateWindow(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	
	// Last parameter specifies whether the background should be erased
	InvalidateRect(hwnd, &rect, false);
}

// Allocates memory heap and returns a copy of the specified string.
// Caller must free the heap memory when necessary.
// (Similar to strdup() on Unix)
char* DuplicateString(const char* str)
{
	if (str == NULL)
		return NULL;

	size_t len = lstrlen(str) + 1;
	char* copy = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
	if (copy)
		memcpy(copy, str, len);
	return copy;
}

void FreeMemory(void* ptr)
{
	if (ptr != NULL)
	{
		HeapFree(GetProcessHeap(), 0, ptr);
	}
}


// Creates a black-and-white bitmap mask for creating transparency.
// Uses the specified color as the transparent color.
// Reference:  http://www.winprog.org/tutorial/transparency.html
HBITMAP CreateBitmapMask(HBITMAP bitmap, COLORREF transparent_color)
{
	HDC mem_dc_1, mem_dc_2;
	HBITMAP mask;
	BITMAP bmp_info;

	// Create monochrome (1 bit) mask bitmap.  

	GetObject(bitmap, sizeof(BITMAP), &bmp_info);
	mask = CreateBitmap(bmp_info.bmWidth, bmp_info.bmHeight, 1, 1, NULL);

	mem_dc_1 = CreateCompatibleDC(0);
	mem_dc_2 = CreateCompatibleDC(0);

	SelectObject(mem_dc_1, bitmap);
	SelectObject(mem_dc_2, mask);

	// Set the background colour of the colour image to the colour
	// you want to be transparent.
	SetBkColor(mem_dc_1, transparent_color);

	// Copy the bits from the colour image to the B+W mask... everything
	// with the background colour ends up white while everythig else ends up
	// black...Just what we wanted.

	BitBlt(mem_dc_2, 0, 0, bmp_info.bmWidth, bmp_info.bmHeight, mem_dc_1, 0, 0, SRCCOPY);

	// Take our new mask and use it to turn the transparent colour in our
	// original colour image to black so the transparency effect will
	// work right.
	BitBlt(mem_dc_1, 0, 0, bmp_info.bmWidth, bmp_info.bmHeight, mem_dc_2, 0, 0, SRCINVERT);

	// Clean up
	DeleteDC(mem_dc_1);
	DeleteDC(mem_dc_2);

	return mask;
}


// If x and y are both -1, then the function centers the image in the device context
void PaintTransparentBitmap(HDC dc, HBITMAP bitmap, HBITMAP mask, COLORREF bg_color, int x, int y)
{
	BITMAP bmp_info;
	RECT client_rect;
	HWND hwnd = WindowFromDC(dc);
	HDC mem_dc = CreateCompatibleDC(dc);

	GetObject(bitmap, sizeof(bmp_info), &bmp_info);
	GetClientRect(hwnd, &client_rect);
	HBRUSH bg_brush = CreateSolidBrush(bg_color);
	FillRect(dc, &client_rect, bg_brush);
	DeleteObject(bg_brush);

	if (x == -1 && y == -1)
	{
		// Center the image in the DC
		x = (client_rect.right - client_rect.left - bmp_info.bmWidth) / 2;
		y = (client_rect.bottom - client_rect.top - bmp_info.bmHeight) / 2;
	}

	HBITMAP old_bmp = (HBITMAP)SelectObject(mem_dc, mask);
	BitBlt(dc, x, y, bmp_info.bmWidth, bmp_info.bmHeight, mem_dc, 0, 0, SRCAND);
	SelectObject(mem_dc, bitmap);
	BitBlt(dc, x, y, bmp_info.bmWidth, bmp_info.bmHeight, mem_dc, 0, 0, SRCPAINT);

	SelectObject(mem_dc, old_bmp);
	DeleteDC(mem_dc);
}


// Given a full path with file name, it modifies the string in-place to contain only the path.
// Ex:  C:\Dir1\Dir2\file.txt -> C:\Dir1\Dir2\ 
void RemoveFilenameFromPath(char* file_name, size_t file_name_buffer_len)
{
	if (!file_name || !file_name_buffer_len)
		return;

	for (size_t pos = file_name_buffer_len - 1; pos >= 0; pos--)
	{
		// Iterate through string backwards until we find a backslash (\)
		if (file_name[pos] != '\\')
			file_name[pos] = '\0';
		else
			break;
	}
}

// Given a full path with file name, it extracts the file name portion and puts t
// in the specified file_name buffer.  Ex:  C:\Dir1\Dir2\file.txt -> file.txt
void GetFilenameFromPath(char* path, size_t path_buffer_len, char* file_name, 
	size_t file_name_buffer_len)
{
	if (!path || !path_buffer_len || !file_name || !file_name_buffer_len)
		return;
	ZeroMemory(file_name, file_name_buffer_len);
	for (size_t pos = path_buffer_len - 1; pos >= 0; pos--)
	{
		if (path[pos] == '\\')
		{
			// Everything to right of this position is the file name
			size_t bytes_to_copy = lstrlen(path + pos + 1);
			if (bytes_to_copy > file_name_buffer_len - 1)
				bytes_to_copy = file_name_buffer_len - 1;
			memcpy(file_name, path + pos + 1, bytes_to_copy);
			break;
		}
	}
}