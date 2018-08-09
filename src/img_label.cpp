/******************************************************************************
img_label.cpp - Label GUI control that displays JPEG/PNG images with transparency
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


#include "img_label.h"

// If stretch = 1, the image will be stretched to fit the area.
// If stretch = 0, the image will NOT be stretched.  It will be alpha blended.
static void PaintImgLabel(HWND hwnd, ImageLabelState* label_data, int stretch)
{
	PAINTSTRUCT ps = {};
	HDC dc = 0;
	BITMAP bmp_info = {};
	RECT client_rect;
	GetClientRect(hwnd, &client_rect);
	const int width = client_rect.right - client_rect.left;
	const int height = client_rect.bottom - client_rect.top;

	dc = BeginPaint(hwnd, &ps);
	
	HDC mem_dc = CreateCompatibleDC(dc);

	if (label_data->image)
	{
		if (stretch == 1)
		{
			// Select the image for the memory DC
			HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, label_data->image);

			// Get info about the image (e.g. width, height) and save it in bmp_info struct
			GetObject(label_data->image, sizeof(bmp_info), &bmp_info);

			// HALFTONE mode results in much smoother resizing
			SetStretchBltMode(dc, HALFTONE);
			SetBrushOrgEx(dc, 0, 0, NULL);
			StretchBlt(dc, 0, 0, width, height, mem_dc, 0, 0, bmp_info.bmWidth, bmp_info.bmHeight, SRCCOPY);
			SelectObject(mem_dc, old_bitmap);
		}
		else
		{
			// Draw background to overwrite whatever the previous image was
			HBITMAP mem_bmp = CreateCompatibleBitmap(dc, width, height);
			HBITMAP old_bmp = (HBITMAP)SelectObject(mem_dc, mem_bmp);
			if (label_data->border_color)
				SetDCPenColor(mem_dc, label_data->border_color);
			else
				SetDCPenColor(mem_dc, label_data->background_color);
			SetDCBrushColor(mem_dc, label_data->background_color);
			SelectObject(mem_dc, GetStockObject(DC_PEN));
			SelectObject(mem_dc, GetStockObject(DC_BRUSH));
			Rectangle(mem_dc, client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);

			// Copy the memory DC to the screen DC
			BitBlt(dc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);
			SelectObject(mem_dc, old_bmp);
			DeleteObject(mem_bmp);

			// Alpha blend the image
			GetObject(label_data->image, sizeof(bmp_info), &bmp_info);		// Gets image info and puts it in bmp_info
			SIZE img_size = { bmp_info.bmWidth, bmp_info.bmHeight };
			SelectObject(mem_dc, label_data->image);
			
			// Use the source image's alpha channel for blending
			BLENDFUNCTION blend = {};
			blend.BlendOp = AC_SRC_OVER;
			blend.SourceConstantAlpha = label_data->alpha;
			blend.AlphaFormat = AC_SRC_ALPHA;

			RECT rect;
			GetClientRect(hwnd, &rect);
			const int img_left = (rect.right - rect.left - img_size.cx) / 2;
			const int img_top = (rect.bottom - rect.top - img_size.cy) / 2;

			// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/dd183351(v=vs.85).aspx
			AlphaBlend(dc, img_left, img_top, img_size.cx, img_size.cy, mem_dc, 0, 0, img_size.cx,
				img_size.cy, blend);

		}

	}
	else
	{
		HBITMAP mem_bmp = CreateCompatibleBitmap(dc, width, height);
		HBITMAP old_bmp = (HBITMAP) SelectObject(mem_dc, mem_bmp);

		// No image, so just draw blank background
		if (label_data->border_color)
			SetDCPenColor(mem_dc, label_data->border_color);
		else
			SetDCPenColor(mem_dc, label_data->background_color);
		SetDCBrushColor(mem_dc, label_data->background_color);
		SelectObject(mem_dc, GetStockObject(DC_PEN));
		SelectObject(mem_dc, GetStockObject(DC_BRUSH));
		Rectangle(mem_dc, client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);
		
		// Copy the memory DC to the screen DC
		BitBlt(dc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);
		SelectObject(mem_dc, old_bmp);
		DeleteObject(mem_bmp);
	}

	DeleteDC(mem_dc);
	EndPaint(hwnd, &ps);
}


static LRESULT CALLBACK ImgLabelProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImageLabelState* label_data = {};
	if (uMsg == WM_NCCREATE) {
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		// Get CREATESTRUCT by casting lParam
		CREATESTRUCT* createstruct_ptr = reinterpret_cast<CREATESTRUCT*>(lParam);

		// CREATESTRUCT.lpCreateParams is the void pointer specified as CreateWindow() parameter
		// Get pointer to LabelData by casting lpCreateParams.
		label_data = reinterpret_cast<ImageLabelState*>(createstruct_ptr->lpCreateParams);

		// Store LabelData in instance data for this window handle.  This enables us to retrieve it later
		// using GetWindowLongPtr()
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)label_data);
	}
	else {
		label_data = reinterpret_cast<ImageLabelState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (uMsg) {

		case WM_DESTROY:
		{
			if (!label_data)
				break;
			
			if (label_data->image)
				DeleteObject(label_data->image);
			FreeMemory(label_data);
			return 0;
		}
		case WM_PAINT:
		{
			if (!label_data)
				break;

			PaintImgLabel(hwnd, label_data, label_data->stretch);
			return 0;
		} break;

		case WM_LBUTTONUP:
		{
			// Send message to parent to notify of click.  Regular STATIC controls send WM_COMMAND + STN_CLICKED.
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), STN_CLICKED), LPARAM(hwnd));
		} break;

		case WP_LM_CLEARIMAGE:
		{
			if (!label_data)
				break;

			if (label_data->image)
				DeleteObject(label_data->image);	// Delete old HBITMAP
			label_data->image = NULL;
			PaintImgLabel(hwnd, label_data, label_data->stretch);
			InvalidateWindow(hwnd);
		} break;

		case WP_LM_SETIMAGE_FROM_RES:
		{
			// Load image from the specified resource ID
			// wParam contains the resource ID
			// lParam specifies whether the image should be stretched to fill the label
			//		lParam = 1 -> image will be stretched
			//		lParam = 0 -> image will NOT be stretched and will be alpha blended

			if (!label_data)
				break;			
			
			if (label_data->image)
				DeleteObject(label_data->image);

			label_data->image = LoadPNGFromResource(wParam);
			label_data->stretch = lParam;

			PaintImgLabel(hwnd, label_data, label_data->stretch);
			InvalidateWindow(hwnd);
		} break;

		case WP_LM_SETIMAGE_FROM_BUFFER:
		{
			// Load image from the provided file buffer
			// wParam contains the file buffer
			// lParam contains the file buffer size

			if (!label_data)
				break;

			unsigned char* img_buf = (unsigned char*)wParam;
			const size_t buf_size = (size_t)lParam;

			if (label_data->image)
			{
				DeleteObject(label_data->image);	// Delete old HBITMAP
			}
			
			if (!img_buf || !buf_size)
			{
				// No valid image specified.  Paint a blank background.
				label_data->image = NULL;
			}
			else
			{				
				label_data->image = LoadImageFromBuffer(img_buf, buf_size);
			}

			label_data->stretch = 1;

			PaintImgLabel(hwnd, label_data, label_data->stretch);
			InvalidateWindow(hwnd);
		} break;

		case WP_LM_SETIMAGEALPHA:
		{
			if (!label_data)
				break;

			const BYTE alpha = (BYTE)wParam;
			if (alpha >= 0 && alpha <= 255)
			{
				label_data->alpha = alpha;
				PaintImgLabel(hwnd, label_data, label_data->stretch);
				RECT rect;
				GetClientRect(hwnd, &rect);
				InvalidateRect(hwnd, &rect, false);
			}

		} break;

	}

	//return CallWindowProc(g_orig_btn_proc, hwnd, uMsg, wParam, lParam);
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ImageLabelRegisterClass(void)
{
	WNDCLASS wc = { 0 };
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ImgLabelProc;
	wc.lpszClassName = WP_IMG_LABEL_CLASS;
	RegisterClass(&wc);
}

void ImageLabelUnregisterClass(void)
{
	UnregisterClass(WP_IMG_LABEL_CLASS, NULL);
}

HWND CreateImageLabel(int img_res_id, bool stretch, BYTE alpha, COLORREF background_color, HWND parent_hwnd,
	HMENU lbl_id, HINSTANCE parent_inst, int x, int y, int width, int height)
{
	HWND lbl_hwnd = 0;
	ImageLabelState* lbl_state = (ImageLabelState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ImageLabelState));
	if (lbl_state)
	{
		if (img_res_id)
			lbl_state->image = LoadPNGFromResource(img_res_id);
		lbl_state->stretch = stretch;
		lbl_state->alpha = alpha;
		lbl_state->background_color = background_color;
		lbl_hwnd = CreateWindow(WP_IMG_LABEL_CLASS, NULL, WS_VISIBLE | WS_CHILD, x, y, width, height,
			parent_hwnd, lbl_id, parent_inst, lbl_state);
	}
	return lbl_hwnd;
}