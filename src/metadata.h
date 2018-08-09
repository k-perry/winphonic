/******************************************************************************
metadata.h - Header file for metadata.cpp
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

#include "util.h"
#include "image.h"


// Functions for reading ID3v2 from MP3 and comments from OGG files

// MP3 ID3v2 ===============================================================================

// Constants for MP3 ID3v2
// Frame IDs are case sensitive
#define ID3V2_TITLE_FRAME_ID			"TIT2"
#define ID3V2_ARTIST_FRAME_ID			"TPE1"
#define ID3V2_ALBUM_FRAME_ID			"TALB"
#define ID3V2_GENRE_FRAME_ID			"TCON"
#define ID3V2_TRACK_NUM_FRAME_ID		"TRCK"
#define ID3V2_YEAR_FRAME_ID				"TYER"
#define ID3V2_COMMENT_FRAME_ID			"COMM"
#define ID3V2_COMPOSER_FRAME_ID			"TCOM"
#define ID3V2_ALBUM_ART_FRAME_ID		"APIC"
#define ID3V2_FRAME_TEXT_ENC_ASCII		0
#define ID3V2_FRAME_TEXT_ENC_UTF16_BOM	1
#define ID3V2_FRAME_TEXT_ENC_UTF16_BE	2
#define ID3V2_FRAME_TEXT_ENC_UTF8		3
#define ID3V2_HEADER_LEN				10
#define ID3V2_FRAME_HEADER_LEN			10
#define ID3V2_FRAME_ID_LEN				4
#define ID3V2_FRAME_SIZE_LEN			4
#define ID3V2_FRAME_FLAGS_LEN			2
#define ID3V2_FRAME_HEADER_LEN			10


// All JPEG/PNG images must start with the following bytes, respectively
// Sources:
// https://en.wikipedia.org/wiki/JPEG
// https://en.wikipedia.org/wiki/Portable_Network_Graphics
#define JPEG_MAGIC_NUMBER	"\xFF\xD8\xFF"
#define PNG_MAGIC_NUMBER	"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"

struct ID3v2Image {
	unsigned char* data;
	unsigned int size;
	int format;						// IMG_FORMAT_JPG = 0, IMG_FORMAT_PNG = 1
};

struct ID3v2Header {
	unsigned int major_version;		// e.g. for ID3v2.3.1, this will be "3" (???)
	unsigned int revision;			// e.g. for ID3v2.3.1, this will be "1" (???)
	unsigned char flags;
	unsigned int tag_size;			// Size of entire ID3v2 tag, including any album art.  NOT the size of the header.
};

struct ID3v2Frame {
	unsigned char id[4];			// e.g. "TIT2" for title
	unsigned int frame_size;
	unsigned char flags[2];
	unsigned char* data;			// Raw frame data. Can be string or JPEG.
};

struct AudioFileMetadata {
	char* title;
	char* artist;
	char* album;
	char* genre;
	char* track_num;
	char* date;
	char* comment_description;	// Comment (ID3v2) or description (OGG)
	ID3v2Image* album_art;
};
// ================================================================================================

// OGG Comments ===================================================================================
// Reference:  https://xiph.org/vorbis/doc/v-comment.html

// Constants for OGG comments
// Field IDs are NOT case sensitive
#define OGG_TITLE_FIELD					"TITLE"
#define OGG_ARTIST_FIELD				"ARTIST"
#define OGG_ALBUM_FIELD					"ALBUM"
#define OGG_GENRE_FIELD					"GENRE"
#define OGG_TRACK_NUM_FIELD				"TRACKNUMBER"
#define OGG_DATE_FIELD					"DATE"
#define OGG_DESCRIPTION_FIELD			"DESCRIPTION"

// ================================================================================================

// Functions
void ParseID3v2(const char* buffer, AudioFileMetadata* metadata);
void ParseOggComments(const char* buffer, AudioFileMetadata* metadata);