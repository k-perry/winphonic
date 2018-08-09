/******************************************************************************
main.cpp - Entry point; handles audio playback, playlists, and GUI
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


#include "main.h"

#define WIN32_LEAN_AND_MEAN

static void GetINIFilePath(char* ini_path, size_t len)
{
	char current_dir[MAX_PATH];

	// Get the current directory
	GetModuleFileName(NULL, current_dir, MAX_PATH);
	RemoveFilenameFromPath(current_dir, MAX_PATH);

	// Append the INI file name to end
	StringCbCopyA(ini_path, len, current_dir);
	StringCbCatA(ini_path, len, "settings.ini");
}


// Write settings to INI file
static void WriteSettings(AppState* state, char* ini_path)
{
	char always_on_top[2];
	StringCbPrintfA(always_on_top, ARRAYSIZE(always_on_top), "%i", (state->options.always_on_top) ? 1 : 0);
	WritePrivateProfileString(SETTINGS_SECTION, "AlwaysOnTop", always_on_top, ini_path);

	char snap_to_edges[2];
	StringCbPrintfA(snap_to_edges, ARRAYSIZE(snap_to_edges), "%i", (state->options.snap_to_edges) ? 1 : 0);
	WritePrivateProfileString(SETTINGS_SECTION, "SnapToEdges", snap_to_edges, ini_path);

	char shuffle[2];
	StringCbPrintfA(shuffle, ARRAYSIZE(shuffle), "%i", (state->options.shuffle) ? 1 : 0);
	WritePrivateProfileString(SETTINGS_SECTION, "Shuffle", shuffle, ini_path);

	char repeat[2];
	StringCbPrintfA(repeat, ARRAYSIZE(repeat), "%i", (state->options.repeat) ? 1 : 0);
	WritePrivateProfileString(SETTINGS_SECTION, "Repeat", repeat, ini_path);

	char volume[4];
	StringCbPrintfA(volume, ARRAYSIZE(volume), "%u", state->volume);
	WritePrivateProfileString(SETTINGS_SECTION, "Volume", volume, ini_path);

	char playlist_size[4];
	StringCbPrintfA(playlist_size, ARRAYSIZE(playlist_size), "%i", state->options.playlist_size);
	WritePrivateProfileString(SETTINGS_SECTION, "PlaylistSize", playlist_size, ini_path);

	// Get current window position and save it
	RECT window_rect;
	GetWindowRect(state->main_hwnd, &window_rect);
	char x[10];		// long range on x86: -2,147,483,648 to +2,147,483,647
	StringCbPrintfA(x, ARRAYSIZE(x), "%li", window_rect.left);
	WritePrivateProfileString(SETTINGS_SECTION, "X", x, ini_path);
	char y[10];
	StringCbPrintfA(y, ARRAYSIZE(y), "%li", window_rect.top);
	WritePrivateProfileString(SETTINGS_SECTION, "Y", y, ini_path);

	char playlist_visible[2];
	StringCbPrintfA(playlist_visible, ARRAYSIZE(playlist_visible), "%i", (state->is_playlist_visible) ? 1 : 0);
	WritePrivateProfileString(SETTINGS_SECTION, "PlaylistVisible", playlist_visible, ini_path);

	char curr_song_idx[10];
	StringCbPrintfA(curr_song_idx, ARRAYSIZE(curr_song_idx), "%i", GetPlaylistCurrentIndex(state->playlist_view));
	WritePrivateProfileString(SETTINGS_SECTION, "CurrentSongIndex", curr_song_idx, ini_path);

	WritePlaylistToSettings(state, ini_path);
}

static void ReadSettings(AppState* state, char* ini_path)
{
	state->options.always_on_top = (GetPrivateProfileInt(SETTINGS_SECTION,
		"AlwaysOnTop", 0, state->ini_path)) ? true : false;
	state->options.snap_to_edges = (GetPrivateProfileInt(SETTINGS_SECTION,
		"SnapToEdges", 0, state->ini_path)) ? true : false;
	state->options.shuffle = (GetPrivateProfileInt(SETTINGS_SECTION, "Shuffle",
		0, state->ini_path)) ? true : false;
	state->options.repeat = (GetPrivateProfileInt(SETTINGS_SECTION, "Repeat",
		0, state->ini_path)) ? true : false;
	state->volume = GetPrivateProfileInt(SETTINGS_SECTION, "Volume", 20, state->ini_path);
	state->options.playlist_size = static_cast<PlaylistSize>(GetPrivateProfileInt(SETTINGS_SECTION,
		"PlaylistSize", 0, state->ini_path));
	if (!state->options.playlist_size)
		state->options.playlist_size = SMALL;

	state->options.x = (int)GetPrivateProfileInt(SETTINGS_SECTION, "X", 0, state->ini_path);
	state->options.y = (int)GetPrivateProfileInt(SETTINGS_SECTION, "Y", 0, state->ini_path);
	// Basic sanity check for the X and Y values from settings.ini
	int max_x = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	if (state->options.x > max_x || state->options.x < -max_x)
		state->options.x = 0;
	int max_y = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	if (state->options.y > max_y || state->options.y < -max_y)
		state->options.y = 0;

	state->is_playlist_visible = (GetPrivateProfileInt(SETTINGS_SECTION, "PlaylistVisible",
		0, state->ini_path)) ? true : false;

}


static void WritePlaylistToSettings(AppState* state, char* ini_path)
{
	if (!state || !state->playlist_view.size() || !ini_path)
		return;

	// Start by copying the first file path to the buffer
	size_t file_list_buffer_len = lstrlen(state->playlist_view[0]->path) + 1;
	char* file_list_buffer = DuplicateString(state->playlist_view[0]->path);
	HANDLE heap = GetProcessHeap();
	for (unsigned int i = 1; i < state->playlist_view.size(); i++)
	{
		// Increase the file list buffer size to hold this file path
		file_list_buffer_len += lstrlen(state->playlist_view[i]->path) + 2;
		file_list_buffer = (char*)HeapReAlloc(heap, HEAP_ZERO_MEMORY, file_list_buffer, file_list_buffer_len);
		StringCbCatA(file_list_buffer, file_list_buffer_len, "|");	// Add the separator
		StringCbCatA(file_list_buffer, file_list_buffer_len, state->playlist_view[i]->path);	// Add the file path
	}
	WritePrivateProfileString(SETTINGS_SECTION, "PlaylistFiles", file_list_buffer, ini_path);
	HeapFree(heap, 0, file_list_buffer);
}


static void ReadPlaylistFromSettings(AppState* state, char* ini_path)
{
	// Get the list of files that were in playlist last run.  Start with small buffer and increase as needed.
	size_t buffer_size = 256;
	char* file_list_buffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_size);
	DWORD bytes_read = 0;
	while (true)
	{
		bytes_read = GetPrivateProfileString(SETTINGS_SECTION, "PlaylistFiles", 0, file_list_buffer,
			buffer_size, state->ini_path);
		if (bytes_read < buffer_size - 1 || bytes_read == 0)
			break;

		// Buffer is too small, so double the size and try again
		buffer_size *= 2;
		file_list_buffer = (char*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, file_list_buffer, buffer_size);
	}

	if (bytes_read)
	{
		// If successfully read some bytes, then try to make a playlist
		GetPlaylistFromFileList(state->playlist_view, file_list_buffer, bytes_read);

		for (unsigned int i = 0; i < state->playlist_view.size(); i++)
		{
			GetSongInfo(state->playlist_view[i]);
		}
		UpdatePlaylistWindow(state->controls.playlist_hwnd, state->playlist_view, state->controls.lbl_pl_info);

		// Copy vector
		state->playlist = state->playlist_view;

		if (state->options.shuffle)
		{
			std::random_shuffle(state->playlist.begin(), state->playlist.end());
			int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
			if (curr_pl_idx > 0)
			{
				// Swap so that the current song is first in the playlist
				std::swap(state->playlist[curr_pl_idx], state->playlist[0]);
			}
		}

		UINT curr_song_idx = GetPrivateProfileInt(SETTINGS_SECTION, "CurrentSongIndex", 0, state->ini_path);
		if (curr_song_idx >= state->playlist.size() || curr_song_idx < 0)
			curr_song_idx = 0;
		state->curr_song = state->playlist_view[curr_song_idx];
		state->curr_song->is_current = true;
		RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist_view.size());
		ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
		UpdateInfoLabels(state, false);
		//LoadCurrentSong(state);
	}

	HeapFree(GetProcessHeap(), 0, file_list_buffer);

}

static void KeyDownHandler(AppState* state, int key_code)
{
	switch (key_code)
	{
		case 0x5A:		// Z = previous
		case VK_LEFT:	// Left Arrow = previous
		{
			PrevBtnHandler(state);
		} break;

		case 0x58:		// X = play
		{
			PlayBtnHandler(state);
		} break;

		case 0x43:		// C = pause
		{
			PauseBtnHandler(state);
		} break;
		
		case 0x56:		// V = stop
		case VK_ESCAPE:	// ESC = stop
		{
			if (state->player_state != STOPPED)
				StopBtnHandler(state);
		} break;

		case 0x42:		// B = next
		case VK_RIGHT:	// Right Arrow = next
		{
			NextBtnHandler(state);
		} break;

		case VK_SPACE:	// SPACE = play/pause
		{
			if (state->player_state == PLAYING)
				PauseBtnHandler(state);
			else
				PlayBtnHandler(state);
		} break;

		case 0x4F:	 // O = open file
		{
			OpenFile(state, false);
		} break;

		case 0x41:	// A = add file to playlist
		{	
			OpenFile(state, true);
			// Display the playlist window, if necesary
			if (!state->is_playlist_visible)
				TogglePlaylistVisible(state->main_hwnd, &state->is_playlist_visible, 
					true, state->options.playlist_size, state->controls.btn_playlist, 
					state->options.always_on_top);
		} break;

		case 0x50:	// P = toggle playlist visible
		{
			TogglePlaylistVisible(state->main_hwnd, &state->is_playlist_visible,
				true, state->options.playlist_size, state->controls.btn_playlist,
				state->options.always_on_top);
		} break;

		case 0x52:	// R = toggle repeat
		{
			RepeatBtnHandler(state, true);
		} break;

		case 0x53:	// S = toggle shuffle
		{
			ShuffleBtnHandler(state, true);
		} break;

		case VK_DELETE:		// Delete = delete file from playlist
		{
			const int sel_idx = SendMessage(state->controls.playlist_hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
			if (sel_idx >= 0)
				DeleteSongFromPlaylist(state, sel_idx);
		} break;

		case VK_RETURN:		// Enter/Return = play currently selected song
		{
			PlaylistDoubleClickHandler(state);
		} break;
	}
}


// Paints the window background and titlebar
static void PaintWindow(AppState* state)
{
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(state->main_hwnd, &paint);
	RECT main_rect = { 0, 0, WIN_WIDTH, WIN_HEIGHT };
	RECT playlist_rect = { 0, WIN_HEIGHT, WIN_WIDTH, WIN_HEIGHT + state->options.playlist_size };

	FillRect(dc, &main_rect, state->gdi.main_bg_brush);				// Main window area
	FillRect(dc, &playlist_rect, state->gdi.playlist_bg_brush);		// Playlist area is different color
	
	RECT titlebar_rect = { 0, 0, paint.rcPaint.right, 30 };		// Draw the title bar rectangle
	FillRect(dc, &titlebar_rect, state->gdi.titlebar_brush);

	// Draw title bar text
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, TEXT_COLOR);
	HFONT old_font = (HFONT)SelectObject(dc, state->gdi.titlebar_font);
	titlebar_rect.left = 35;
	DrawTextA(dc, "Winphonic", -1, &titlebar_rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
	SelectObject(dc, old_font);

	EndPaint(state->main_hwnd, &paint);
}


// Performs custom draw for playlist listview control.  Draws the currently playing
// song in a different font and color.
static int PaintPlaylist(AppState* state, LPNMLVCUSTOMDRAW lv_custom_draw)
{
	// Playlist window custom draw
	switch (lv_custom_draw->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
		{
			return CDRF_NOTIFYITEMDRAW;
		} break;

		case CDDS_ITEMPREPAINT:
		{
			const unsigned int item_idx = (unsigned int)lv_custom_draw->nmcd.dwItemSpec;
			
			if (state->playlist_view[item_idx]->is_valid)
			{
				if (state->playlist_view[item_idx]->is_current)
				{
					// Currently playing song
					lv_custom_draw->clrText = PLAYLIST_CURRENT_COLOR;
					lv_custom_draw->clrTextBk = PLAYLIST_COLOR;
					SelectObject(lv_custom_draw->nmcd.hdc, state->gdi.playlist_font_current);
				}
				else
				{
					// Normal playlist entry
					lv_custom_draw->clrText = PLAYLIST_TEXT_COLOR;
					lv_custom_draw->clrTextBk = PLAYLIST_COLOR;
					SelectObject(lv_custom_draw->nmcd.hdc, state->gdi.playlist_font_normal);

				}
			}
			else
			{
				// Invalid playlist entry
				lv_custom_draw->clrText = PLAYLIST_INVALID_COLOR;
				lv_custom_draw->clrTextBk = PLAYLIST_COLOR;
				SelectObject(lv_custom_draw->nmcd.hdc, state->gdi.playlist_font_invalid);
			}
			
			return CDRF_NEWFONT;
		} break;
	}
	return 0;
}


static void TimerHandler(AppState* state, int timer_id)
{
	if (timer_id == TIMER_UPDATE_SONG_POS)
	{
		if (!state->bass_stream)
			return;

		if (BASS_ChannelIsActive(state->bass_stream) == BASS_ACTIVE_STOPPED &&
			state->player_state != STOPPED)
		{
			// Song finished playing, so play the next song in playlist
			int curr_song_idx = GetPlaylistCurrentIndex(state->playlist);
			if (curr_song_idx == (int)state->playlist.size() - 1 && !state->options.repeat)
			{
				// Last song in playlist and repeat is turned off
				ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
				SendMessage(state->controls.lbl_time_pos, WM_SETTEXT, 0, 0);
				SendMessage(state->controls.lbl_time_length, WM_SETTEXT, 0, 0);
				state->player_state = STOPPED;
				return;
			}
			
			int count = 0;		// Only try each song in playlist once before giving up
			while (!SelectNextSong(state))
			{
				// Keep trying until we find a playable song
				count++;
				if (count >= (int) state->playlist.size())
				{
					// Could not find a valid song in the playlist.  Clean up.
					ClearInfoLabels(&state->controls, state->main_hwnd);
					ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
					state->player_state = STOPPED;
					return;
				}
			}
			UpdateInfoLabels(state, true);
			BASS_ChannelPlay(state->bass_stream, false);
			state->player_state = PLAYING;
		}
		else if (state->player_state == PLAYING)
		{
			// Song isn't finished, so update the time label and the menu_pos trackbar
			const QWORD position = BASS_ChannelGetPosition(state->bass_stream, BASS_POS_BYTE);
			const int position_seconds = (int)BASS_ChannelBytes2Seconds(state->bass_stream, position);
			char time[8];
			StringCbPrintfA(time, 8, "%u:%02u", position_seconds / 60, position_seconds % 60);
			SendMessage(state->controls.lbl_time_pos, WM_SETTEXT, 0, (LPARAM)time);
			SendMessage(state->controls.tb_pos, WP_TBM_SETPOS, 0, position_seconds);
		}
	}
	else if (timer_id == TIMER_REVERT_TITLE)
	{
		// Revert to the previous song title, then delete this timer
		if (state->curr_song != NULL)
		{
			if (state->curr_song->metadata.title)
				SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.title);
			else
				SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->file_name);
		}
		else
		{
			SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, 0);
		}
		KillTimer(state->main_hwnd, TIMER_REVERT_TITLE);
	}
}


// Mouse wheel changes volume
static void MouseWheelHandler(AppState* state, short wheel_dir)
{
	int vol = SendMessage(state->controls.tb_vol, WP_TBM_GETPOS, 0, 0);
	int change_amt = 3;		// Amount that each mouse wheel "click" should change the volume
	
	if (wheel_dir == WHEEL_DELTA)
	{
		// Scrolled forward (up)
		if (vol <= (100 - change_amt))
			vol += change_amt;
		else
			vol = 100;
	}
	else
	{
		// Scrolled backward (down)
		if (vol >= change_amt)
			vol -= change_amt;
		else
			vol = 0;
	}
	state->volume = vol;
	SendMessage(state->controls.tb_vol, WP_TBM_SETPOS, 0, state->volume);
	if (state->bass_stream)
		BASS_ChannelSetAttribute(state->bass_stream, BASS_ATTRIB_VOL, state->volume / 100.0f);

	char vol_text[16];
	StringCbPrintfA(vol_text, 16, "Volume:  %u%%", state->volume);
	SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)vol_text);
	// Use timer to revert to the previous title after 1 second (1000 ms)
	SetTimer(state->main_hwnd, TIMER_REVERT_TITLE, 1000, NULL);
}


static void PlaylistDoubleClickHandler(AppState* state)
{
	// User double-clicked item in playlist_view
	const int sel_idx = SendMessage(state->controls.playlist_hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (sel_idx >= 0)
	{
		if (state->curr_song != NULL)
			state->curr_song->is_current = false;

		state->curr_song = state->playlist_view[sel_idx];
		state->curr_song->is_current = true;
		if (LoadCurrentSong(state))
		{
			RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist_view.size());
			UpdateInfoLabels(state, true);
			// Set volume to current value from the volume trackbar
			//state->volume = SendMessage(state->controls.tb_vol, WP_TBM_GETPOS, 0, 0);
			if (state->bass_stream)
				BASS_ChannelSetAttribute(state->bass_stream, BASS_ATTRIB_VOL, state->volume / 100.0f);

			BASS_ChannelPlay(state->bass_stream, false);
			state->player_state = PLAYING;
		}
		else
		{
			state->player_state = STOPPED;
		}
		
	}
}


static void PlayBtnHandler(AppState* state)
{
	if (state->bass_stream)
	{
		if (state->player_state == PLAYING)
		{
			// Restart track from beginning
			BASS_ChannelPlay(state->bass_stream, true);
			SendMessage(state->controls.tb_pos, WP_TBM_SETPOS, 0, 0);
			state->player_state = PLAYING;
		}
		else if (state->player_state == STOPPED)
		{
			if (state->curr_song != NULL)
			{
				BASS_ChannelPlay(state->bass_stream, true);
				ResetPositionTrackbar(state->controls.tb_pos, 0, state->curr_song->song_length_secs, 0);
				SendMessage(state->controls.lbl_time_length, WM_SETTEXT, 0, (LPARAM)&state->curr_song->song_length_str);
				state->player_state = PLAYING;
			}
			else
			{
				// User has deleted the current song from playlist.  Clean up.
				ClearInfoLabels(&state->controls, state->main_hwnd);
				BASS_StreamFree(state->bass_stream);
				state->bass_stream = 0;

				if (state->playlist.size() > 0)
				{

					int count = 0;
					while (!SelectNextSong(state))
					{
						// Keep trying until we find a playable song
						count++;
						if (count >= (int) state->playlist.size())
						{
							// Could not find a valid song in the playlist.  Clean up.
							ClearInfoLabels(&state->controls, state->main_hwnd);
							ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
							state->player_state = STOPPED;
							return;
						}
					}
					UpdateInfoLabels(state, true);
					BASS_ChannelPlay(state->bass_stream, false);
					state->player_state = PLAYING;
				}
				else
				{
					// Playlist is empty.  Ask the user if they want to open a file.
					OpenFile(state, false);
				}
			}

		}
		else if (state->player_state == PAUSED)
		{
			BASS_ChannelPlay(state->bass_stream, false);
			state->player_state = PLAYING;
		}
	}
	else
	{
		// There is no BASS stream
		if (state->playlist.size() > 0)
		{
			if (state->curr_song == NULL)
			{
				// If there are songs in the playlist, and none of them are the current song,
				// select the first song in playlist, and play it.
				state->curr_song = state->playlist[0];
				state->curr_song->is_current = true;
			}
			if (LoadCurrentSong(state))
			{
				RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist_view.size());
				UpdateInfoLabels(state, true);
				BASS_ChannelPlay(state->bass_stream, false);
				state->player_state = PLAYING;
			}
		}
		else
		{
			// If there are no songs in playlist, show the open file dialog
			OpenFile(state, false);
		}
	}
}

static void PauseBtnHandler(AppState* state)
{
	if (state->bass_stream)
	{
		if (state->player_state == PAUSED)
		{
			// Stream is already paused, so pause button resumes it
			BASS_ChannelPlay(state->bass_stream, false);
			state->player_state = PLAYING;
		}
		else if (state->player_state == PLAYING)
		{
			// Stream is playing, so pause button pauses it
			BASS_ChannelPause(state->bass_stream);
			state->player_state = PAUSED;
		}
	}
}

static void StopBtnHandler(AppState* state)
{
	if (state->bass_stream)
	{
		BASS_ChannelStop(state->bass_stream);

		if (state->player_state == PLAYING || state->player_state == PAUSED)
		{
			SendMessage(state->controls.lbl_time_pos, WM_SETTEXT, 0, 0);
			SendMessage(state->controls.lbl_time_length, WM_SETTEXT, 0, 0);
			ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
		}
		if (state->curr_song == NULL)
		{
			// User has deleted the current song.  Clean up everything.
			// Clear all the labels.
			ClearInfoLabels(&state->controls, state->main_hwnd);
			BASS_StreamFree(state->bass_stream);
			state->bass_stream = 0;
			state->curr_song = NULL;
		}

		state->player_state = STOPPED;
	}
}

static void PrevBtnHandler(AppState* state)
{
	if (state->playlist_view.size() > 0)
	{
		const int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
		if (state->options.repeat || curr_pl_idx > 0)
		{
			if (state->bass_stream)
			{
				BASS_ChannelStop(state->bass_stream);
				BASS_StreamFree(state->bass_stream);
				state->bass_stream = 0;
			}
			
			int count = 0;
			while (!SelectPrevSong(state))
			{
				// Keep trying until we find a playable song
				count++;
				if (count >= (int) state->playlist.size())
				{
					// Could not find a valid song in the playlist.  Clean up.
					ClearInfoLabels(&state->controls, state->main_hwnd);
					ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
					state->player_state = STOPPED;
					return;
				}
			}
			if (state->player_state == PLAYING || state->player_state == PAUSED)
			{
				UpdateInfoLabels(state, true);
				ResetPositionTrackbar(state->controls.tb_pos, 0, state->curr_song->song_length_secs, 0);
				BASS_ChannelPlay(state->bass_stream, false);
				state->player_state = PLAYING;
			}
			else
			{	// player_state = STOPPED
				UpdateInfoLabels(state, false);
				ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
			}

		}
	}
}

static void NextBtnHandler(AppState* state)
{
	if (state->playlist.size() > 0)
	{
		const int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
		if (state->options.repeat || curr_pl_idx < (int)state->playlist.size() - 1)
		{
			if (state->bass_stream)
			{
				BASS_ChannelStop(state->bass_stream);
				BASS_StreamFree(state->bass_stream);
				state->bass_stream = 0;
			}
			int count = 0;
			while (!SelectNextSong(state))
			{
				// Keep trying until we find a playable song
				count++;
				if (count >= (int) state->playlist.size())
				{
					// Could not find a valid song in the playlist.  Clean up.
					ClearInfoLabels(&state->controls, state->main_hwnd);
					ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
					state->player_state = STOPPED;
					return;
				}
			}
			if (state->player_state == PLAYING || state->player_state == PAUSED)
			{
				UpdateInfoLabels(state, true);
				ResetPositionTrackbar(state->controls.tb_pos, 0, state->curr_song->song_length_secs, 0);
				BASS_ChannelPlay(state->bass_stream, false);
				state->player_state = PLAYING;
			}
			else
			{	// player_state = STOPPED
				UpdateInfoLabels(state, false);
				ResetPositionTrackbar(state->controls.tb_pos, 0, 0, 0);
			}

		}
	}
}


static void ShuffleBtnHandler(AppState* state, bool toggle)
{
	if (toggle)
		state->options.shuffle = !state->options.shuffle;
	
	if (state->options.shuffle)
	{
		SendMessage(state->controls.btn_shuffle, WP_BM_SETIMAGE, IDB_SHUFFLE_ON, 0);
		if (state->playlist.size() > 0)
		{
			std::random_shuffle(state->playlist.begin(), state->playlist.end());
			const int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
			if (curr_pl_idx > 0)
				// Swap so that the current song is first in the playlist
				std::swap(state->playlist[curr_pl_idx], state->playlist[0]);
		}
	}
	else
	{
		SendMessage(state->controls.btn_shuffle, WP_BM_SETIMAGE, IDB_SHUFFLE_OFF, 0);
		if (state->playlist.size() > 0)
			state->playlist = state->playlist_view;
	}
}


static void RepeatBtnHandler(AppState* state, bool toggle)
{
	if (toggle)
		state->options.repeat = !state->options.repeat;

	if (state->options.repeat)
		SendMessage(state->controls.btn_repeat, WP_BM_SETIMAGE, IDB_REPEAT_ON, 0);
	else
		SendMessage(state->controls.btn_repeat, WP_BM_SETIMAGE, IDB_REPEAT_OFF, 0);
}


static void MoveUpBtnHandler(AppState* state)
{
	const int sel_idx = SendMessage(state->controls.playlist_hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (sel_idx >= 1)
	{
		std::swap(state->playlist_view[sel_idx], state->playlist_view[sel_idx - 1]);
		UpdatePlaylistWindow(state->controls.playlist_hwnd, state->playlist_view, state->controls.lbl_pl_info);
		ListView_SetItemState(state->controls.playlist_hwnd, sel_idx - 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
	}
	if (!state->options.shuffle)
	{
		// If shuffle is turned on, moving an item up/down doesn't really do anything
		state->playlist = state->playlist_view;
	}
}

static void MoveDownBtnHandler(AppState* state)
{
	const int sel_idx = SendMessage(state->controls.playlist_hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
	if (sel_idx >= 0 && sel_idx < (int)state->playlist_view.size() - 1)
	{
		std::swap(state->playlist_view[sel_idx], state->playlist_view[sel_idx + 1]);
		UpdatePlaylistWindow(state->controls.playlist_hwnd, state->playlist_view, state->controls.lbl_pl_info);
		ListView_SetItemState(state->controls.playlist_hwnd, sel_idx + 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);

	}
	if (!state->options.shuffle)
	{
		// If shuffle is turned on, moving an item up/down doesn't really do anything
		state->playlist = state->playlist_view;
	}
}

static void SettingsBtnHandler(AppState* state)
{
	HMENU menu = CreatePopupMenu();
	HMENU pl_size_submenu = CreatePopupMenu();
	POINT menu_pos = { 0, 30 };
	ClientToScreen(state->main_hwnd, &menu_pos);
	AppendMenu(menu, MF_STRING, IDM_ALWAYS_ON_TOP, "Always On Top");
	if (state->options.always_on_top)
		CheckMenuItem(menu, IDM_ALWAYS_ON_TOP, MF_CHECKED);

	AppendMenu(menu, MF_STRING, IDM_SNAP_TO_EDGES, "Snap to Screen Edges");
	if (state->options.snap_to_edges)
		CheckMenuItem(menu, IDM_SNAP_TO_EDGES, MF_CHECKED);

	AppendMenu(menu, MF_STRING | MF_POPUP, (UINT_PTR)pl_size_submenu, "Playlist Size");
	AppendMenu(pl_size_submenu, MF_STRING, IDM_PLAYLIST_SMALL, "Small (250 pixels)");
	AppendMenu(pl_size_submenu, MF_STRING, IDM_PLAYLIST_MEDIUM, "Medium (500 pixels)");
	AppendMenu(pl_size_submenu, MF_STRING, IDM_PLAYLIST_LARGE, "Large (750 pixels)");

	if (state->options.playlist_size == SMALL)
		CheckMenuRadioItem(pl_size_submenu, IDM_PLAYLIST_SMALL, IDM_PLAYLIST_LARGE, IDM_PLAYLIST_SMALL, MF_BYCOMMAND);
	else if (state->options.playlist_size == MEDIUM)
		CheckMenuRadioItem(pl_size_submenu, IDM_PLAYLIST_SMALL, IDM_PLAYLIST_LARGE, IDM_PLAYLIST_MEDIUM, MF_BYCOMMAND);
	else if (state->options.playlist_size == LARGE)
		CheckMenuRadioItem(pl_size_submenu, IDM_PLAYLIST_SMALL, IDM_PLAYLIST_LARGE, IDM_PLAYLIST_LARGE, MF_BYCOMMAND);

	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_STRING, IDM_ABOUT, "About...");
	AppendMenu(menu, MF_SEPARATOR, 0, 0);
	AppendMenu(menu, MF_STRING, IDM_EXIT, "Exit");
	TrackPopupMenu(menu, TPM_LEFTBUTTON, menu_pos.x, menu_pos.y, 0, state->main_hwnd, 0);
	DestroyMenu(menu);
}

static void ResetPositionTrackbar(HWND tb_pos, int min, int max, int pos)
{
	SendMessage(tb_pos, WP_TBM_SETMIN, 0, min);
	SendMessage(tb_pos, WP_TBM_SETPOS, 0, pos);
	SendMessage(tb_pos, WP_TBM_SETMAX, 0, max);
}


static void ClearInfoLabels(ControlHandles* controls, HWND main_hwnd)
{
	SendMessage(controls->lbl_album_art, WP_LM_CLEARIMAGE, 0, 0);
	SendMessage(controls->lbl_title, WM_SETTEXT, 0, 0);
	SendMessage(controls->lbl_artist, WM_SETTEXT, 0, 0);
	SendMessage(controls->lbl_album, WM_SETTEXT, 0, 0);
	SendMessage(controls->lbl_file_info, WM_SETTEXT, 0, 0);
	SendMessage(controls->lbl_time_pos, WM_SETTEXT, 0, 0);
	SendMessage(controls->lbl_time_length, WM_SETTEXT, 0, 0);
	SetWindowText(main_hwnd, "Winphonic");
}

static void ResizePlaylist(int playlist_size, HWND main_hwnd, ControlHandles* controls, 
	bool* is_playlist_visible, bool always_on_top)
{
	if (!*is_playlist_visible)
		TogglePlaylistVisible(main_hwnd, is_playlist_visible, true, playlist_size, controls->btn_playlist, always_on_top);
	
	const int win_height_pl = WIN_HEIGHT + playlist_size;
	SetWindowPos(controls->playlist_hwnd, 0, 0, 0, WIN_WIDTH, playlist_size - 30, SWP_NOMOVE);
	SetWindowPos(main_hwnd, 0, 0, 0, WIN_WIDTH, win_height_pl, SWP_NOMOVE);
	SetWindowPos(controls->btn_add, 0, 0, win_height_pl - 25, 0, 0, SWP_NOSIZE);
	SetWindowPos(controls->btn_delete, 0, 25, win_height_pl - 25, 0, 0, SWP_NOSIZE);
	SetWindowPos(controls->btn_move_up, 0, 65, win_height_pl - 25, 0, 0, SWP_NOSIZE);
	SetWindowPos(controls->btn_move_down, 0, 85, win_height_pl - 25, 0, 0, SWP_NOSIZE);
	SetWindowPos(controls->lbl_pl_info, 0, 265, win_height_pl - 25, 0, 0, SWP_NOSIZE);
}


// Free heap memory associated with the Song object
static void FreeSong(Song* song)
{
	if (song == NULL)
		return;
	
	FreeMemory(song->metadata.title);
	FreeMemory(song->metadata.artist);
	FreeMemory(song->metadata.album);
	FreeMemory(song->metadata.genre);
	FreeMemory(song->metadata.track_num);
	FreeMemory(song->metadata.date);
	FreeMemory(song->metadata.comment_description);
	if (song->metadata.album_art)
	{
		FreeMemory(song->metadata.album_art->data);
		FreeMemory(song->metadata.album_art);
	}
	if (song->playlist_song_name != NULL && song->playlist_song_name != song->file_name)
	{
		// If playlist_song_name == file_name when there is no metadata for the file
		// Must check to avoid a double free bug
		FreeMemory(song->playlist_song_name);
	}	
	FreeMemory(song->file_name);
	FreeMemory(song->path);
	FreeMemory(song);
}

// Delete a song from the playlist vectors and from the playlist window
static void DeleteSongFromPlaylist(AppState* state, int pl_view_idx_to_del)
{
	const int curr_pl_view_idx = GetPlaylistCurrentIndex(state->playlist_view);

	// If sel_idx == curr_pl_view_idx, then user just deleted the current song
	if (pl_view_idx_to_del == curr_pl_view_idx)
	{
		state->curr_song = NULL;
	}
	Song* song_to_del = state->playlist_view[pl_view_idx_to_del];
	FreeSong(song_to_del);		// Clean up the song before deleting
	state->playlist_view.erase(state->playlist_view.begin() + pl_view_idx_to_del);
	int pl_idx_to_del = -1;
	for (pl_idx_to_del = 0; pl_idx_to_del < (int) state->playlist.size(); pl_idx_to_del++)
	{
		if (state->playlist[pl_idx_to_del] == song_to_del)
			state->playlist.erase(state->playlist.begin() + pl_idx_to_del);
	}
	UpdatePlaylistWindow(state->controls.playlist_hwnd, state->playlist_view, state->controls.lbl_pl_info);
}


// Update the text labels on main window with song title, artist, album, and album art
static void UpdateInfoLabels(AppState* state, bool display_song_len)
{
	// Title
	if (state->curr_song->metadata.title)
		SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.title);
	else
		// No title found in ID3v2 tag.  Use the file name as the title.
		SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->file_name);
	
	// Artist
	if (state->curr_song->metadata.artist)
		SendMessage(state->controls.lbl_artist, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.artist);
	else
		SendMessage(state->controls.lbl_artist, WM_SETTEXT, 0, 0);		// Blank

	// Album
	if (state->curr_song->metadata.album)
		SendMessage(state->controls.lbl_album, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.album);
	else
		SendMessage(state->controls.lbl_album, WM_SETTEXT, 0, 0);		// Blank
		
	// Album art
	if (state->curr_song->metadata.album_art)
		SendMessage(state->controls.lbl_album_art, WP_LM_SETIMAGE_FROM_BUFFER, (WPARAM)state->curr_song->metadata.album_art->data, (LPARAM)state->curr_song->metadata.album_art->size);
	else
		SendMessage(state->controls.lbl_album_art, WP_LM_CLEARIMAGE, 0, 0);

	// Create the file info text
	char file_info[48] = {};
	if (state->curr_song->is_stereo)
	{
		if (state->curr_song->format == MP3)
			StringCbPrintfA(file_info, 48, "MP3, %u kbps, %u kHz, Stereo", state->curr_song->bitrate, 
				state->curr_song->frequency);
		else if (state->curr_song->format == OGG)
			StringCbPrintfA(file_info, 48, "OGG, %u kbps, %u kHz, Stereo", state->curr_song->bitrate, 
				state->curr_song->frequency);
	}
	else
	{
		if (state->curr_song->format == MP3)
			StringCbPrintfA(file_info, 48, "MP3, %u kbps, %u kHz, Mono", state->curr_song->bitrate, 
				state->curr_song->frequency);
		else if (state->curr_song->format == OGG)
			StringCbPrintfA(file_info, 48, "OGG, %u kbps, %u kHz, Mono", state->curr_song->bitrate, 
				state->curr_song->frequency);
	}
	SendMessage(state->controls.lbl_file_info, WM_SETTEXT, 0, (LPARAM)&file_info);

	if (display_song_len)
		SendMessage(state->controls.lbl_time_length, WM_SETTEXT, 0, (LPARAM)state->curr_song->song_length_str);

	// Set the window caption to the current song (shown in Windows task bar)
	SetWindowText(state->main_hwnd, state->curr_song->playlist_song_name);
}

// Shows/hides the playlist portion of the main window
static void TogglePlaylistVisible(HWND hwnd, bool* is_playlist_visible, bool toggle, int playlist_size, HWND btn_playlist, bool always_on_top)
{
	if (toggle)
		*is_playlist_visible = !*is_playlist_visible;
	
	RECT rc;
	GetClientRect(hwnd, &rc);
	MapWindowPoints(hwnd, GetParent(hwnd), (LPPOINT)&rc, 2);
	const int win_height_pl = WIN_HEIGHT + playlist_size;

	if (*is_playlist_visible)
	{	// Show playlist
		if (always_on_top)
			SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left, win_height_pl, 0);
		else
			SetWindowPos(hwnd, HWND_NOTOPMOST, rc.left, rc.top, rc.right - rc.left, win_height_pl, 0);
		SendMessage(btn_playlist, WP_BM_SETCOLOR_NORMAL, (WPARAM)PLAYLIST_AREA_COLOR, 0);
	}
	else
	{	// Hide playlist
		if (always_on_top)
			SetWindowPos(hwnd, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left, WIN_HEIGHT, 0);
		else
			SetWindowPos(hwnd, HWND_NOTOPMOST, rc.left, rc.top, rc.right - rc.left, WIN_HEIGHT, 0);
		SendMessage(btn_playlist, WP_BM_SETCOLOR_NORMAL, (WPARAM)BTN_NORMAL_COLOR, 0);
	}
}

// Gets the song metadata, song length, bitrate, frequency, and stereo and stores them in the Song struct
static void GetSongInfo(Song* song)
{
	if (song->has_info)
		// This song already has ID3v2 and format info.  This happens when we add additional songs
		// to the playlist_view using the "Add" button.  No need to do any work.
		return;

	const HSTREAM temp_stream = BASS_StreamCreateFile(false, song->path, 0, 0, 0);	// Create temporary BASS stream
	if (temp_stream)
	{
		// Get song length
		song->song_length_bytes = BASS_ChannelGetLength(temp_stream, BASS_POS_BYTE);
		song->song_length_secs = (int)BASS_ChannelBytes2Seconds(temp_stream, song->song_length_bytes);
		StringCbPrintfA(song->song_length_str, 8, "%u:%02u", song->song_length_secs / 60, song->song_length_secs % 60);
					
		const char* ext = GetFilenameExt(song->file_name);	// Get the file extension to determine if it is MP3 or OGG

		// Get metadata (ID3v2 for MP3, comments for OGG)
		song->metadata = {};
		if (!lstrcmpi(ext, "mp3"))
		{
			song->format = MP3;
			// If it is an MP3, get ID3v2
			const char* id3v2_buffer = BASS_ChannelGetTags(temp_stream, BASS_TAG_ID3V2);
			if (id3v2_buffer)
			{
				ParseID3v2(id3v2_buffer, &song->metadata);
			}
		}
		else if (!lstrcmpi(ext, "ogg"))
		{
			song->format = OGG;
			const char* ogg_comments_buffer = BASS_ChannelGetTags(temp_stream, BASS_TAG_OGG);
			if (ogg_comments_buffer)
			{
				ParseOggComments(ogg_comments_buffer, &song->metadata);
			}
		}

		// Construct the playlist text in this format:  Artist - SongTitle
		// Sometimes artist metadata is ridiciculously long, so truncate artist after 30 chars.
		if (song->metadata.artist && song->metadata.title)
		{
			const int max_artist_len = 30;
			const int artist_len = lstrlen(song->metadata.artist);
			if (artist_len < max_artist_len)
			{
				const size_t buf_len = artist_len + lstrlen(song->metadata.title) + 4;
				song->playlist_song_name = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf_len);
				StringCbPrintfA(song->playlist_song_name, buf_len, "%s - %s", song->metadata.artist, song->metadata.title);
			}
			else
			{
				// Truncate the artist
				// Ex:
				//		Before: Miles Kane, Zach Dawes, Loren Shane Humphrey, Tyler Parkford - Cry On My Guitar
				//		After:  Miles Kane, Zach Dawes, Loren ... - Cry On My Guitar
				const size_t buf_len = max_artist_len + lstrlen(song->metadata.title) + 7;
				song->playlist_song_name = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf_len);
				StringCbPrintfA(song->playlist_song_name, buf_len, "%.30s... - %s", song->metadata.artist, song->metadata.title);
			}
		}
		else
		{
			// No metadata.  Use the file name as the playlist text
			song->playlist_song_name = song->file_name;
		}

		// Get bitrate.
		// Valid bitrates for MP3 = 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320.
		float bitrate = 0;
		BASS_ChannelGetAttribute(temp_stream, BASS_ATTRIB_BITRATE, &bitrate);
		song->bitrate = (int) round(bitrate);
		
		// Get frequency (e.g. 44.1 kHz)
		float freq = 0;
		BASS_ChannelGetAttribute(temp_stream, BASS_ATTRIB_FREQ, &freq);
		song->frequency = (int) freq / 1000;	// Divide by 1000 to convert Hz to kHz (e.g. 44100 -> 44.1)

		// Get mono/stereo information
		BASS_CHANNELINFO channel_info;
		BASS_ChannelGetInfo(temp_stream, &channel_info);
		song->is_stereo = (channel_info.chans > 1) ? true : false;
		
		BASS_StreamFree(temp_stream);
		song->has_info = true;
		song->is_valid = true;
	}
	else
	{
		// Could not open the song.  Invalid file?
		song->playlist_song_name = song->file_name;
		song->has_info = false;
		song->is_valid = false;
	}
}


// Force playlist listview to repaint so that the current song is painted in a different color
static void RedrawPlaylistWindow(HWND playlist_hwnd, unsigned int num_items)
{
	ListView_RedrawItems(playlist_hwnd, 0, num_items);
	UpdateWindow(playlist_hwnd);		// Windows API function
}


// Adds all the items in the playlist_view vector to playlist_hwnd
static void UpdatePlaylistWindow(HWND playlist_hwnd, std::vector<Song*>& playlist_view, HWND lbl_pl_info)
{
	ListView_DeleteAllItems(playlist_hwnd);
	SendMessage(playlist_hwnd, LVM_SETITEMCOUNT, playlist_view.size(), 0);
	unsigned int total_pl_length = 0;
	for (unsigned int i = 0; i < playlist_view.size(); i++)
	{
		LVITEM item = {};
		item.mask = LVIF_TEXT;
		item.cchTextMax = MAX_PATH;

		// Row
		item.iItem = i;			
		ListView_InsertItem(playlist_hwnd, &item);

		// Song name
		item.iSubItem = 0;
		item.pszText = playlist_view[i]->playlist_song_name;
		ListView_SetItem(playlist_hwnd, &item);

		// Length
		item.iSubItem = 1;
		item.pszText = playlist_view[i]->song_length_str;
		ListView_SetItem(playlist_hwnd, &item);
		total_pl_length += playlist_view[i]->song_length_secs;
	}

	// General playlist info.  Displayed bottom right of playlist window.
	char pl_info[48];
	StringCbPrintfA(pl_info, 48, "%u Songs, Time: %u:%02u", playlist_view.size(), total_pl_length / 60, total_pl_length % 60);
	SendMessage(lbl_pl_info, WM_SETTEXT, 0, (LPARAM)pl_info);
}


static void GetPlaylistFromFileList(std::vector<Song*>& playlist_view, char* file_list, size_t file_list_len)
{
	if (!lstrlen(file_list))
		return;
		
	int last_sep_pos = -1;
	int last_backslash_pos = 0;
	HANDLE heap = GetProcessHeap();
	// Read through each byte until we find a separator or reach the end
	for (unsigned int curr_pos = 0; curr_pos < file_list_len; curr_pos++)
	{
		if (file_list[curr_pos] == '\\')
		{
			last_backslash_pos = curr_pos;
		}
		else if (file_list[curr_pos] == '|')
		{
			if (curr_pos - last_sep_pos <= 8 || curr_pos - last_sep_pos > MAX_PATH)
			{
				// This can't be a valid file
				last_sep_pos = curr_pos;
				continue;
			}
			
			Song* song = (Song*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Song));
			size_t path_len = curr_pos - last_sep_pos - 1;
			song->path = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, path_len + 1);
			size_t file_name_len = curr_pos - last_backslash_pos - 1;
			song->file_name = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_name_len + 1);
			memcpy(song->path, file_list + last_sep_pos + 1, path_len);
			memcpy(song->file_name, file_list + last_backslash_pos + 1, file_name_len);
			playlist_view.push_back(song);
			last_sep_pos = curr_pos;
		}
		else if (curr_pos == file_list_len - 1)
		{
			if (curr_pos - last_sep_pos <= 8 || curr_pos - last_sep_pos > MAX_PATH)
			{
				// This can't be a valid file
				last_sep_pos = curr_pos;
				break;
			}
			Song* song = (Song*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Song));
			size_t path_len = curr_pos - last_sep_pos;
			song->path = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, path_len + 1);
			size_t file_name_len = curr_pos - last_backslash_pos;
			song->file_name = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_name_len + 1);
			memcpy(song->path, file_list + last_sep_pos + 1, path_len);
			memcpy(song->file_name, file_list + last_backslash_pos + 1, file_name_len);
			playlist_view.push_back(song);
		}
	}
}

// Parses a buffer of file names from the Windows "Open File" dialog.  file_offset specifies where the 
// name of the directory ends and the filenames begin.  Returns a std::vector containing the songs.
static void GetPlaylistFromFileBuffer(std::vector<Song*>& playlist_view, char* file_buffer, const int file_buffer_size,
	const int file_offset, char* file_title)
{
	if (!file_buffer || !file_title)
		return;

	HANDLE heap = GetProcessHeap();
	char* dir = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_offset);
	if (!dir)
		return;

	memcpy(dir, file_buffer, file_offset - 1);
	size_t dir_len = lstrlen(dir);

	// For multiple selection, the file buffer will contain null characters as separators
	// See:  https://stackoverflow.com/a/41371253
	if (file_buffer[file_offset - 1] == '\0')
	{
		// Multiple files were selected.  Format if 3 files were selected:
		// <Directory>\0<File1>\0<File2>\0<File3>\0\0
		// Directory is only listed once, since all songs must be in same directory.
		int last_null_pos = file_offset - 1;
		for (int curr_byte_pos = file_offset; curr_byte_pos < file_buffer_size; curr_byte_pos++)
		{
			if (file_buffer[curr_byte_pos] == '\0')
			{
				if (file_buffer[curr_byte_pos - 1] == '\0')
				{
					// Two null bytes in a row means we have reached the end of file names
					break;
				}

				// Allocate memory for new Song struct
				Song* song = (Song*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Song));
				size_t file_name_len = curr_byte_pos - last_null_pos;
				size_t path_len = dir_len + file_name_len + 1;		// 1 extra byte for added backslash
				song->path = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, path_len + 1);
				song->file_name = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_name_len + 1);

				// Copy file_name to struct
				memcpy(song->file_name, file_buffer + last_null_pos + 1, file_name_len);

				// Copy path to struct by concatenating the directory and file_name
				memcpy(song->path, dir, dir_len);		// Copy directory portion of path
				if (dir[dir_len] != '\\')
				{
					// Directory does NOT already end in a backslash
					song->path[dir_len] = '\\';
					memcpy(song->path + dir_len + 1, song->file_name, file_name_len);
				}
				else
				{
					// Directory already ends in backslash
					memcpy(song->path + dir_len, song->file_name, file_name_len);
				}
				playlist_view.push_back(song);		// Add to end of playlist
				last_null_pos = curr_byte_pos;
			}

		}
	}
	else
	{
		// Single file selected.  File buffer format:  C:\Music\Led Zeppelin - Stairway to Heaven.mp3\0
		Song* song = (Song*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(Song));
		song->path = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_buffer_size + 1);	// Already null terminated?
		if (song->path)
			memcpy(song->path, file_buffer, file_buffer_size);
		const size_t file_name_len = lstrlen(file_title);
		song->file_name = (char*)HeapAlloc(heap, HEAP_ZERO_MEMORY, file_name_len + 1);
		if (song->file_name)
			memcpy(song->file_name, file_title, file_name_len);
		playlist_view.push_back(song);
	}
}


// Displays the Windows "Open File" dialog, gets the user input, and processes it
static void OpenFile(AppState* state, bool is_add_btn)
{
	char file_name[MAX_PATH] = {};
	const int file_buffer_size = 4096;
	char file_buffer[file_buffer_size] = {};	// Must use huge buffer, in case user selects many files
	
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = state->main_hwnd;
	ofn.lpstrFile = file_buffer;
	ofn.nMaxFile = file_buffer_size;
	ofn.lpstrFilter = "Audio Files\0*.mp3;*.ogg\0MP3\0*.mp3\0OGG\0*.ogg\0";
	ofn.lpstrFileTitle = file_name;
	ofn.nMaxFileTitle = MAX_PATH;
	if (is_add_btn)
		ofn.lpstrTitle = "Add File(s) to Playlist";
	else
		ofn.lpstrTitle = "Open File(s)";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY;
	if (!GetOpenFileName(&ofn))
	{
		// User clicked "Cancel" or there was an error
		return;
	}

	if (!is_add_btn && !state->playlist_view.empty())
	{
		// User clicked the "open" button, NOT the "add" button.  Must clear all previous items in playlist.
		for (unsigned int i = 0; i < state->playlist_view.size(); i++)
		{
			Song* song = state->playlist_view[i];
			FreeSong(song);
		}
		// Erase all elements
		state->playlist_view.erase(state->playlist_view.begin(), state->playlist_view.end());
		state->playlist.erase(state->playlist.begin(), state->playlist.end());
	}
	
	GetPlaylistFromFileBuffer(state->playlist_view, file_buffer, file_buffer_size, ofn.nFileOffset, ofn.lpstrFileTitle);
	for (unsigned int i = 0; i < state->playlist_view.size(); i++)
	{
		GetSongInfo(state->playlist_view[i]);
	}
	UpdatePlaylistWindow(state->controls.playlist_hwnd, state->playlist_view, state->controls.lbl_pl_info);

	// Copy vector
	state->playlist = state->playlist_view;

	if (state->options.shuffle)
	{
		std::random_shuffle(state->playlist.begin(), state->playlist.end());
		int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
		if (curr_pl_idx > 0)
		{
			// Swap so that the current song is first in the playlist
			std::swap(state->playlist[curr_pl_idx], state->playlist[0]);
		}
	}

	if (!is_add_btn)
	{
		state->curr_song = state->playlist[0];
		state->curr_song->is_current = true;

		RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist_view.size());
		ResetPositionTrackbar(state->controls.tb_pos, 0, state->curr_song->song_length_secs, 0);
		UpdateInfoLabels(state, true);

		// If user added multiple files, automatically show the playlist
		if (state->playlist_view.size() > 1 && !state->is_playlist_visible)
			TogglePlaylistVisible(state->main_hwnd, &state->is_playlist_visible, true, state->options.playlist_size, state->controls.btn_playlist, state->options.always_on_top);

		if (LoadCurrentSong(state))
		{
			// Play the song
			if (BASS_ChannelPlay(state->bass_stream, false))
			{
				state->player_state = PLAYING;
			}
		}
	}
}


// Create all of the GUI buttons
static void CreateButtons(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance, int playlist_size)
{
	// Buttons in titlebar area
	controls->btn_min = CreateImageButton(IDB_MIN, TITLEBAR_COLOR, BTN_HOVER_COLOR, RGB(0, 0, 0),
		main_hwnd, (HMENU)BTN_MIN, instance, WIN_WIDTH - 60, 0, 30, 30);
	controls->btn_close = CreateImageButton(IDB_CLOSE, TITLEBAR_COLOR, BTN_HOVER_COLOR, RGB(0, 0, 0),
		main_hwnd, (HMENU)BTN_CLOSE, instance, WIN_WIDTH - 30, 0, 30, 30);
	controls->btn_settings = CreateImageButton(IDB_LOGO_SMALL, TITLEBAR_COLOR, BTN_HOVER_COLOR, 
		RGB(0, 0, 0), main_hwnd, (HMENU)BTN_SETTINGS, instance, 0, 0, 30, 30);

	// Controller buttons at bottom of main window
	controls->btn_prev = CreateImageButton(IDB_PREVIOUS, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_PREV, instance, 0, 140, 40, 40);
	controls->btn_play = CreateImageButton(IDB_PLAY, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_PLAY, instance, 40, 140, 40, 40);
	controls->btn_pause = CreateImageButton(IDB_PAUSE, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_PAUSE, instance, 80, 140, 40, 40);
	controls->btn_stop = CreateImageButton(IDB_STOP, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_STOP, instance, 120, 140, 40, 40);
	controls->btn_next = CreateImageButton(IDB_NEXT, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_NEXT, instance, 160, 140, 40, 40);
	controls->btn_open = CreateImageButton(IDB_OPEN, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_OPEN, instance, 230, 140, 40, 40);
	controls->btn_shuffle = CreateImageButton(IDB_SHUFFLE_OFF, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_SHUFFLE, instance, 290, 150, 30, 30);
	controls->btn_repeat = CreateImageButton(IDB_REPEAT_OFF, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_REPEAT, instance, 320, 150, 30, 30);
	controls->btn_playlist = CreateImageButton(IDB_PLAYLIST, BTN_NORMAL_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_PLAYLIST, instance, 380, 140, 40, 40);
	
	
	// Buttons in playlist area
	const int win_height_pl = WIN_HEIGHT + playlist_size;
	controls->btn_add = CreateImageButton(IDB_ADD, PLAYLIST_AREA_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_ADD, instance, 0, win_height_pl - 25, 25, 25);
	controls->btn_delete = CreateImageButton(IDB_DELETE, PLAYLIST_AREA_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_DELETE, instance, 25, win_height_pl - 25, 25, 25);
	controls->btn_move_up = CreateImageButton(IDB_MOVE_UP, PLAYLIST_AREA_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_MOVE_UP, instance, 60, win_height_pl - 25, 25, 25);
	controls->btn_move_down = CreateImageButton(IDB_MOVE_DOWN, PLAYLIST_AREA_COLOR, BTN_HOVER_COLOR, BTN_PRESSED_COLOR,
		main_hwnd, (HMENU)BTN_MOVE_DOWN, instance, 85, win_height_pl - 25, 25, 25);
}

// Create the song position and the volume trackbars
static void CreateTrackbars(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance)
{
	controls->tb_pos = CreateTrackbar(TB_HORIZONTAL, BACKGROUND_COLOR, TITLEBAR_COLOR, TEXT_COLOR, IDB_CIRCLE, 
		0, 0, 0, main_hwnd, instance, 45, 123, WIN_WIDTH - 90, 12);
	controls->tb_vol = CreateTrackbar(TB_VERTICAL, BACKGROUND_COLOR, TITLEBAR_COLOR, TEXT_COLOR, IDB_CIRCLE,
		0, 100, 20, main_hwnd, instance, WIN_WIDTH - 20, 38, 12, 80);
}


// Create all of the GUI text labels
static void CreateTextLabels(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance, int playlist_size)
{
	const char font_name[] = "Segoe UI";
	
	controls->lbl_title = CreateTextLabel(NULL, font_name, 14, true, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_LEFT, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_TITLE, instance, 95, 30, 285, 25);
	controls->lbl_artist = CreateTextLabel(NULL, font_name, 12, false, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_LEFT, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_ARTIST, instance, 95, 55, 285, 25);
	controls->lbl_album = CreateTextLabel(NULL, font_name, 10, false, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_LEFT, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_ALBUM, instance, 95, 80, 285, 23);
	controls->lbl_file_info = CreateTextLabel(NULL, font_name, 8, false, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_LEFT, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_FILE_INFO, instance, 95, 103, 285, 15);
	controls->lbl_time_pos = CreateTextLabel(NULL, font_name, 10, false, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_CENTER, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_TIME_POS, instance, 1, 118, 40, 20);
	controls->lbl_time_length = CreateTextLabel(NULL, font_name, 10, false, BACKGROUND_COLOR, TEXT_COLOR, LBL_H_CENTER, LBL_V_TOP,
		main_hwnd, (HMENU)LBL_TIME_LEN, instance, WIN_WIDTH - 45, 118, 45, 20);
	controls->lbl_pl_info = CreateTextLabel(NULL, font_name, 9, false, PLAYLIST_AREA_COLOR, TEXT_COLOR, LBL_H_RIGHT, LBL_V_CENTER,
		main_hwnd, (HMENU)LBL_PL_INFO, instance, 265, WIN_HEIGHT + playlist_size - 25, 150, 25);
}


static void CreateImgLabels(ControlHandles* controls, HWND main_hwnd, HINSTANCE instance)
{
	controls->lbl_album_art = CreateImageLabel(0, false, 30, ALBUM_ART_COLOR, main_hwnd, 
		(HMENU)LBL_ALBUM_ART, instance, 5, 35, 80, 80);
}

static HWND CreatePlaylistWindow(HWND main_hwnd, HINSTANCE instance, HFONT pl_font, int playlist_size)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	HWND playlist_hwnd = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL |  
		LVS_REPORT | LVS_NOCOLUMNHEADER, 0, WIN_HEIGHT + 5, WIN_WIDTH, playlist_size - 30,
		main_hwnd, (HMENU)1, instance, 0);

	// Set listview styles and font
	SendMessage(playlist_hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_FLATSB,
		LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);
	ListView_SetBkColor(playlist_hwnd, PLAYLIST_COLOR);
	SendMessage(playlist_hwnd, WM_SETFONT, (WPARAM)pl_font, 0);
	
	// Song name
	LVCOLUMN song_name_col = {};
	song_name_col.mask = LVCF_WIDTH | LVCF_SUBITEM;
	song_name_col.iSubItem = 0;
	song_name_col.cx = 330;
	ListView_InsertColumn(playlist_hwnd, 0, &song_name_col);

	// Song length
	LVCOLUMN song_length_col = {};
	song_length_col.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
	song_length_col.iSubItem = 1;
	song_length_col.cx = 50;
	song_length_col.fmt = LVCFMT_RIGHT;
	ListView_InsertColumn(playlist_hwnd, 1, &song_length_col);

	return playlist_hwnd;
}

static void CreateGDIObjects(AppState* state)
{
	state->gdi.main_bg_brush = CreateSolidBrush(BACKGROUND_COLOR);
	state->gdi.playlist_bg_brush = CreateSolidBrush(PLAYLIST_AREA_COLOR);
	state->gdi.titlebar_brush = CreateSolidBrush(TITLEBAR_COLOR);

	state->gdi.titlebar_font = GetFont("Segoe UI", 11, false, false, false);
	state->gdi.playlist_font_normal = GetFont("Segoe UI", 9, false, false, false);
	state->gdi.playlist_font_current = GetFont("Segoe UI", 9, true, false, false);
	state->gdi.playlist_font_invalid = GetFont("Segoe UI", 9, false, true, false);
}


static bool LoadCurrentSong(AppState* state)
{
	if (state->bass_stream) {
		BASS_StreamFree(state->bass_stream);	// Free the previous stream, if necessary
		state->bass_stream = 0;
	}

	if (state->curr_song == NULL)
	{
		return false;
	}
	state->bass_stream = BASS_StreamCreateFile(false, state->curr_song->path, 0, 0, 0);

	if (state->bass_stream)
	{
		state->curr_song->song_length_bytes = BASS_ChannelGetLength(state->bass_stream, BASS_POS_BYTE);
		state->curr_song->song_length_secs = (int)BASS_ChannelBytes2Seconds(state->bass_stream, state->curr_song->song_length_bytes);
		ResetPositionTrackbar(state->controls.tb_pos, 0, state->curr_song->song_length_secs, 0);
		BASS_ChannelSetAttribute(state->bass_stream, BASS_ATTRIB_VOL, state->volume / 100.0f);
		state->curr_song->is_valid = true;
		return true;
	}
	else
	{
		state->curr_song->is_valid = false;
		RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist_view.size());
	}

	return false;
}

static int GetPlaylistCurrentIndex(std::vector<Song*> &playlist)
{
	for (unsigned int i = 0; i < playlist.size(); i++)
	{
		if (playlist[i]->is_current)
			return (int) i;
	}
	return -1;
}

static int GetPrevSongIndex(unsigned int curr_idx, unsigned int pl_size, bool repeat)
{
	int result;
	if (curr_idx > 0)
		result = curr_idx - 1;
	else if (repeat)
		result = pl_size - 1;	// Wrap around and jump to end of playlist
	else
		result = -1;			// Error

	return result;
}

static bool SelectPrevSong(AppState* state)
{
	int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);
	//int curr_pl_view_idx = GetPlaylistCurrentIndex(state->playlist_view);

	int prev_idx = -1;
	if (state->curr_song == NULL || curr_pl_idx == -1)
	{
		// User deleted the current song.  Go back to beginning of playlist.
		prev_idx = 0;
	}
	else
	{
		prev_idx = GetPrevSongIndex(curr_pl_idx, state->playlist.size(), state->options.repeat);
	}

	if (prev_idx >= 0)
	{
		if (state->curr_song != NULL)
			state->curr_song->is_current = false;
		state->curr_song = state->playlist[prev_idx];
		state->curr_song->is_current = true;

		// Find this item in the view playlist and scroll to ensure it is visible
		int curr_pl_view_idx = GetPlaylistCurrentIndex(state->playlist_view);
		ListView_EnsureVisible(state->controls.playlist_hwnd, curr_pl_view_idx, FALSE);
		
		if (LoadCurrentSong(state))
		{
			RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist.size());
			return true;
		}
	}
	return false;		// Unable to select the previous song

}


// Returns -1 if there is not a valid song to play next.  This will happen on the last song of the
// playlist when repeat is NOT turned on.
static int GetNextSongIndex(unsigned int curr_idx, unsigned int pl_size, bool repeat)
{
	int result;
	if (curr_idx < pl_size - 1)
		result = curr_idx + 1;
	else if (repeat)
		result = 0;		// Wrap around and jump to first song in playlist
	else
		result = -1;	// Error

	return result;
}

static bool SelectNextSong(AppState* state)
{
	int curr_pl_idx = GetPlaylistCurrentIndex(state->playlist);

	int next_idx = -1;
	if (state->curr_song == NULL || curr_pl_idx == -1)
	{
		// User deleted the current song.  Go back to beginning of playlist.
		next_idx = 0;
	}
	else
	{
		next_idx = GetNextSongIndex(curr_pl_idx, state->playlist.size(), state->options.repeat);
	}

	if (next_idx >= 0)
	{
		if (state->curr_song != NULL)
			state->curr_song->is_current = false;
		
		state->curr_song = state->playlist[next_idx];
		state->curr_song->is_current = true;

		// Find this item in the view playlist and scroll to ensure it is visible
		int curr_pl_view_idx = GetPlaylistCurrentIndex(state->playlist_view);
		ListView_EnsureVisible(state->controls.playlist_hwnd, curr_pl_view_idx, FALSE);

		if (LoadCurrentSong(state))
		{
			RedrawPlaylistWindow(state->controls.playlist_hwnd, state->playlist.size());
			return true;
		}
	}
	return false;		// Unable to select the previous song
}


// Initializes the main window.  Creates the GUI controls.
static void Initialize(AppState* state)
{

	CreateButtons(&state->controls, state->main_hwnd, state->instance, state->options.playlist_size);
	CreateTrackbars(&state->controls, state->main_hwnd, state->instance);
	CreateTextLabels(&state->controls, state->main_hwnd, state->instance, state->options.playlist_size);
	CreateImgLabels(&state->controls, state->main_hwnd, state->instance);
	state->controls.playlist_hwnd = CreatePlaylistWindow(state->main_hwnd,
		state->instance, state->gdi.playlist_font_normal, state->options.playlist_size);
	CreateGDIObjects(state);

	// Use SetTimer() to cause WM_TIMER message to fire every interval for updating 
	// song menu_pos and time elapsed
	SetTimer(state->main_hwnd, TIMER_UPDATE_SONG_POS, 100, NULL);

	// Initialize shuffle
	ShuffleBtnHandler(state, false);

	// Initialize repeat
	RepeatBtnHandler(state, false);

	// Initialize always on top
	if (state->options.always_on_top)
		// SetWindowPos doesn't work when called in WM_CREATE, so use this hack to set always on top
		PostMessage(state->main_hwnd, WM_USER + 100, 0, 0);

	// Initialize volume
	SendMessage(state->controls.tb_vol, WP_TBM_SETPOS, 0, (LPARAM)state->volume);

	// Initialize playlist visible
	TogglePlaylistVisible(state->main_hwnd, &state->is_playlist_visible, false, state->options.playlist_size,
		state->controls.btn_playlist, state->options.always_on_top);
	
	// TODO:  Initialize playlist


	InitCommonControls();
	
}



LRESULT CALLBACK MainProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	AppState* state = {};
	if (uMsg == WM_NCCREATE)
	{
		// The first time MainProc is called, we store the lParam (AppState) as GWLP_USERDATA.
		// This allows us to avoid using lots of global variables.
		// See:  https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx
		CREATESTRUCT* createstruct_ptr = reinterpret_cast<CREATESTRUCT*>(lParam);
		state = reinterpret_cast<AppState*>(createstruct_ptr->lpCreateParams);
		state->main_hwnd = hwnd;
		state->instance = GetModuleHandle(0);
		//state->instance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
	}
	else
	{
		// Retrieve AppState from GWLP_USERDATA
		state = reinterpret_cast<AppState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}
		
	switch (uMsg)
	{
		case WM_CREATE:
		{
			Initialize(state);
		} break;

		case WM_USER + 100:
		{
			// Hack so that SetWindowPos() works for setting always on top
			if (state->options.always_on_top)
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		} break;

		case WM_CLOSE:
		{
			state->is_running = false;
		} break;

		case WM_DESTROY:
		{
			state->is_running = false;
		} break;

		case WM_COMMAND:
		{
			UINT ctrl_id = LOWORD(wParam);

			switch (ctrl_id)
			{
				case BTN_CLOSE:
				{
					state->is_running = false;
				} break;

				case BTN_MIN:
				{
					ShowWindow(hwnd, SW_MINIMIZE);
				} break;

				case BTN_OPEN:
				{
					OpenFile(state, false);
					//RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				} break;

				case BTN_PLAY:
				{
					PlayBtnHandler(state);
				} break;

				case BTN_PAUSE:
				{
					PauseBtnHandler(state);
				} break;

				case BTN_STOP:
				{
					StopBtnHandler(state);
				} break;

				case BTN_PREV:
				{
					PrevBtnHandler(state);
				} break;

				case BTN_NEXT:
				{
					NextBtnHandler(state);
				} break;

				case BTN_SHUFFLE:
				{					
					ShuffleBtnHandler(state, true);
				} break;

				case BTN_REPEAT:
				{
					RepeatBtnHandler(state, true);
				} break;

				case BTN_PLAYLIST:
				{
					TogglePlaylistVisible(state->main_hwnd, &state->is_playlist_visible, true, state->options.playlist_size, state->controls.btn_playlist, state->options.always_on_top);
				} break;

				case BTN_ADD:
				{
					OpenFile(state, true);
				} break;

				case BTN_DELETE:
				{
					const int sel_idx = SendMessage(state->controls.playlist_hwnd, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
					if (sel_idx >= 0)
					{
						DeleteSongFromPlaylist(state, sel_idx);
					}
				} break;

				case BTN_MOVE_UP:
				{
					MoveUpBtnHandler(state);
				} break;

				case BTN_MOVE_DOWN:
				{
					MoveDownBtnHandler(state);
				} break;


				case BTN_SETTINGS:
				{
					SettingsBtnHandler(state);
				} break;

				case IDM_ALWAYS_ON_TOP:
				{
					state->options.always_on_top = !state->options.always_on_top;
					if (state->options.always_on_top)
						SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
					else
						SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				} break;

				case IDM_SNAP_TO_EDGES:
				{
					state->options.snap_to_edges = !state->options.snap_to_edges;
				} break;

				case IDM_PLAYLIST_SMALL:
				{
					state->options.playlist_size = SMALL;
					ResizePlaylist(state->options.playlist_size, hwnd, &state->controls, 
						&state->is_playlist_visible, state->options.always_on_top);
				} break;

				case IDM_PLAYLIST_MEDIUM:
				{
					state->options.playlist_size = MEDIUM;
					ResizePlaylist(state->options.playlist_size, hwnd, &state->controls, 
						&state->is_playlist_visible, state->options.always_on_top);
				} break;

				case IDM_PLAYLIST_LARGE:
				{
					state->options.playlist_size = LARGE;
					ResizePlaylist(state->options.playlist_size, hwnd, &state->controls, 
						&state->is_playlist_visible, state->options.always_on_top);
				} break;

				case IDM_ABOUT:
				{
					AboutDlgState* about_dlg_state = (AboutDlgState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AboutDlgState));
					about_dlg_state->bg_brush = state->gdi.main_bg_brush;
					about_dlg_state->bg_color = BACKGROUND_COLOR;
					about_dlg_state->text_color = TEXT_COLOR;
					about_dlg_state->btn_normal_color = BTN_NORMAL_COLOR;
					about_dlg_state->btn_hover_color = BTN_HOVER_COLOR;
					about_dlg_state->btn_pressed_color = BTN_PRESSED_COLOR;
					g_about_dlg_hwnd = CreateDialogParam(state->instance, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc, (LPARAM)about_dlg_state);
					ShowWindow(g_about_dlg_hwnd, SW_SHOW);
				} break;

				case IDM_EXIT:
				{
					state->is_running = false;
				} break;
			}

		} break;

		case WM_PAINT:
		{
			PaintWindow(state);
		} break;
		
		case WM_TIMER:
		{
			TimerHandler(state, (int)wParam);
		} break;

		case WP_TBM_POSCHANGED:
		{
			HWND sender = (HWND)wParam;
			if (sender == state->controls.tb_pos)
			{
				if (state->bass_stream)
				{
					// User changed the song menu_pos trackbar.  Seek to the specified location.
					int new_pos = (int)lParam;
					// Must cast to double to force floating point division
					double pos = (new_pos / (double)state->curr_song->song_length_secs) * 
						state->curr_song->song_length_bytes;
					if (!BASS_ChannelSetPosition(state->bass_stream, (QWORD)pos, BASS_POS_BYTE))
					{
						OutputDebugString("BASS Error while Seeking\n");
						break;
					}
					if (state->curr_song->metadata.title)
						SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.title);
					else
						SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->file_name);
					
				}
			}
			else if (sender == state->controls.tb_vol)
			{
				// User is done changing the volume, so put old text back
				if (state->curr_song != NULL)
				{
					if (state->curr_song->metadata.title)
						SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->metadata.title);
					else
						SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)state->curr_song->file_name);
				}
				else
				{
					SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, 0);
				}
			}

		} break;

		case WP_TBM_DRAGGING:
		{
			HWND sender = (HWND)wParam;
			if (sender == state->controls.tb_pos)
			{
				if (state->bass_stream)
				{
					// User is currently dragging the menu_pos trackbar.  Don't seek until user is
					// finished.  But, we can display the potential location where title normally is.
					// Example:  "Seek to:  999:99 / 999:99"
					int pos = (int)lParam;
					char seek_text[32];
					StringCbPrintfA(seek_text, 32, "Seek to:  %u:%02u / %u:%02u", pos / 60, pos % 60, state->curr_song->song_length_secs / 60, state->curr_song->song_length_secs % 60);
					SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)seek_text);
				}
			}
			else if (sender == state->controls.tb_vol)
			{
				state->volume = (int)lParam;
				char vol_text[16];
				StringCbPrintfA(vol_text, 16, "Volume:  %u%%", state->volume);
				SendMessage(state->controls.lbl_title, WM_SETTEXT, 0, (LPARAM)vol_text);
				if (state->bass_stream)
					BASS_ChannelSetAttribute(state->bass_stream, BASS_ATTRIB_VOL, state->volume / 100.0f);
			}
		} break;

		case WM_MOUSEWHEEL:
		{
			MouseWheelHandler(state, HIWORD(wParam));
		} break;

		case WM_LBUTTONDOWN:
		{
			// Hack to make the entire window draggable/movable
			// See https://stackoverflow.com/a/35522584 
			ReleaseCapture();
			SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);

		} break;

		case WM_ENTERSIZEMOVE:
		{
			if (state->options.snap_to_edges)
			{
				POINT curr_pos;
				RECT win_rect;
				GetWindowRect(hwnd, &win_rect);
				GetCursorPos(&curr_pos);
				state->width = curr_pos.x - win_rect.left;
				state->height = curr_pos.y - win_rect.top;
			}
			
		} break;

		case WM_MOVING:
		{
			if (state->options.snap_to_edges)
			{
				// Snap window to edges of screen
				// Reference:  https://www.borngeek.com/2008/04/12/snap-a-window-to-the-screen-edge/
				RECT* win_rect = (RECT*)lParam;
				POINT curr_pos;
				GetCursorPos(&curr_pos);
				OffsetRect(win_rect, curr_pos.x - (win_rect->left + state->width), curr_pos.y - (win_rect->top + state->height));
				HMONITOR curr_monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
				MONITORINFO monitor_info;
				monitor_info.cbSize = sizeof(monitor_info);
				GetMonitorInfo(curr_monitor, &monitor_info);
				RECT monitor_rect = monitor_info.rcWork;

				if (abs(win_rect->left - monitor_rect.left) <= 10)
					OffsetRect(win_rect, monitor_rect.left - win_rect->left, 0);
				else if (abs(win_rect->right - monitor_rect.right) <= 10)
					OffsetRect(win_rect, monitor_rect.right - win_rect->right, 0);
				
				if (abs(win_rect->top - monitor_rect.top) <= 10)
					OffsetRect(win_rect, 0, monitor_rect.top - win_rect->top);
				else if (abs(win_rect->bottom - monitor_rect.bottom) <= 10)
					OffsetRect(win_rect, 0, monitor_rect.bottom - win_rect->bottom);
			}
			
		} break;

		case WM_NOTIFY:
		{
			LPNMHDR msg_info = (LPNMHDR)lParam;
			
			// Handle notify messages from the playlist ListView
			if (msg_info->hwndFrom == state->controls.playlist_hwnd)
			{
				if (msg_info->code == NM_CUSTOMDRAW)
				{
					return PaintPlaylist(state, (LPNMLVCUSTOMDRAW)lParam);
				}
				else if (msg_info->code == NM_DBLCLK)
				{
					PlaylistDoubleClickHandler(state);
				}
				else if (msg_info->code == LVN_KEYDOWN)
				{
					LPNMLVKEYDOWN key_info = (LPNMLVKEYDOWN)lParam;
					SendMessage(state->main_hwnd, WM_KEYDOWN, (WPARAM)key_info->wVKey, 0);
				}
			}

		} break;

		case WM_KEYDOWN:
		{
			KeyDownHandler(state, (int)wParam);

		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_code)
{
	// Before doing anything else, ensure that the correct version of bass.dll is included.
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
	{
		MessageBox(0, "Error:  An incorrect version of BASS.DLL was found.  Program will exit.", 0, MB_ICONERROR);
		return 0;
	}
	
	WNDCLASS main_class = {};
	main_class.lpfnWndProc = MainProc;
	main_class.hInstance = instance;
	main_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	main_class.lpszClassName = "Winphonic";
	main_class.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	main_class.style = CS_DROPSHADOW;	// Gives main window a tiny drop shadow, like Windows 10 Explorer
	
	ImageButtonRegisterClass();
	TextButtonRegisterClass();
	TextLabelRegisterClass();
	ImageLabelRegisterClass();
	TrackbarRegisterClass();

	if (RegisterClass(&main_class))
	{
		AppState* state = (AppState*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AppState));

		// Read the settings from the INI file
		GetINIFilePath(state->ini_path, MAX_PATH);
		ReadSettings(state, state->ini_path);
		
		// Create the main window
		HWND main_hwnd = CreateWindow(main_class.lpszClassName, "Winphonic", WS_VISIBLE | WS_POPUP, 
			state->options.x, state->options.y, WIN_WIDTH, WIN_HEIGHT, 0, 0, instance, state);

		if (!BASS_Init(-1, 48000, 0, state->main_hwnd, NULL))
		{
			if (!BASS_Init(-1, 44100, 0, state->main_hwnd, NULL))
			{
				MessageBox(0, "Error:  Could not initialize BASS.  Program will exit.", 0, MB_ICONERROR);
				return 0;
			}
		}

		ReadPlaylistFromSettings(state, state->ini_path);
		
		// Message processing loop
		if (main_hwnd)
		{
			state->is_running = true;
			while (state->is_running)
			{
				MSG message;
				if (GetMessage(&message, 0, 0, 0) > 0)
				{
					if (IsWindow(g_about_dlg_hwnd))
					{
						if (IsDialogMessage(g_about_dlg_hwnd, &message))
							continue;
					}
					TranslateMessage(&message);
					DispatchMessage(&message);
					
				}
				else
				{
					break;
				}
			}

			// Clean up before shutting down.
			BASS_Free();
			KillTimer(main_hwnd, TIMER_UPDATE_SONG_POS);
			WriteSettings(state, state->ini_path);
		}
	}

	ImageButtonUnregisterClass();
	TextButtonUnregisterClass();
	TextLabelUnregisterClass();
	ImageLabelUnregisterClass();
	TrackbarUnregisterClass();
	
	return 0;
}