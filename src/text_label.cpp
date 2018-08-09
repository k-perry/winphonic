/******************************************************************************
text_label.cpp - Label GUI control for displaying text
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

#include "text_label.h"

static void PaintTextLabel(HWND hwnd, TextLabelData* label_data)
{
	if (!label_data)
		return;

	PAINTSTRUCT ps;
	HDC hdc;
	RECT client_rect;
	GetClientRect(hwnd, &client_rect);
	
	int width = client_rect.right - client_rect.left;
	int height = client_rect.bottom - client_rect.top;

	hdc = BeginPaint(hwnd, &ps);

	// Draw everything in this new device context and then blit it to screen
	HDC mem_dc = CreateCompatibleDC(hdc);
	HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(mem_dc, mem_bmp);

	if (!label_data->background_color)
	{
		// If no background color specified, set it to black
		label_data->background_color = RGB(0, 0, 0);
	}

	HBRUSH bg_brush = CreateSolidBrush(label_data->background_color);
	FillRect(mem_dc, &client_rect, bg_brush);
	DeleteObject(bg_brush);
		
	// Create the font, if it already hasn't been done
	if (!(label_data->font))
	{
		label_data->font = GetFont(label_data->font_name, label_data->font_size,
			label_data->font_bold, false, false);
	}

	SetBkMode(mem_dc, TRANSPARENT);
	SetTextColor(mem_dc, label_data->text_color);
	SelectObject(mem_dc, label_data->font);
	
	// Set up DrawText() formatting flags based on user options.
	// Must use DT_SINGLELINE, or else vertical alignment is ignored.
	UINT drawtext_fmt = DT_SINGLELINE | DT_END_ELLIPSIS;
	if (label_data->horiz_align == LBL_H_LEFT)
		drawtext_fmt |= DT_LEFT;
	else if (label_data->horiz_align == LBL_H_CENTER)
		drawtext_fmt |= DT_CENTER;
	else if (label_data->horiz_align == LBL_H_RIGHT)
		drawtext_fmt |= DT_RIGHT;

	if (label_data->vert_align == LBL_V_TOP)
		drawtext_fmt |= DT_TOP;
	else if (label_data->vert_align == LBL_V_CENTER)
		drawtext_fmt |= DT_VCENTER;
	else if (label_data->vert_align == LBL_V_BOTTOM)
		drawtext_fmt |= DT_BOTTOM;

	if (label_data->text)
	{
		if (!DrawTextA(mem_dc, label_data->text, -1, &client_rect, drawtext_fmt))
		{
			DebugOut("Text Label Error:  DrawText() failed.\n");
		}
	}
	
	// Copy the memory DC to the screen DC
	if (!BitBlt(hdc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY))
	{
		DebugOut("Text Label Error:  BitBlt() failed.\n");
	}

	DeleteDC(mem_dc);
	DeleteObject(mem_bmp);
	EndPaint(hwnd, &ps);
}


static LRESULT CALLBACK TextLabelProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TextLabelData* label_data = {};
	if (uMsg == WM_NCCREATE)
	{
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		// Get CREATESTRUCT by casting lParam
		CREATESTRUCT* createstruct_ptr = reinterpret_cast<CREATESTRUCT*>(lParam);

		// CREATESTRUCT.lpCreateParams is the void pointer specified as CreateWindow() parameter
		// Get pointer to LabelData by casting lpCreateParams.
		label_data = reinterpret_cast<TextLabelData*>(createstruct_ptr->lpCreateParams);

		// Store LabelData in instance data for this window handle.  This enables us to retrieve it later
		// using GetWindowLongPtr()
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)label_data);
	}
	else
	{
		label_data = reinterpret_cast<TextLabelData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (uMsg)
	{
		case WM_DESTROY:
		{
			if (!label_data)
				break;

			if (label_data->font)
				DeleteObject(label_data->font);
			FreeMemory(label_data->font_name);
			FreeMemory(label_data->text);
			FreeMemory(label_data);
			return 0;
		}
		case WM_PAINT:
		{
			PaintTextLabel(hwnd, label_data);
			return 0;
		} break;

		case WM_LBUTTONUP:
		{
			// Send message to parent to notify of click.  Regular STATIC controls send WM_COMMAND + STN_CLICKED.
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), STN_CLICKED), LPARAM(hwnd));
		} break;

		case WM_SETTEXT:
		{
			if (!label_data)
				break;

			FreeMemory(label_data->text);
			
			char* new_text = (char*)lParam;
			if (new_text)
				label_data->text = DuplicateString(new_text);
			else
				label_data->text = NULL;

			PaintTextLabel(hwnd, label_data);
			InvalidateWindow(hwnd);

		} break;

	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void TextLabelRegisterClass(void)
{
	WNDCLASS wc = { 0 };
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = TextLabelProc;
	wc.lpszClassName = WP_TEXT_LABEL_CLASS;
	RegisterClass(&wc);
}

void TextLabelUnregisterClass(void)
{
	UnregisterClass(WP_TEXT_LABEL_CLASS, NULL);
}


HWND CreateTextLabel(const char* text, const char* font_name, int font_size, bool font_bold, 
	COLORREF background_color, COLORREF text_color, HorizontalAlign horiz_align, VerticalAlign vert_align, 
	HWND parent_hwnd, HMENU lbl_id,	HINSTANCE parent_inst, int x, int y, int width, int height)
{
	HWND lbl_hwnd = 0;
	TextLabelData* lbl_data = (TextLabelData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TextLabelData));
	if (lbl_data)
	{
		if (text)
			lbl_data->text = DuplicateString(text);
		
		lbl_data->font_name = DuplicateString(font_name);
		lbl_data->font_size = font_size;
		lbl_data->font_bold = font_bold;
		lbl_data->background_color = background_color;
		lbl_data->text_color = text_color;
		lbl_data->horiz_align = horiz_align;
		lbl_data->vert_align = vert_align;
		lbl_hwnd = CreateWindow(WP_TEXT_LABEL_CLASS, NULL, WS_VISIBLE | WS_CHILD, x, y, width, height,
			parent_hwnd, lbl_id, parent_inst, lbl_data);
	}
	return lbl_hwnd;
}