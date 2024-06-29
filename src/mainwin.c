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

#include "skippy.h"

/* from 'uncover': */
static Visual *
find_argb_visual (Display *dpy, int scr)
{
	XVisualInfo template = {
		.screen = scr, .depth = 32, .class = TrueColor,
	};
	int	nvi = 0;
	XVisualInfo *xvi = XGetVisualInfo (dpy,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
			&template, &nvi);
	if (!xvi) return NULL;

	Visual *visual = NULL;;
	for (int i = 0; i < nvi; ++i) {
		XRenderPictFormat *format =
			XRenderFindVisualFormat (dpy, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask) {
			visual = xvi[i].visual;
			break;
		}
	}

	XFree (xvi);
	return visual;
}

MainWin *
mainwin_create(session_t *ps) {
	Display * const dpy = ps->dpy;

	// Get ARGB visual.
	// FIXME: Move this to skippy.c?
	if (!ps->argb_visual)
		ps->argb_visual = find_argb_visual(dpy, ps->screen);

	// calloc() makes sure it's filled with zero
	MainWin *mw = allocchk(calloc(1, sizeof(MainWin)));

	mw->ps = ps;
	mw->bg_pixmap = None;
	mw->background = None;

#ifdef CFG_XINERAMA
	mw->xin_info = mw->xin_active = 0;
	mw->xin_screens = 0;
#endif /* CFG_XINERAMA */
	
	// mw->pressed = mw->focus = 0;
	mw->pressed = mw->client_to_focus = 0;
	mw->clientondesktop = 0;
	mw->focuslist = 0;
	mw->refocus = false;

	XWindowAttributes rootattr;
	XGetWindowAttributes(dpy, ps->root, &rootattr);
	mw->x = mw->y = 0;
	mw->width = rootattr.width;
	mw->height = rootattr.height;

	if (!ps->o.pseudoTrans) {
		mw->depth  = 32;
		mw->visual = ps->argb_visual;
		if (!mw->visual) {
			printfef(true, "(): Couldn't find ARGB visual, lazy transparency can't work.");
			goto mainwin_create_err;
		}
	}
    else {
		mw->depth = DefaultDepth(dpy, ps->screen);
		mw->visual = DefaultVisual(dpy, ps->screen);
	}

	mw = mainwin_reload(ps, mw);
	if (!mw)
		goto mainwin_create_err;

	XSetWindowAttributes wattr;
	wattr.colormap = mw->colormap;
	wattr.background_pixel = wattr.border_pixel = 0;
	wattr.override_redirect = True;
	// I have no idea why, but seemingly without ButtonPressMask, we can't
	// receive ButtonRelease events in some cases
	wattr.event_mask = VisibilityChangeMask | ButtonPressMask
		| ButtonReleaseMask | KeyPressMask | KeyReleaseMask;

	mw->window = XCreateWindow(dpy, ps->root, 0, 0, mw->width, mw->height, 0,
			mw->depth, InputOutput, mw->visual,
			CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect, &wattr);
	if (!mw->window)
		goto mainwin_create_err;

	wm_wid_set_info(ps, mw->window, "skippy-xd main window", None);

	mainwin_create_pixmap(mw);

	XCompositeRedirectSubwindows (ps->dpy, ps->root, CompositeRedirectAutomatic);

	return mw;

mainwin_create_err:
	if (mw)
		free(mw);
	return NULL;
}

