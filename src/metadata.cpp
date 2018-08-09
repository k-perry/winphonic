/******************************************************************************
metadata.cpp - Functions for parsing MP3 and OGG metadata (e.g. ID3 tags)
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


#include <Windows.h>
#include "metadata.h"


// Decodes the tag size from the tag header
// References:
//		https://en.wikipedia.org/wiki/Synchsafe
//		https://stackoverflow.com/a/5652842
unsigned int ID3v2_DecodeTagSize(unsigned char size_bytes[4])
{
	unsigned int size = (size_bytes[0] << 21) | (size_bytes[1] << 14) | 
		(size_bytes[2] << 7) | size_bytes[3];
	return size;
}


// Decodes the frame size from the frame header
// Reference:  https://hydrogenaud.io/index.php/topic,67145.0.html
unsigned int ID3v2_DecodeFrameSize(unsigned char size_bytes[4]) {
	unsigned int size = (size_bytes[0] << 24) | (size_bytes[1] << 16) | 
		(size_bytes[2] << 8) | size_bytes[3];
	return size;
}


// Parses ID3v2 header
ID3v2Header ID3v2_ParseHeader(unsigned char raw_header[10])
{
	// Header Format:  ID3VVFSSSS (10 bytes)
	//		ID3 = first 3 bytes are always "ID3"
	//		VV = version information (2 bytes)
	//		F = flags (1 byte); each of the 8 bits represents a flag
	//		SSSS = tag size, excluding header (4 bytes)
	// Source:  http://id3.org/metadata.3.0#ID3v2_header

	ID3v2Header header;
	header.major_version = raw_header[3];
	header.revision = raw_header[4];
	header.flags = raw_header[5];

	// Get the size of the entire tag, excluding the header itself
	unsigned char raw_tag_size[4];
	memcpy(raw_tag_size, &(raw_header[6]), 4);
	header.tag_size = ID3v2_DecodeTagSize(raw_tag_size);
	return header;
}


// Reads the first frame from the buffer and fills the specified ID3v2Frame struct.
// If there are subsequent frames in the specified buffer, they are ignored.
// To read all the frames, repeatedly call ID3v2_ParseFrame() and increment pointer position.
// Returns 0 if successful. Returns -1 if the frame was invalid.
int ID3v2_ParseFrame(const char* buffer, ID3v2Frame* frame)
{
	// Each frame has 10 byte header in following format:  IIIISSSSFF
	//		IIII = 4 bytes for frame ID
	//		SSSS = 4 bytes for frame size (excludes frame header)
	//		FF = 2 bytes for frame flags
	
	memcpy(frame->id, buffer, ID3V2_FRAME_ID_LEN);
	
	for (int i = 0; i < ID3V2_FRAME_ID_LEN; i++) {
		// Check to make sure this frame has a valid, alphanumeric ID
		if (frame->id[i] < 48 || frame->id[i] > 90)
		{
			// Not alphanumeric character.  Invalid frame.
			return -1;
		}
	}

	// Get frame size
	unsigned char frame_size_bytes[ID3V2_FRAME_SIZE_LEN];
	memcpy(frame_size_bytes, buffer + ID3V2_FRAME_ID_LEN, ID3V2_FRAME_SIZE_LEN);
	frame->frame_size = ID3v2_DecodeFrameSize(frame_size_bytes);

	// Get frame flags
	memcpy(frame->flags, buffer + ID3V2_FRAME_ID_LEN + ID3V2_FRAME_SIZE_LEN, ID3V2_FRAME_FLAGS_LEN);
	
	// Get frame data pointer
	frame->data = (unsigned char*)buffer + ID3V2_FRAME_HEADER_LEN;

	return 0;
}


// Given the frame data with text encoding byte, convert it to a valid ANSI string.
char* ID3v2_FrameDataToString(ID3v2Frame* frame)
{
	// First byte of frame data indicates the text encoding
	// 00 = ISO-8859-1 (ASCII)
	// 01 = UTF-16 big endian OR little endian with BOM (ID3v2.2 and ID3v2.3)
	//		01 FF FE = UTF-16 little endian
	//		01 FE FF = UTF-16 big endian
	// 02 = UTF-16 big endian without BOM (ID3v2.4)
	// 03 = UTF-8 (ID3v2.4)
	// See:  https://en.wikipedia.org/wiki/ID3 and https://en.wikipedia.org/wiki/Byte_order_mark
	
	// Frame contains no data
	if (frame->frame_size <= 1)
		return NULL;
		
	char* result = NULL;
	HANDLE heap = GetProcessHeap();

	switch (frame->data[0]) {
		case ID3V2_FRAME_TEXT_ENC_ASCII:
		{
			result = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, frame->frame_size);
			memcpy(result, frame->data + 1, frame->frame_size - 1);

		} break;

		case ID3V2_FRAME_TEXT_ENC_UTF16_BOM:
		{
			// Convert from UTF-16 to ANSI
			wchar_t* wide_str = (wchar_t*)HeapAlloc(heap, HEAP_ZERO_MEMORY, frame->frame_size + 1);
			memcpy(wide_str, frame->data + 1, frame->frame_size - 1);
			result = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, frame->frame_size);
			
			// Add 1 to wide string pointer so that the BOM is skipped
			WideCharToMultiByte(CP_ACP, 0, wide_str + 1, -1, result, frame->frame_size, 0, 0);		
		} break;

		case ID3V2_FRAME_TEXT_ENC_UTF16_BE:
		{
			// TODO:  UTF-16 big endian.  Need samples of MP3s that use this encoding.
		} break;

		case ID3V2_FRAME_TEXT_ENC_UTF8:
		{
			// TODO:  UTF-8.  Need samples of MP3s that use this encoding.
		} break;
	}

	return result;
}


// Get the picture embedded in an ID3v2 tag.  This is typically album art in JPEG format.
ID3v2Image* ID3v2_GetAttachedPicture(unsigned char* frame_data, unsigned int frame_size)
{
	// APIC frames have an extra header that contains:
	//		- Text encoding byte
	//		- MIME type (e.g. image/jpeg)
	//		- Picture type byte (e.g. $03 = front album cover)
	//		- Description (text string with max length of 64 bytes)
	// See:  http://id3.org/metadata.3.0#Attached_picture
	// This header is typically 14 bytes, but not always.  Thus, we must
	// scan through the bytes until we find where the image data starts.

	// All JPEG/PNG images must start with the following bytes:
	// JPEG = \xFF\xD8\xFF
	// PNG = \x89\x50\x4E\x47\x0D\x0A\x1A\x0A

	ID3v2Image* img = (ID3v2Image*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3v2Image));
	
	// Iterate through each byte until we find where the image data starts
	// Since description is limited to 64 bytes, just scan the first 100 bytes
	for (int i = 0; i < 100; i++)
	{
		unsigned char* pos = frame_data + i;
		if (!memcmp(pos, JPEG_MAGIC_NUMBER, 3))
		{
			// Found JPEG data
			img->size = frame_size - (pos - frame_data);
			img->data = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, img->size);
			memcpy(img->data, pos, img->size);
			img->format = IMG_FORMAT_JPG;
			break;
		}
		else if (!memcmp(pos, PNG_MAGIC_NUMBER, 8))
		{
			// TODO: Need samples of MP3s that use PNG encoding for the attached image
			// Found PNG data
			img->size = frame_size - (pos - frame_data);
			img->data = (unsigned char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, img->size);
			memcpy(img->data, pos, img->size);
			img->format = IMG_FORMAT_PNG;
			break;
		}
	}

	return img;
}


// This function fills the specified AudioFileMetadata struct with info from the buffer.
// Returns 0 if success.  Returns non-zero otherwise.
void ParseID3v2(const char* buffer, AudioFileMetadata* metadata)
{
	// ID3v2 header is always first 10 bytes
	unsigned char raw_header[10];
	memcpy(raw_header, buffer, 10);
	
	// Must use memcmp instead of strcmp because it is not null-terminated string
	if (memcmp(raw_header, "ID3", 3)) {
		// Invalid ID3 tag.  Must begin with "ID3".
		return;
	}

	// Parse the raw header bytes
	ID3v2Header header = ID3v2_ParseHeader(raw_header);

	// Frame's starting position in relation to start of tag header.  Start with value of 10, because
	// first frame starts 10 bytes from start (first 10 bytes are the ID3v2 header).
	unsigned int frame_offset = ID3V2_HEADER_LEN;

	// Read through all the bytes and parse each frame
	while (frame_offset < header.tag_size) {

		ID3v2Frame frame = {};
		if (ID3v2_ParseFrame(buffer + frame_offset, &frame) == -1)
			break;
		
		// If memcmp() returns 0, that means the memory matches
		if (!memcmp(frame.id, ID3V2_TITLE_FRAME_ID, 4))
			metadata->title = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_ARTIST_FRAME_ID, 4))
			metadata->artist = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_ALBUM_FRAME_ID, 4))
			metadata->album = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_GENRE_FRAME_ID, 4))
			metadata->genre = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_YEAR_FRAME_ID, 4))
			metadata->date = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_TRACK_NUM_FRAME_ID, 4))
			metadata->track_num = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_COMMENT_FRAME_ID, 4))
			metadata->comment_description = ID3v2_FrameDataToString(&frame);
		else if (!memcmp(frame.id, ID3V2_ALBUM_ART_FRAME_ID, 4))
			metadata->album_art = ID3v2_GetAttachedPicture(frame.data, frame.frame_size);

		frame_offset += frame.frame_size + ID3V2_FRAME_HEADER_LEN;
	}
}


void ParseOggComments(const char* buffer, AudioFileMetadata* metadata)
{
	// From BASS documentation:
	// "A pointer to a series of null-terminated UTF-8 strings is returned,
	// the final string ending with a double null."
	// Reference:  http://www.un4seen.com/doc/#bass/BASS_ChannelGetTags.html
	unsigned int curr_pos = 0;
	int last_null_pos = -1;
	int last_equals_pos = -1;
	HANDLE heap = GetProcessHeap();
	while (true)
	{
		if (buffer[curr_pos] == '=')
		{
			last_equals_pos = curr_pos;
		}
		else if (buffer[curr_pos] == '\0')
		{
			if (last_null_pos == curr_pos - 1)
			{
				// Two consecutive null characters signals the end of the buffer
				break;
			}

			const char* field_name_ptr = buffer + last_null_pos + 1;
			unsigned int field_name_len = last_equals_pos - last_null_pos - 1;
			char* field_name = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, field_name_len + 1);
			memcpy(field_name, field_name_ptr, field_name_len);
			
			const char* field_value_ptr = buffer + last_equals_pos + 1;
			unsigned int field_value_len = curr_pos - last_equals_pos - 1;
			char* field_value = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, field_value_len + 1);
			memcpy(field_value, field_value_ptr, field_value_len);
						
			// OGG field names are NOT case sensitive, so must use case insensitive string compare
			// Ex: "ARTIST", "Artist", and "artist" are all valid field identifiers
			if (!lstrcmpi(field_name, OGG_TITLE_FIELD))
				metadata->title = field_value;
			if (!lstrcmpi(field_name, OGG_ARTIST_FIELD))
				metadata->artist = field_value;
			if (!lstrcmpi(field_name, OGG_ALBUM_FIELD))
				metadata->album = field_value;
			if (!lstrcmpi(field_name, OGG_GENRE_FIELD))
				metadata->genre = field_value;
			if (!lstrcmpi(field_name, OGG_TRACK_NUM_FIELD))
				metadata->track_num = field_value;
			if (!lstrcmpi(field_name, OGG_DATE_FIELD))
				metadata->date = field_value;
			if (!lstrcmpi(field_name, OGG_DESCRIPTION_FIELD))
				metadata->comment_description = field_value;

			last_null_pos = curr_pos;
		}

		curr_pos++;
	}
}