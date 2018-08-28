// Copyright (C) 2018 Alaskan Emily, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.


#include "glow.h"

#include <interface/Window.h>
#include <opengl/GLView.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

struct Glow_Window {
    BGLView *gl_view;
    BWindow *b_window;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

static const int glow_gl_view_flags = BGL_RGB|BGL_ALPHA|BGL_DEPTH|BGL_DOUBLE|BGL_ACCUM|BGL_STENCIL;

///////////////////////////////////////////////////////////////////////////////////////////////////

void Glow_ViewportSize(unsigned w, unsigned h, unsigned *out_w, unsigned *out_h){
    out_w[0] = w;
    out_h[0] = h;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void *Glow_GetSystemWindow(Glow_Window *window){
    return window->gl_view;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned Glow_WindowStructSize(){
    return sizeof(Glow_Window);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Glow_CreateWindow(Glow_Window *out, unsigned w, unsigned h, const char *title, int flags){
    const BRect rect = BRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    const window_look look = ((flags & GLOW_UNDECORATED) != 0) ?
        B_NO_BORDER_WINDOW_LOOK :
        B_DOCUMENT_WINDOW_LOOK;
    const window_feel feel = B_NORMAL_WINDOW_FEEL;
    const int window_flags = ((flags & GLOW_RESIZABLE) != 0) ?
        B_NOT_RESIZABLE : 0;

    BWindow *const b_window = new BWindow(rect, title, look, feel, window_flags);
    BGLView *const gl_view = new BGLView(rect, "Glow", B_FOLLOW_ALL_SIDES, 0, glow_gl_view_flags);
    b_window->AddChild(gl_view);
    out->b_window = b_window;
    out->gl_view = gl_view;
}
