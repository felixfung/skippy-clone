/* Skippy - Seduces Kids Into Perversion
 *
 * Copyright (C) 2004 Hyriand <hyriand@thegraveyard.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SKIPPY_MAINWIN_H
#define SKIPPY_MAINWIN_H

struct _mainwin_t {
	session_t *ps;
	Visual *visual;
	Colormap colormap;
	int depth;
	dlist *clients;
	
	/// @brief Whether the KeyRelease events should already be acceptable.
	bool pressed_key;
	/// @brief Whether the ButtonRelease events should already be acceptable.
	bool pressed_mouse;
	int poll_time;
	
	Window window;
	Picture background;
	Pixmap bg_pixmap;
	int x, y, xoff, yoff;
	int width, height, distance;
	float multiplier;

	XRenderPictFormat *format;
	XTransform transform, desktoptransform;
	
	XRenderColor normalTint, highlightTint, shadowTint;
	Pixmap normalPixmap, highlightPixmap, shadowPixmap;
	Picture normalPicture, highlightPicture, shadowPicture;
	
	ClientWin *pressed, *focus;
	dlist *clientondesktop, *focuslist, *desktopwins, *dminis, *panels;
	
	KeySym *keysyms_Up;
	KeySym *keysyms_Down;
	KeySym *keysyms_Left;
	KeySym *keysyms_Right;
	KeySym *keysyms_Prev;
	KeySym *keysyms_Next;
	KeySym *keysyms_Cancel;
	KeySym *keysyms_Select;
	KeySym *keysyms_Iconify;
	KeySym *keysyms_Shade;
	KeySym *keysyms_Close;
	KeySym *keysyms_PivotSwitch;
	KeySym *keysyms_PivotExpose;
	KeySym *keysyms_PivotPaging;

	KeyCode *keycodes_Up;
	KeyCode *keycodes_Down;
	KeyCode *keycodes_Left;
	KeyCode *keycodes_Right;
	KeyCode *keycodes_Prev;
	KeyCode *keycodes_Next;
	KeyCode *keycodes_Cancel;
	KeyCode *keycodes_Select;
	KeyCode *keycodes_Iconify;
	KeyCode *keycodes_Shade;
	KeyCode *keycodes_Close;
	KeyCode *keycodes_PivotSwitch;
	KeyCode *keycodes_PivotExpose;
	KeyCode *keycodes_PivotPaging;

	bool refocus;
	bool mapped;

#ifdef CFG_XINERAMA
	int xin_screens;
	XineramaScreenInfo *xin_info, *xin_active;
#endif /* CFG_XINERAMA */

	/// @brief The client window to eventually focus.
	ClientWin *client_to_focus;
	/// @brief the originally focused window
	ClientWin *client_to_focus_on_cancel;
};

MainWin *mainwin_create(session_t *ps);
MainWin *mainwin_reload(session_t *ps, MainWin *mw);
void mainwin_destroy(MainWin *);
void mainwin_map(MainWin *);
void mainwin_unmap(MainWin *);
int mainwin_handle(MainWin *, XEvent *);
void mainwin_update_background_config(MainWin *mw);
void mainwin_update_background(MainWin *mw);
void mainwin_update(MainWin *mw);
MainWin *mainwin_create_pixmap(MainWin *mw);
void mainwin_transform(MainWin *mw, float f);

#endif /* SKIPPY_MAINWIN_H */
