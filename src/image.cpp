/******************************************************************************
image.cpp - Load PNG and JPEG images (including transparency) into an HBITMAP
Based on:  http://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
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

#include "image.h"

// Loads a PNG/JPEG image from the specified stream using Windows Imaging Component
IWICBitmapSource* LoadBitmapFromStream(IStream* image_stream, int image_format)
{
	IWICBitmapSource* bmp_source = NULL;
	CoInitialize(NULL);
	IWICBitmapDecoder* bmp_decoder = NULL;
	GUID decoder;
	if (image_format == IMG_FORMAT_JPG)
	{
		decoder = CLSID_WICJpegDecoder;
	}
	else if (image_format == IMG_FORMAT_PNG)
	{
		// NOTE:  CLSID_WICPngDecoder does not appear to work on Windows 7
		decoder = CLSID_WICPngDecoder;
	}
	
	HRESULT cci_result = CoCreateInstance(decoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(bmp_decoder), 
		reinterpret_cast<void**>(&bmp_decoder));
	if (FAILED(cci_result))
	{
		switch (cci_result)
		{
			case REGDB_E_CLASSNOTREG:
				OutputDebugStringA("LoadBitmapFromStream:  CoCreateInstance failed with error code REGDB_E_CLASSNOTREG\n");
				break;
			case E_NOINTERFACE:
				OutputDebugStringA("LoadBitmapFromStream:  CoCreateInstance failed with error code E_NOINTERFACE\n");
				break;
			default:
				OutputDebugStringA("LoadBitmapFromStream:  CoCreateInstance failed with a different error code\n");
		}
		return NULL;
	}

	if (FAILED(bmp_decoder->Initialize(image_stream, WICDecodeMetadataCacheOnLoad)))
	{
		OutputDebugStringA("LoadBitmapFromStream:  bmp_decoder->Initialize failed.\n");
		bmp_decoder->Release();
		return NULL;
	}

	// Check for the presence of the first frame in the bitmap
	UINT nFrameCount = 0;
	if (FAILED(bmp_decoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
	{
		OutputDebugStringA("LoadBitmapFromStream:  bmp_decoder->GetFrameCount failed.\n");
		bmp_decoder->Release();
		return NULL;
	}

	// Load the first frame (i.e., the image)
	IWICBitmapFrameDecode* bmp_frame_decode = 0;
	if (FAILED(bmp_decoder->GetFrame(0, &bmp_frame_decode)))
	{
		OutputDebugStringA("LoadBitmapFromStream:  bmp_decoder->GetFrame failed.\n");
		bmp_decoder->Release();
		return NULL;
	}

	// Convert the image to 32bpp BGRA format with pre-multiplied alpha.
	// It may not be stored in that format natively in the image,
	// but we need this format to create the DIB to use on-screen.
	WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, bmp_frame_decode, &bmp_source);
	
	bmp_frame_decode->Release();
	bmp_decoder->Release();
	return bmp_source;
}

// Creates a 32-bit DIB from the specified WIC bitmap.
HBITMAP CreateHBITMAP(IWICBitmapSource * bmp_source)
{
	HBITMAP bmp = NULL;
	UINT width = 0;
	UINT height = 0;
	if (FAILED(bmp_source->GetSize(&width, &height)) || width == 0 || height == 0)
		return NULL;

	// Prepare structure giving bitmap information.  Negative height means top-down DIB.
	BITMAPINFO bmp_info = {};
	bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmp_info.bmiHeader.biWidth = width;
	bmp_info.bmiHeader.biHeight = -((LONG)height);
	bmp_info.bmiHeader.biPlanes = 1;
	bmp_info.bmiHeader.biBitCount = 32;
	bmp_info.bmiHeader.biCompression = BI_RGB;

	// Create a DIB section that can hold the image
	void* image_bits = NULL;
	HDC screen_dc = GetDC(NULL);
	bmp = CreateDIBSection(screen_dc, &bmp_info, DIB_RGB_COLORS, &image_bits, NULL, 0);
	ReleaseDC(NULL, screen_dc);
	if (bmp == NULL)
	{
		OutputDebugStringA("CreateHBITMAP:  CreateDIBSection failed.\n");
		return NULL;
	}

	// Extract the image into the HBITMAP
	const UINT stride = width * 4;
	const UINT num_img_pixels = stride * height;

	if (FAILED(bmp_source->CopyPixels(NULL, stride, num_img_pixels, static_cast<BYTE *>(image_bits))))
	{
		// couldn't extract image; delete HBITMAP
		DeleteObject(bmp);
		OutputDebugStringA("CreateHBITMAP:  CopyPixels failed.\n");
		return NULL;
	}

	return bmp;
}

// Creates a stream object initialized with the data from an executable resource.
IStream * CreateStreamFromResource(LPCTSTR res_name, LPCTSTR res_type)
{
	IStream * stream = NULL;

	// Find the resource's info block
	HRSRC res_info_block = FindResource(NULL, res_name, res_type);
	if (res_info_block == NULL) {
		OutputDebugStringA("CreateStreamOnResource:  FindResource failed.\n");
		return NULL;
	}

	// Load the resource and get handle of the resource data
	const DWORD res_size = SizeofResource(NULL, res_info_block);
	HGLOBAL res_data_handle = LoadResource(NULL, res_info_block);
	if (res_data_handle == NULL)
	{
		OutputDebugStringA("CreateStreamOnResource:  LoadResource failed.\n");
		return NULL;
	}

	// Lock the resource and get a pointer to first byte of resource data
	LPVOID res_data_ptr = LockResource(res_data_handle);
	if (res_data_ptr == NULL) {
		OutputDebugStringA("CreateStreamOnResource:  LockResource failed.\n");
		return NULL;
	}

	// allocate memory to hold the resource data
	HGLOBAL res_mem_handle = GlobalAlloc(GMEM_MOVEABLE, res_size);
	if (res_mem_handle == NULL)
	{
		OutputDebugStringA("CreateStreamOnResource:  GlobalAlloc failed.\n");
		return NULL;
	}

	// get a pointer to the allocated memory
	LPVOID res_mem_ptr = GlobalLock(res_mem_handle);
	if (res_mem_ptr == NULL)
	{
		OutputDebugStringA("CreateStreamOnResource:  GlobalLock failed.\n");
		GlobalFree(res_mem_handle);
		return NULL;
	}

	// copy the data from the resource to the new memory block
	CopyMemory(res_mem_ptr, res_data_ptr, res_size);
	GlobalUnlock(res_mem_handle);

	// create a stream on the HGLOBAL containing the data
	if (!SUCCEEDED(CreateStreamOnHGlobal(res_mem_handle, TRUE, &stream)))
	{
		OutputDebugStringA("CreateStreamOnResource:  CreateStreamOnHGlobal failed.\n");
		GlobalFree(res_mem_handle);
		return NULL;
	}
	else
	{
		// no need to unlock or free the resource
		return stream;
	}
}


// Loads the PNG from the specified resource into a HBITMAP.
HBITMAP LoadPNGFromResource(int res_id)
{
	HBITMAP bmp_png = NULL;

	// Load the PNG image data into a stream
	IStream* stream = CreateStreamFromResource(MAKEINTRESOURCE(res_id), "PNG");
	if (stream == NULL) {
		OutputDebugStringA("LoadPNG:  CreateStreamOnResource failed.\n");
		return NULL;
	}

	// Load the bitmap with WIC
	IWICBitmapSource* bmp_source = LoadBitmapFromStream(stream, IMG_FORMAT_PNG);
	if (bmp_source == NULL) {
		OutputDebugStringA("LoadPNG:  LoadBitmapFromStream failed.\n");
		stream->Release();
		return NULL;
	}

	// create a HBITMAP containing the image
	bmp_png = CreateHBITMAP(bmp_source);
	bmp_source->Release();
	stream->Release();
	return bmp_png;
}

// Looks for magic number to indicate which image format.
// Returns an enum indicating whether image is a JPG or PNG.
// Returns 0 if failed.
int GetImageFormatFromBuffer(unsigned char* buffer)
{
	if (buffer == NULL)
		return 0;

	/*
	JPEG images start with:		FF D8 FF
	PNG images start with:		89 50 4E 47 0D 0A 1A 0A
	*/
	if ((buffer[0] == 0xFF) && (buffer[1] == 0xD8) && (buffer[2] == 0xFF))
	{	// JPEG
		return IMG_FORMAT_JPG;
	}
	else if ((buffer[0] == 0x89) && (buffer[1] == 0x50) && (buffer[2] == 0x4E) && (buffer[3] == 0x47))
	{	// PNG
		return IMG_FORMAT_PNG;
	}
	return 0;
}

// Takes a buffer containing file (PNG or JPG) data and returns an HBITMAP
HBITMAP LoadImageFromBuffer(unsigned char* buffer, size_t buf_size)
{
	HBITMAP bitmap = NULL;

	// BYTE = unsigned char on Windows 10 x64
	IStream* stream = SHCreateMemStream((BYTE*)buffer, buf_size);
	if (stream)
	{
		const int img_format = GetImageFormatFromBuffer(buffer);
		if (img_format)
		{
			IWICBitmapSource* bitmap_source = LoadBitmapFromStream(stream, img_format);
			if (bitmap_source)
			{
				bitmap = CreateHBITMAP(bitmap_source);
				bitmap_source->Release();
			}
		}
		stream->Release();
	}
	return bitmap;
}