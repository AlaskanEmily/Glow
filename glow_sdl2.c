/* Copyright (C) 2019 Alaskan Emily, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "glow.h"
#include "SDL.h"
#include "SDL_video.h"
#include "SDL_syswm.h"

/******************************************************************************/

struct Glow_Context{
    SDL_Window *win;
    SDL_GLContext ctx;
};

/******************************************************************************/

struct Glow_Window{
    SDL_Window *win;
    struct Glow_Context ctx;
};

/******************************************************************************/

void Glow_ViewportSize(unsigned w, unsigned h,
    unsigned *out_w, unsigned *out_h){
    out_w[0] = w;
    out_h[0] = h;
}

/******************************************************************************/

void *Glow_GetSystemWindow(struct Glow_Window *window){
    SDL_SysWMinfo info;
    if(!SDL_GetWindowWMInfo(window->win, &info))
        return NULL;
    switch(info.subsystem){
#ifdef SDL_VIDEO_DRIVER_WINDOWS
        case SDL_SYSWM_WINDOWS:
            return (void*)info.info.win.window;
#endif
        
#ifdef SDL_VIDEO_DRIVER_WINRT
        case SDL_SYSWM_WINRT:
            return info.info.winrt.window;
#endif
        
#ifdef SDL_VIDEO_DRIVER_MIR
        case SDL_SYSWM_MIR:
            return info.info.mir.surface;
#endif
        
#ifdef SDL_VIDEO_DRIVER_WAYLAND
        case SDL_SYSWM_WAYLAND:
            return info.info.wl.surface;
#endif
        
#ifdef SDL_VIDEO_DRIVER_X11
        case SDL_SYSWM_X11:
            return (void*)info.info.x11.window;
#endif
        
#ifdef SDL_VIDEO_DRIVER_DIRECTFB
        case SDL_SYSWM_DIRECTFB:
            return info.info.dfb.window;
#endif
        
#ifdef SDL_VIDEO_DRIVER_COCOA
        case SDL_SYSWM_COCOA:
            return info.info.cocoa.window;
#endif
        
#ifdef SDL_VIDEO_DRIVER_UIKIT
        case SDL_SYSWM_UIKIT:
            return info.info.uikit.window;
#endif
        case SDL_SYSWM_UNKNOWN: /* FALLTHROUGH */
        default:
            return NULL;
    }
}

/******************************************************************************/

void *Glow_GetProcAddress(const char *name){
    return SDL_GL_GetProcAddress(name);
}

/******************************************************************************/

unsigned Glow_WindowStructSize(void){
    return sizeof(struct Glow_Window);
}

/******************************************************************************/

void Glow_CreateWindow(struct Glow_Window *out,
    unsigned w, unsigned h, const char *title, int flags){
    
    int sdl_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
    
#ifdef SDL_WINDOW_ALLOW_HIGHDPI
    sdl_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    
    if((flags & GLOW_RESIZABLE) != 0)
        sdl_flags |= SDL_WINDOW_RESIZABLE;
    if((flags & GLOW_UNDECORATED) != 0)
        sdl_flags |= SDL_WINDOW_BORDERLESS;
    
    /* This is used every time a window is created. SDL2 ref-counts this, so
     * it is OK to use this on window construction and a matched quit occurs on
     * window destruction.
     */
    SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_EVENTS);
    
    out->ctx.win = out->win = SDL_CreateWindow(title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, sdl_flags);
    out->ctx.ctx = NULL;
}

/******************************************************************************/

void Glow_DestroyWindow(struct Glow_Window *win){
    SDL_DestroyWindow(win->win);
    if(win->ctx.ctx != NULL)
        SDL_GL_DeleteContext(win->ctx.ctx);
    SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_EVENTS);
}

/******************************************************************************/

void Glow_SetTitle(struct Glow_Window *win, const char *title){
    SDL_SetWindowTitle(win->win, title);
}

/******************************************************************************/

void Glow_ShowWindow(struct Glow_Window *win){
    SDL_ShowWindow(win->win);
    SDL_RaiseWindow(win->win);
}

/******************************************************************************/

void Glow_HideWindow(struct Glow_Window *win){
    SDL_HideWindow(win->win);
}

/******************************************************************************/

void Glow_GetWindowSize(const struct Glow_Window *win,
    unsigned *out_w, unsigned *out_h){

    int w, h;
    SDL_GetWindowSize(win->win, &w, &h);
    out_w[0] = w;
    out_h[0] = h;
}

/******************************************************************************/

void Glow_FlipScreen(struct Glow_Window *win){
    SDL_GL_SwapWindow(win->win);
}

/******************************************************************************/