MainWin *
mainwin_reload(session_t *ps, MainWin *mw) {
	Display * const dpy = ps->dpy;

	// convert the keybindings settings strings into arrays of KeySyms
	keys_str_syms(ps->o.bindings_keysUp, &mw->keysyms_Up);
	keys_str_syms(ps->o.bindings_keysDown, &mw->keysyms_Down);
	keys_str_syms(ps->o.bindings_keysLeft, &mw->keysyms_Left);
	keys_str_syms(ps->o.bindings_keysRight, &mw->keysyms_Right);
	keys_str_syms(ps->o.bindings_keysPrev, &mw->keysyms_Prev);
	keys_str_syms(ps->o.bindings_keysNext, &mw->keysyms_Next);
	keys_str_syms(ps->o.bindings_keysCancel, &mw->keysyms_Cancel);
	keys_str_syms(ps->o.bindings_keysSelect, &mw->keysyms_Select);
	keys_str_syms(ps->o.bindings_keysIconify, &mw->keysyms_Iconify);
	keys_str_syms(ps->o.bindings_keysShade, &mw->keysyms_Shade);
	keys_str_syms(ps->o.bindings_keysClose, &mw->keysyms_Close);
	keys_str_syms(ps->o.bindings_keysPivotSwitch, &mw->keysyms_PivotSwitch);
	keys_str_syms(ps->o.bindings_keysPivotExpose, &mw->keysyms_PivotExpose);
	keys_str_syms(ps->o.bindings_keysPivotPaging, &mw->keysyms_PivotPaging);

	// convert the arrays of KeySyms into arrays of KeyCodes, for this specific Display
	keysyms_arr_keycodes(dpy, mw->keysyms_Up, &mw->keycodes_Up);
	keysyms_arr_keycodes(dpy, mw->keysyms_Down, &mw->keycodes_Down);
	keysyms_arr_keycodes(dpy, mw->keysyms_Left, &mw->keycodes_Left);
	keysyms_arr_keycodes(dpy, mw->keysyms_Right, &mw->keycodes_Right);
	keysyms_arr_keycodes(dpy, mw->keysyms_Prev, &mw->keycodes_Prev);
	keysyms_arr_keycodes(dpy, mw->keysyms_Next, &mw->keycodes_Next);
	keysyms_arr_keycodes(dpy, mw->keysyms_Cancel, &mw->keycodes_Cancel);
	keysyms_arr_keycodes(dpy, mw->keysyms_Select, &mw->keycodes_Select);
	keysyms_arr_keycodes(dpy, mw->keysyms_Iconify, &mw->keycodes_Iconify);
	keysyms_arr_keycodes(dpy, mw->keysyms_Shade, &mw->keycodes_Shade);
	keysyms_arr_keycodes(dpy, mw->keysyms_Close, &mw->keycodes_Close);
	keysyms_arr_keycodes(dpy, mw->keysyms_PivotSwitch, &mw->keycodes_PivotSwitch);
	keysyms_arr_keycodes(dpy, mw->keysyms_PivotExpose, &mw->keycodes_PivotExpose);
	keysyms_arr_keycodes(dpy, mw->keysyms_PivotPaging, &mw->keycodes_PivotPaging);

	// we check all possible pairs, one pair at a time. This is in a specific order, to give a more helpful error msg
	check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysDown", mw->keysyms_Down);
	check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysLeft", mw->keysyms_Left);
	check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysRight", mw->keysyms_Right);
	check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysSelect", mw->keysyms_Select);
	//check_keybindings_conflict(ps->o.config_path, "keysUp", mw->keysyms_Up, "keysPivot", mw->keysyms_Pivot);
	check_keybindings_conflict(ps->o.config_path, "keysDown", mw->keysyms_Down, "keysLeft", mw->keysyms_Left);
	check_keybindings_conflict(ps->o.config_path, "keysDown", mw->keysyms_Down, "keysRight", mw->keysyms_Right);
	check_keybindings_conflict(ps->o.config_path, "keysDown", mw->keysyms_Down, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysDown", mw->keysyms_Down, "keysSelect", mw->keysyms_Select);
	//check_keybindings_conflict(ps->o.config_path, "keysDown", mw->keysyms_Down, "keysPivot", mw->keysyms_Pivot);
	check_keybindings_conflict(ps->o.config_path, "keysLeft", mw->keysyms_Left, "keysRight", mw->keysyms_Right);
	check_keybindings_conflict(ps->o.config_path, "keysLeft", mw->keysyms_Left, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysLeft", mw->keysyms_Left, "keysSelect", mw->keysyms_Select);
	//check_keybindings_conflict(ps->o.config_path, "keysLeft", mw->keysyms_Left, "keysPivot", mw->keysyms_Pivot);
	check_keybindings_conflict(ps->o.config_path, "keysRight", mw->keysyms_Prev, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysRight", mw->keysyms_Prev, "keysSelect", mw->keysyms_Select);
	//check_keybindings_conflict(ps->o.config_path, "keysRight", mw->keysyms_Prev, "keysPivot", mw->keysyms_Pivot);
	check_keybindings_conflict(ps->o.config_path, "keysPrev", mw->keysyms_Prev, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysPrev", mw->keysyms_Prev, "keysSelect", mw->keysyms_Select);
	check_keybindings_conflict(ps->o.config_path, "keysNext", mw->keysyms_Next, "keysCancel", mw->keysyms_Cancel);
	check_keybindings_conflict(ps->o.config_path, "keysNext", mw->keysyms_Next, "keysSelect", mw->keysyms_Select);
	check_keybindings_conflict(ps->o.config_path, "keysCancel", mw->keysyms_Cancel, "keysSelect", mw->keysyms_Select);
	
	if (ps->o.updateFreq != 0.0)
		mw->poll_time = (1.0 / ps->o.updateFreq) * 1000.0;
	else
		mw->poll_time = (1.0 / 60.0) * 1000.0;

	mw->colormap = XCreateColormap(dpy, ps->root, mw->visual, AllocNone);
	mw->format = XRenderFindVisualFormat(dpy, mw->visual);

	XColor exact_color;

	if(!XParseColor(ps->dpy, mw->colormap, ps->o.normal_tint, &exact_color)) {
		printfef(true, "(): Couldn't look up color '%s', reverting to black.", ps->o.normal_tint);
		mw->normalTint.red = mw->normalTint.green = mw->normalTint.blue = 0;
	}
	else
	{
		mw->normalTint.red = exact_color.red;
		mw->normalTint.green = exact_color.green;
		mw->normalTint.blue = exact_color.blue;
	}
	mw->normalTint.alpha = alphaconv(ps->o.normal_tintOpacity);

	if(! XParseColor(ps->dpy, mw->colormap, ps->o.highlight_tint, &exact_color))
	{
		printfef(true, "(): Couldn't look up color '%s', reverting to #444444", ps->o.highlight_tint);
		mw->highlightTint.red = mw->highlightTint.green = mw->highlightTint.blue = 0x44;
	}
	else
	{
		mw->highlightTint.red = exact_color.red;
		mw->highlightTint.green = exact_color.green;
		mw->highlightTint.blue = exact_color.blue;
	}
	mw->highlightTint.alpha = alphaconv(ps->o.highlight_tintOpacity);

	;
	if(! XParseColor(ps->dpy, mw->colormap, ps->o.shadow_tint, &exact_color))
	{
		printfef(true, "(): Couldn't look up color '%s', reverting to #040404", ps->o.shadow_tint);
		mw->shadowTint.red = mw->shadowTint.green =	mw->shadowTint.blue = 0x04;
	}
	else
	{
		mw->shadowTint.red = exact_color.red;
		mw->shadowTint.green = exact_color.green;
		mw->shadowTint.blue = exact_color.blue;
	}
	mw->shadowTint.alpha = alphaconv(ps->o.shadow_tintOpacity);

	mw->distance = ps->o.distance;

	return mw;
}

MainWin *
mainwin_create_pixmap(MainWin *mw) {
	session_t *ps = mw->ps;

	XRenderPictureAttributes pa;
	XRenderColor clear;

	pa.repeat = True;
	clear.alpha = alphaconv(ps->o.normal_opacity);
	if(mw->normalPixmap != None)
		XFreePixmap(ps->dpy, mw->normalPixmap);
	mw->normalPixmap = XCreatePixmap(ps->dpy, mw->window, 1, 1, 8);
	if(mw->normalPicture != None)
		XRenderFreePicture(ps->dpy, mw->normalPicture);
	mw->normalPicture = XRenderCreatePicture(ps->dpy, mw->normalPixmap,
			XRenderFindStandardFormat(ps->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(ps->dpy, PictOpSrc, mw->normalPicture, &clear, 0, 0, 1, 1);

	clear.alpha = alphaconv(ps->o.highlight_opacity);
	if(mw->highlightPixmap != None)
		XFreePixmap(ps->dpy, mw->highlightPixmap);
	mw->highlightPixmap = XCreatePixmap(ps->dpy, mw->window, 1, 1, 8);
	if(mw->highlightPicture != None)
		XRenderFreePicture(ps->dpy, mw->highlightPicture);
	mw->highlightPicture = XRenderCreatePicture(ps->dpy, mw->highlightPixmap,
			XRenderFindStandardFormat(ps->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(ps->dpy, PictOpSrc, mw->highlightPicture, &clear, 0, 0, 1, 1);

	clear.alpha = alphaconv(ps->o.shadow_opacity);
	if(mw->shadowPixmap != None)
		XFreePixmap(ps->dpy, mw->shadowPixmap);
	mw->shadowPixmap = XCreatePixmap(ps->dpy, mw->window, 1, 1, 8);
	if(mw->shadowPicture != None)
		XRenderFreePicture(ps->dpy, mw->shadowPicture);
	mw->shadowPicture = XRenderCreatePicture(ps->dpy, mw->shadowPixmap,
			XRenderFindStandardFormat(ps->dpy, PictStandardA8), CPRepeat, &pa);
	XRenderFillRectangle(ps->dpy, PictOpSrc, mw->shadowPicture, &clear, 0, 0, 1, 1);

	return mw;
}

void
mainwin_update_background_config(MainWin *mw) {
	session_t *ps = mw->ps;
	pictspec_t spec = ps->o.bg_spec;

	spec.twidth = mw->width;
	spec.theight = mw->height;

	pictw_t *p = simg_load_s(ps, &spec);
	if (!p)
		return;

	free_pictw(ps, &ps->o.background);
	ps->o.background = simg_postprocess(ps, p, PICTPOSP_ORIG,
			mw->width, mw->height, spec.alg, spec.valg, &spec.c);
}

void
mainwin_update_background(MainWin *mw) {
	session_t *ps = mw->ps;

	mainwin_update_background_config(mw);

	if(mw->bg_pixmap)
		XFreePixmap(ps->dpy, mw->bg_pixmap);
	mw->bg_pixmap = XCreatePixmap(ps->dpy, mw->window,
			mw->width, mw->height, mw->depth);

	XRenderPictureAttributes pa;
	pa.repeat = True;

	if(mw->background)
		XRenderFreePicture(ps->dpy, mw->background);
	mw->background = XRenderCreatePicture(ps->dpy,
			mw->bg_pixmap, mw->format, CPRepeat, &pa);
	
	Pixmap root = wm_get_root_pmap(ps->dpy);
	Picture from = XRenderCreatePicture(ps->dpy, root,
			XRenderFindVisualFormat(ps->dpy,
				DefaultVisual(ps->dpy, ps->screen)), 0, 0);
	XRenderComposite(ps->dpy,
			PictOpSrc, from,
			None, mw->background, mw->x, mw->y, 0, 0, 0, 0, mw->width, mw->height);

	if (ps->o.background)
		XRenderComposite(ps->dpy,
				PictOpOver, ps->o.background->pict,
				None, mw->background, 0, 0, 0, 0, 0, 0, mw->width, mw->height);

	if (ps->o.from)
		XRenderFreePicture(ps->dpy, ps->o.from);
	ps->o.from = from;
	XSetWindowBackgroundPixmap(ps->dpy, mw->window, mw->bg_pixmap);
	XClearWindow(ps->dpy, mw->window);
}

void
mainwin_update(MainWin *mw)
{
	session_t * const ps = mw->ps;

#ifdef CFG_XINERAMA
	XineramaScreenInfo *iter;
	int i;
	Window dummy_w;
	int root_x, root_y, dummy_i;
	unsigned int dummy_u;

	if (ps->xinfo.xinerama_exist && XineramaIsActive(ps->dpy)) {
		if(mw->xin_info)
			XFree(mw->xin_info);
		mw->xin_info = XineramaQueryScreens(ps->dpy, &mw->xin_screens);
		printfdf(false, "(): Xinerama is enabled (%d screens).", mw->xin_screens);
	}
	
	if(! mw->xin_info || ! mw->xin_screens)
	{
		mainwin_update_background(mw);
		return;
	}
	
	printfdf(false, "(): XINERAMA --> querying pointer... ");
	XQueryPointer(ps->dpy, ps->root, &dummy_w, &dummy_w, &root_x, &root_y, &dummy_i, &dummy_i, &dummy_u);
	printfdf(false, "(): XINERAMA +%i+%i\n", root_x, root_y);
	
	printfdf(false, "(): XINERAMA --> figuring out which screen we're on... ");
	iter = mw->xin_info;
	for(i = 0; i < mw->xin_screens; ++i)
	{
		if(root_x >= iter->x_org && root_x < iter->x_org + iter->width &&
		   root_y >= iter->y_org && root_y < iter->y_org + iter->height)
		{
			printfdf(false, "(): XINERAMA screen %i %ix%i+%i+%i\n", iter->screen_number, iter->width, iter->height, iter->x_org, iter->y_org);
			break;
		}
		iter++;
	}
	if(i == mw->xin_screens)
	{
		printfdf(false, "(): XINERAMA unknown\n");
		return;
	}
	mw->x = iter->x_org;
	mw->y = iter->y_org;
	mw->width = iter->width;
	mw->height = iter->height;
	XMoveResizeWindow(ps->dpy, mw->window, iter->x_org, iter->y_org, mw->width, mw->height);
	mw->xin_active = iter;
#endif /* CFG_XINERAMA */
	mainwin_update_background(mw);
}

void
mainwin_map(MainWin *mw) {
	session_t *ps = mw->ps;

	wm_set_fullscreen(ps, mw->window, mw->x, mw->y, mw->width, mw->height);
	mw->pressed = NULL;
	mw->pressed_key = mw->pressed_mouse = false;
	XMapWindow(ps->dpy, mw->window);
	XRaiseWindow(ps->dpy, mw->window);

	// Might because of WM reparent, XSetInputFocus() doesn't work here
	// Focus is already on mini window?
	XSetInputFocus(ps->dpy, mw->window, RevertToParent, CurrentTime);
	/* {
		int ret = XGrabKeyboard(ps->dpy, mw->window, True, GrabModeAsync,
				GrabModeAsync, CurrentTime);
		if (Success != ret)
			printfef(true, "(): Failed to grab keyboard (%d), troubles ahead.", ret);
	} */
	mw->mapped = true;
}

void
mainwin_unmap(MainWin *mw)
{
	if(mw->bg_pixmap)
	{
		XFreePixmap(mw->ps->dpy, mw->bg_pixmap);
		mw->bg_pixmap = None;
	}
	XUngrabKeyboard(mw->ps->dpy, CurrentTime);
	XUnmapWindow(mw->ps->dpy, mw->window);
	mw->mapped = false;
}

void
mainwin_destroy(MainWin *mw) {
	session_t *ps = mw->ps; 

	// Free all clients associated with this main window
	dlist_free_with_func(mw->clients, (dlist_free_func) clientwin_destroy);

	dlist_free(mw->clientondesktop);
	dlist_free(mw->panels);

	if(mw->background != None)
		XRenderFreePicture(ps->dpy, mw->background);
	
	if(mw->bg_pixmap != None)
		XFreePixmap(ps->dpy, mw->bg_pixmap);
	
	if(mw->normalPicture != None)
		XRenderFreePicture(ps->dpy, mw->normalPicture);
	
	if(mw->highlightPicture != None)
		XRenderFreePicture(ps->dpy, mw->highlightPicture);
	
	if(mw->normalPixmap != None)
		XFreePixmap(ps->dpy, mw->normalPixmap);
	
	if(mw->highlightPixmap != None)
		XFreePixmap(ps->dpy, mw->highlightPixmap);
	
	XDestroyWindow(ps->dpy, mw->window);
	
#ifdef CFG_XINERAMA
	if(mw->xin_info)
		XFree(mw->xin_info);
#endif /* CFG_XINERAMA */
	
	free(mw->keysyms_Up);
	free(mw->keysyms_Down);
	free(mw->keysyms_Left);
	free(mw->keysyms_Right);
	free(mw->keysyms_Prev);
	free(mw->keysyms_Next);
	free(mw->keysyms_Cancel);
	free(mw->keysyms_Select);
	free(mw->keysyms_Iconify);
	free(mw->keysyms_Shade);
	free(mw->keysyms_Close);
	free(mw->keysyms_PivotSwitch);
	free(mw->keysyms_PivotExpose);
	free(mw->keysyms_PivotPaging);

	free(mw->keycodes_Up);
	free(mw->keycodes_Down);
	free(mw->keycodes_Left);
	free(mw->keycodes_Right);
	free(mw->keycodes_Prev);
	free(mw->keycodes_Next);
	free(mw->keycodes_Cancel);
	free(mw->keycodes_Select);
	free(mw->keycodes_Iconify);
	free(mw->keycodes_Shade);
	free(mw->keycodes_Close);
	free(mw->keycodes_PivotSwitch);
	free(mw->keycodes_PivotExpose);
	free(mw->keycodes_PivotPaging);

	free(mw);
}

void
mainwin_transform(MainWin *mw, float f)
{
	mw->transform.matrix[0][0] = XDoubleToFixed(1.0 / f);
	mw->transform.matrix[0][1] = 0.0;
	mw->transform.matrix[0][2] = 0.0;
	mw->transform.matrix[1][0] = 0.0;
	mw->transform.matrix[1][1] = XDoubleToFixed(1.0 / f);
	mw->transform.matrix[1][2] = 0.0;
	mw->transform.matrix[2][0] = 0.0;
	mw->transform.matrix[2][1] = 0.0;
	mw->transform.matrix[2][2] = XDoubleToFixed(1.0);
}

int
mainwin_handle(MainWin *mw, XEvent *ev) {
	printfdf(false, "(): ");
	session_t *ps = mw->ps;

	switch(ev->type) {
		case EnterNotify:
			printfdf(false, "(): EnterNotify");
			if (!mw->client_to_focus)
				XSetInputFocus(ps->dpy, mw->window, RevertToParent, CurrentTime);
			break;
		case KeyPress:
		case KeyRelease:
			printfdf(false, "(): KeyPress or KeyRelease");
			// if(mw->client_to_focus)
			// {
				// printfdf(false, "(): clientwin_handle(mw->client_to_focus, ev);");
			if(clientwin_handle(mw->client_to_focus, ev))
				return 1;

			// }
			// else
			// {
			// 	printfdf(false, "(): mw->client_to_focus == NULL");				
			// }
			break;
		case ButtonPress:
			mw->pressed_mouse = true;
			break;
		case ButtonRelease:
			if (mw->pressed_mouse) {
				const unsigned button = ev->xbutton.button;
				if (button < MAX_MOUSE_BUTTONS) {
					enum cliop action = ps->o.bindings_miwMouse[button];
					if (action == CLIENTOP_PREV) {
						focus_miniw_prev(ps, mw->client_to_focus);
						return 0;
					}
					else if (action == CLIENTOP_NEXT) {
						focus_miniw_next(ps, mw->client_to_focus);
						return 0;
					}
					else if (action == CLIENTOP_FOCUS) {
						mw->refocus = true;
						return 1;
					}
					else if (action == CLIENTOP_NO) {
						return 0;
					}
				}
				printfdf(false, "(): Detected mouse button release on main window, "
						"exiting.");
				return 1;
			}
			else
				printfdf(false, "(): ButtonRelease %u ignored.", ev->xbutton.button);
			break;
	}

	return 0;
}
