/******************************************************************************
main.h - #defines, structs, and declarations for main.cpp
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

// Forces linking with common controls v6.0, which allows us to use a SysLink control
// for hyperlinks in the "About" dialog
#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")


// Windows libraries to link with
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "windowscodecs.lib")

// Link with BASS audio library
#pragma comment(lib, "..\\lib\\bass.lib")

#define _ITERATOR_DEBUG_LEVEL 0		// To prevent invalid std::vector assertion errors in VS2017

#include <Windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <vector>
#include <algorithm>
#include <strsafe.h>

#include "resource.h"
#include "bass.h"
#include "util.h"
#include "img_button.h"
#include "text_button.h"
#include "trackbar.h"
#include "text_label.h"
#include "img_label.h"
#include "metadata.h"
#include "about_dialog.h"

static HWND g_about_dlg_hwnd;		// Handle for the "About" dialog box

// GUI constants
#define WIN_WIDTH			420
#define WIN_HEIGHT			180
#define BACKGROUND_COLOR	RGB(40, 40, 40)
#define TITLEBAR_COLOR		RGB(20, 20, 20)
#define ALBUM_ART_COLOR		RGB(35, 35, 35)
#define BTN_NORMAL_COLOR	BACKGROUND_COLOR
#define BTN_HOVER_COLOR		RGB(60, 60, 60)
#define BTN_PRESSED_COLOR	TITLEBAR_COLOR
#define TEXT_COLOR			RGB(225, 225, 225)
#define TOOLTIP_COLOR		RGB(50, 50, 50)
#define PLAYLIST_COLOR		RGB(255, 255, 255)			// Background color for playlist control
#define PLAYLIST_TEXT_COLOR			RGB(0, 0, 0)		// Font color for normal song in playlist
#define PLAYLIST_CURRENT_COLOR		RGB(0, 175, 0)		// Font color for current song in playlist
#define PLAYLIST_INVALID_COLOR		RGB(150, 150, 150)	// Font color for invalid song in playlist
#define PLAYLIST_AREA_COLOR			RGB(90, 90, 90)		// Background color for area around playlist

// Control IDs for WM_COMMAND messages
#define BTN_MIN				100
#define BTN_CLOSE			101
#define BTN_PREV			102
#define BTN_PLAY			103
#define BTN_PAUSE			104
#define BTN_STOP			105
#define BTN_NEXT			106
#define BTN_SHUFFLE			107
#define BTN_REPEAT			108
#define BTN_OPEN			109
#define BTN_PLAYLIST		110
#define BTN_SETTINGS		111
#define BTN_ADD				112
#define BTN_DELETE			113
#define BTN_MOVE_UP			114
#define BTN_MOVE_DOWN		115
#define BTN_MORE			116
#define LBL_TITLE			200
#define LBL_ARTIST			201
#define LBL_ALBUM			202
#define LBL_FILE_INFO		203
#define LBL_TIME_POS		204		// Current song position
#define LBL_TIME_LEN		205		// Length of song (or time remaining)
#define LBL_PL_INFO			206		// Playlist info
#define LBL_ALBUM_ART		300		// Image label for showing album art

// Timer IDs
#define TIMER_UPDATE_SONG_POS		1
#define TIMER_REVERT_TITLE			2

// IDs for popup menu items
#define IDM_ALWAYS_ON_TOP	1
#define IDM_SNAP_TO_EDGES	2
#define IDM_PLAYLIST_SMALL	3
#define IDM_PLAYLIST_MEDIUM	4
#define IDM_PLAYLIST_LARGE	5
#define IDM_ABOUT			6
#define IDM_EXIT			7

// Settings INI file
#define SETTINGS_SECTION		"Winphonic Settings"
#define SETTINGS_INI_FILE_NAME	"settings.ini"


enum PlayerStateType { STOPPED, PLAYING, PAUSED };
enum FileFormat { MP3, OGG, AAC, FLAC };
enum PlaylistSize { SMALL = 250, MEDIUM = 500, LARGE = 750};

struct ControlHandles {
	HWND tb_pos;
	HWND tb_vol;
	HWND lbl_album_art;
	HWND lbl_title;
	HWND lbl_artist;
	HWND lbl_album;
	HWND lbl_file_info;
	HWND lbl_time_pos;
	HWND lbl_time_length;
	HWND lbl_pl_info;
	HWND btn_min;
	HWND btn_close;
	HWND btn_prev;
	HWND btn_play;
	HWND btn_pause;
	HWND btn_stop;
	HWND btn_next;
	HWND btn_shuffle;
	HWND btn_repeat;
	HWND btn_open;
	HWND btn_playlist;
	HWND btn_settings;
	HWND btn_add;
	HWND btn_delete;
	HWND btn_move_up;
	HWND btn_move_down;
	HWND btn_more;
	HWND playlist_hwnd;
	HWND dlg_about;

};

// Entries in the playlist
struct Song {
	bool is_current;
	bool is_valid;		// Invalid (e.g. deleted) songs will be drawn in playlist grayed out
	AudioFileMetadata metadata;
	char* playlist_song_name;
	char song_length_str[8] = {};	// Song length displayed in playlist (e.g. "4:13")
	unsigned int song_length_secs = 0;
	QWORD song_length_bytes;	// Song length in bytes.  QWORD = unsigned int64
	char* path;					// Full file path including file name, e.g. C:\Music\Directory\Artist - Song.mp3
	char* file_name;			// e.g. Artist - Song.mp3
	unsigned int bitrate;		// e.g. 256 kbps
	unsigned int frequency;		// e.g. 44100 hertz
	bool is_stereo;
	FileFormat format;
	bool has_info;				// Was song info already looked up?
};


struct GDIObjects {
	HBRUSH main_bg_brush;
	HBRUSH titlebar_brush;
	HBRUSH playlist_bg_brush;		// Region around the playlist
	HFONT titlebar_font;
	HFONT playlist_font_normal;
	HFONT playlist_font_current;
	HFONT playlist_font_invalid;
};

struct Options {
	bool shuffle;
	bool repeat;
	bool always_on_top;
	bool snap_to_edges;
	PlaylistSize playlist_size;
	int x;
	int y;
};

// Main application state
struct AppState {
	HINSTANCE instance;
	HWND main_hwnd;
	ControlHandles controls;
	GDIObjects gdi;
	std::vector<Song*> playlist_view;	// Playlist as shown in the playlist ListView window
	std::vector<Song*> playlist;		// Actual playlist. If shuffle is on, it will be different order from playlist_view
	Song* curr_song;					// Pointer to the current song
	HSTREAM bass_stream;
	PlayerStateType player_state = STOPPED;
	unsigned int volume;
	Options options;
	bool is_playlist_visible = false;	// Is the playlist_view visible?
	bool is_running = false;			// Is the program running?
	int width;							// Current window width
	int height;							// Current window height
	char ini_path[MAX_PATH];			// Full path to INI settings file
};



// Function declarations
static void KeyDownHandler(AppState* state, int key_code);
static void PaintWindow(AppState* state);
static int PaintPlaylist(AppState* state, LPNMLVCUSTOMDRAW lv_custom_draw);
static void TimerHandler(AppState* state, int timer_id);
static void MouseWheelHandler(AppState* state, short wheel_dir);
static void PlaylistDoubleClickHandler(AppState* state);
static void PlayBtnHandler(AppState* state);
static void PauseBtnHandler(AppState* state);
static void StopBtnHandler(AppState* state);
static void PrevBtnHandler(AppState* state);
static void NextBtnHandler(AppState* state);
static void ShuffleBtnHandler(AppState* state, bool toggle);
static void RepeatBtnHandler(AppState* state, bool toggle);
static void MoveUpBtnHandler(AppState* state);
static void MoveDownBtnHandler(AppState* state);
static void SettingsBtnHandler(AppState* state);
static void ResetPositionTrackbar(HWND tb_pos, int min, int max, int pos);
static void ClearInfoLabels(ControlHandles* controls, HWND main_hwnd);
static void ResizePlaylist(int playlist_size, HWND main_hwnd, ControlHandles* controls,
	bool* is_playlist_visible, bool always_on_top);
static void FreeSong(Song* song);
static void DeleteSongFromPlaylist(AppState* state, int pl_view_idx_to_del);
static void UpdateInfoLabels(AppState* state, bool display_song_len);
static void TogglePlaylistVisible(HWND hwnd, bool* is_playlist_visible, bool toggle, 
	int playlist_size, HWND btn_playlist, bool always_on_top);
static void GetSongInfo(Song* song);
static void RedrawPlaylistWindow(HWND playlist_hwnd, unsigned int num_items);
static void UpdatePlaylistWindow(HWND playlist_hwnd, std::vector<Song*>& playlist_view, HWND lbl_pl_info);
static void GetPlaylistFromFileList(std::vector<Song*>& playlist_view, char* file_list, size_t file_list_len);
static void GetPlaylistFromFileBuffer(std::vector<Song*>& playlist_view, char* file_buffer, const int file_buffer_size,
	const int file_offset, char* file_title);
static void OpenFile(AppState* state, bool is_add_btn);
static void CreateButtons(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance, int playlist_size);
static void CreateTrackbars(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance);
static void CreateTextLabels(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance, int playlist_size);
static void CreateImgLabels(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance);
static HWND CreatePlaylistWindow(HWND main_hwnd, HINSTANCE instance, HFONT pl_font, int playlist_size);
static void CreateGDIObjects(AppState* state);
static bool LoadCurrentSong(AppState* state);
static int GetPlaylistCurrentIndex(std::vector<Song*> &playlist);
static int GetPrevSongIndex(unsigned int curr_idx, unsigned int pl_size, bool repeat);
static bool SelectPrevSong(AppState* state);
static int GetNextSongIndex(unsigned int curr_idx, unsigned int pl_size, bool repeat);
static bool SelectNextSong(AppState* state);
static void Initialize(AppState* state);
static void ReadSettings(AppState* state, char* ini_path);
static void ReadPlaylistFromSettings(AppState* state, char* ini_path);
static void WriteSettings(AppState* state, char* ini_path);
static void WritePlaylistToSettings(AppState* state, char* ini_path);
LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_code);