static int glow_write_keyname(const SDL_Event *in, struct Glow_Event *out){
    const char *const name = SDL_GetKeyName(in->key.keysym.sym);
#if defined _WIN32 || defined __CYGWIN__ || defined WIN32
    strncpy(out->value.key, name, GLOW_MAX_KEY_NAME_SIZE-2);
    out->value.key[GLOW_MAX_KEY_NAME_SIZE-1] = 0;
#else
    strlcpy(out->value.key, name, GLOW_MAX_KEY_NAME_SIZE-1);
#endif
    return 1;
}

/******************************************************************************/

static enum Glow_MouseButton glow_translate_button(unsigned s){
    if((s & SDL_BUTTON_LMASK) != 0)
        return eGlowLeft;
    if((s & SDL_BUTTON_RMASK) != 0)
        return eGlowRight;
    if((s & SDL_BUTTON_MMASK) != 0)
        return eGlowMiddle;
    return eGlow_NUM_BUTTONS;
}

/******************************************************************************/

static int glow_convert_sdl2_event(const SDL_Event *in,
    struct Glow_Event *out){
    
    switch(in->type){
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
            out->type = eGlowQuit;
            return 1;
        case SDL_KEYDOWN:
            out->type = eGlowKeyboardPressed;
            return glow_write_keyname(in, out);
        case SDL_KEYUP:
            out->type = eGlowKeyboardReleased;
            return glow_write_keyname(in, out);
        case SDL_MOUSEMOTION:
            out->type = eGlowMouseMoved;
            out->value.mouse.xy[0] = in->motion.x;
            out->value.mouse.xy[1] = in->motion.y;
            out->value.mouse.button = glow_translate_button(in->motion.state);
            return 1;
        case SDL_MOUSEBUTTONDOWN:
            out->type = eGlowMousePressed;
            out->value.mouse.xy[0] = in->button.x;
            out->value.mouse.xy[1] = in->button.y;
            out->value.mouse.button = glow_translate_button(in->button.state);
            return 1;
        case SDL_MOUSEBUTTONUP:
            out->type = eGlowMouseReleased;
            out->value.mouse.xy[0] = in->button.x;
            out->value.mouse.xy[1] = in->button.y;
            out->value.mouse.button = glow_translate_button(in->button.state);
            return 1;
        case SDL_WINDOWEVENT:
            switch(in->window.event){
                case SDL_WINDOWEVENT_SIZE_CHANGED: /* FALLTHROUGH */
                case SDL_WINDOWEVENT_RESIZED:
                    out->type = eGlowResized;
                    out->value.resize[0] = in->window.data1;
                    out->value.resize[1] = in->window.data2;
                    return 1;
            }
            break;
    }
    
    return 0;
}

/******************************************************************************/

unsigned Glow_GetEvent(struct Glow_Window *win, struct Glow_Event *out_event){
    
    SDL_Event ev;
    (void)win;
    
    while(SDL_PollEvent(&ev)){
        if(glow_convert_sdl2_event(&ev, out_event))
            return 1;
    }
    
    return 0;
}

/******************************************************************************/

void Glow_WaitEvent(struct Glow_Window *win, struct Glow_Event *out_event){
    
    SDL_Event ev;
    (void)win;
    
    do{
        SDL_WaitEvent(&ev);
    }while(!glow_convert_sdl2_event(&ev, out_event));
}

/******************************************************************************/

unsigned Glow_ContextStructSize(void){
    return sizeof(struct Glow_Context);
}

/******************************************************************************/

int Glow_CreateContext(struct Glow_Window *win,
    struct Glow_Context *opt_share,
    unsigned major, unsigned minor,
    struct Glow_Context *out){
    
    if(opt_share != NULL){
        SDL_GL_MakeCurrent(win->win, opt_share->ctx);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }
    else{
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    out->ctx = SDL_GL_CreateContext(win->win);
    out->win = win->win;
    return 0;
}

/******************************************************************************/

void Glow_CreateLegacyContext(struct Glow_Window *win,
    struct Glow_Context *out){
    
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    win->ctx.ctx = SDL_GL_CreateContext(win->win);
    if(out)
        out->ctx = win->ctx.ctx;
}

/******************************************************************************/

void Glow_MakeCurrent(struct Glow_Context *ctx){
    SDL_GL_MakeCurrent(ctx->win, ctx->win);
}

/******************************************************************************/

struct Glow_Window *Glow_CreateLegacyWindow(
    unsigned w, unsigned h, const char *title){
    
    struct Glow_Window *const win = malloc(sizeof(struct Glow_Window));
    Glow_CreateWindow(win, w, h, title, 0);
    Glow_CreateLegacyContext(win, NULL);
    return win;
}
