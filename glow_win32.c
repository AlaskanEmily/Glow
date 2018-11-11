/* Copyright (C) 2017-2018 Alaskan Emily, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "glow.h"

#include <Windows.h>
#include <gl/gl.h>

#include <assert.h>
#include <stdlib.h>

/******************************************************************************/

struct Glow_Context{
    HDC dc;
    HGLRC ctx;
    unsigned gl_mag, gl_min;
};

/******************************************************************************/

struct Glow_Window {
    HDC dc;
    HWND win;
    struct Glow_Context ctx;
};

/******************************************************************************/

#ifdef __GNUC__
HINSTANCE __mingw_winmain_hInstance;
LPWSTR __mingw_winmain_lpCmdLine;
DWORD __mingw_winmain_nShowCmd;
extern char **__argv;
extern int __argc;

#ifdef WinMain
#undef WinMain
#endif

#endif

/******************************************************************************/

#define GLOW_CLASS_NAME "GlowWindow"

/******************************************************************************/

static HINSTANCE glow_app = NULL;

/******************************************************************************/

static void glow_translate_local_mouse_pos(const POINT *in_pnt,
    struct Glow_Window *w, glow_pixel_coords_t out_pos){
    RECT rect;
    POINT pnt;
    pnt.x = in_pnt->x - 4;
    pnt.y = in_pnt->y - (GetSystemMetrics(SM_CYFRAME) +
        GetSystemMetrics(SM_CYCAPTION) +
        GetSystemMetrics(SM_CXPADDEDBORDER));

    GetWindowRect(w->win, &rect);

    out_pos[0] = (unsigned short)(pnt.x - rect.left);
    out_pos[1] = (unsigned short)(pnt.y - rect.top);

    if(!PtInRect(&rect, pnt)){
        if(pnt.x < rect.left)
            out_pos[0] = 0;
        else if(pnt.x > rect.right)
            out_pos[0] = (unsigned short)(rect.right - rect.left);
        
        if(pnt.y < rect.top)
            out_pos[1] = 0;
        else if(pnt.y > rect.bottom)
            out_pos[1] = (unsigned short)(rect.bottom - rect.top);
    }
}

/******************************************************************************/

static char glow_get_key_char(unsigned in){
    if(in <= 0x5A && in >= 0x41)
        return (in - 0x41) + 'a';
    if(in <= 0x39 && in >= 0x30)
        return (in - 0x30) + '0';
    if(in == VK_SPACE)
        return ' ';
    if(in == VK_OEM_2)
        return '/';
    if(in == VK_OEM_3)
        return '~';
    if(in == VK_OEM_4)
        return '[';
    if(in == VK_OEM_5 || in == VK_OEM_102)
        return '\\';
    if(in == VK_OEM_6)
        return ']';
    if(in == VK_OEM_7)
        return '\'';
        
    return '\0';
}

/*****************************************************************************/

static const char *glow_get_key_string(unsigned in, unsigned *len){
#define GLOW_IN_VK(N, VAL) case N: len[0] = sizeof(VAL) - 1; assert(sizeof(VAL) < 16); return VAL
#define GLOW_OEM_VK(N) case VK_OEM_ ## N: len[0] = sizeof("OEM_" #N ) - 1; return "OEM_" #N
    switch(in){
        GLOW_IN_VK(VK_ESCAPE, GLOW_ESCAPE);
        case VK_LSHIFT: case VK_RSHIFT:
        GLOW_IN_VK(VK_SHIFT, GLOW_SHIFT);
        case VK_LCONTROL: case VK_RCONTROL:
        GLOW_IN_VK(VK_CONTROL, GLOW_CONTROL);
        GLOW_IN_VK(VK_BACK, GLOW_BACKSPACE);
        GLOW_IN_VK(VK_RETURN, GLOW_ENTER);
        GLOW_IN_VK(VK_TAB, GLOW_TAB);
        GLOW_IN_VK(VK_CLEAR, "clear");
        GLOW_IN_VK(VK_LEFT, GLOW_LEFT_ARROW);
        GLOW_IN_VK(VK_DELETE, GLOW_DELETE);
        GLOW_IN_VK(VK_UP, GLOW_UP_ARROW);
        GLOW_IN_VK(VK_DOWN, GLOW_DOWN_ARROW);
        GLOW_IN_VK(VK_RIGHT, GLOW_RIGHT_ARROW);
        GLOW_OEM_VK(1);
        GLOW_OEM_VK(2);
        GLOW_OEM_VK(3);
        GLOW_OEM_VK(4);
        GLOW_OEM_VK(5);
        GLOW_OEM_VK(6);
        GLOW_OEM_VK(7);
        GLOW_OEM_VK(8);
        GLOW_OEM_VK(102);
        GLOW_IN_VK(VK_NONAME, "NONAME");
    #undef GLOW_IN_VK
        default: return NULL;
    }
}

/*****************************************************************************/

static const PIXELFORMATDESCRIPTOR glow_pixel_format = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1, /* Version, always 1 */
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32, /* Color Depth */
    0, 0, 0, 0, 0, 0, /* Individual color depths and shifts */
    0,
    0,
    0,
    0, 0, 0, 0,
    16, /* Depth buffer size */
    8, /* Stencil buffer size */
    0,
    PFD_MAIN_PLANE,
    0,
    0, 0, 0
};

/******************************************************************************/

static LRESULT WINAPI glow_window_proc(HWND wnd, UINT msg, WPARAM parm, LPARAM lparam){
    if(msg == WM_CREATE){
        struct Glow_Window *const window = (struct Glow_Window *)(((CREATESTRUCT*)lparam)->lpCreateParams);
        const int let = ChoosePixelFormat((window->dc = GetDC(wnd)), &glow_pixel_format);
        SetPixelFormat(window->dc, let, &glow_pixel_format);
        window->ctx.ctx = wglCreateContext(window->dc);
        window->ctx.dc = window->dc;
        wglMakeCurrent(window->dc, window->ctx.ctx);
        glClearColor(0.75f, 0.333f, 0.0f, 1.0f);
        return 0;
    }
    else if(msg == WM_SHOWWINDOW){
        if(parm == FALSE)
            PostQuitMessage(EXIT_SUCCESS);
        else
            ShowWindow(wnd, SW_SHOWNORMAL);
        return 0;
    }
    else if(msg == WM_CLOSE || msg == WM_DESTROY){
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    }
    else{
        return DefWindowProc(wnd, msg, parm, lparam);
    }
}

/******************************************************************************/

BOOL WINAPI DllMain(HINSTANCE app, DWORD reason, LPVOID reserved){
    const HICON icon = LoadIcon(app, MAKEINTRESOURCE(101));
	const HANDLE cursor = LoadCursor(NULL, IDC_ARROW);
    WNDCLASS wc = {
        CS_OWNDC,
        glow_window_proc,
        0,
        0,
        0,
        icon,
        cursor,
        (HBRUSH)(COLOR_BACKGROUND),
        NULL,
        GLOW_CLASS_NAME
    };

    wc.hInstance = glow_app = app;

    RegisterClass(&wc);
}

/******************************************************************************/

void Glow_ViewportSize(unsigned w, unsigned h,
    unsigned *out_w, unsigned *out_h){
    
    out_w[0] = w;
    out_h[0] = h;
    
    return;
    
    RECT size;
    
    size.left = 0;
    size.top = 0;
    size.right = w;
    size.bottom = h;
    
    AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, FALSE);

    size.bottom += (GetSystemMetrics(SM_CYFRAME) +
        GetSystemMetrics(SM_CYCAPTION) +
        GetSystemMetrics(SM_CXPADDEDBORDER));
    
    {
        const DWORD w_thickness = (size.right - w), h_thickness = (size.bottom - h);
        
        printf("w_thickness = %i, h_thickness = %i\n", w_thickness, h_thickness);
        
        out_w[0] = w + w_thickness;
        out_h[0] = h + h_thickness;
    }
}

/******************************************************************************/

void Glow_CreateWindow(struct Glow_Window *out,
    unsigned w, unsigned h, const char *title, int flags){
    
    if(glow_app == NULL){
        const HINSTANCE app = glow_app = GetModuleHandle(NULL);
        const HICON icon = LoadIcon(app, MAKEINTRESOURCE(101));
		const HANDLE cursor = LoadCursor(NULL, IDC_ARROW);
        WNDCLASS wc = {
            CS_OWNDC,
            glow_window_proc,
            0,
            0,
            0,
            icon,
            cursor,
            (HBRUSH)(COLOR_BACKGROUND),
            NULL,
            GLOW_CLASS_NAME
        };
        
        RegisterClass(&wc);
    }

    {
        RECT size;
        
        size.left = 0;
        size.top = 0;
        size.right = w;
        size.bottom = h;
        AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, TRUE);
        size.bottom += (GetSystemMetrics(SM_CYFRAME) +
            GetSystemMetrics(SM_CYCAPTION) +
            GetSystemMetrics(SM_CXPADDEDBORDER));
        out->win = CreateWindow(GLOW_CLASS_NAME, title, WS_OVERLAPPEDWINDOW, 64, 64, size.right, size.bottom, NULL, NULL, glow_app, out);
    }
}

/******************************************************************************/

unsigned Glow_WindowStructSize(){
    return sizeof(struct Glow_Window);
}

/******************************************************************************/

void Glow_DestroyWindow(struct Glow_Window *w){
    wglDeleteContext(w->ctx.ctx);
    DestroyWindow(w->win);
}

/******************************************************************************/

void Glow_SetTitle(struct Glow_Window *w, const char *title){
    SetWindowText(w->win, title);
}

/******************************************************************************/

void Glow_ShowWindow(struct Glow_Window *w){
    ShowWindow(w->win, SW_SHOWNORMAL);
}

/******************************************************************************/

void Glow_HideWindow(struct Glow_Window *w){
    ShowWindowAsync(w->win, SW_HIDE);
}

/******************************************************************************/

void Glow_FlipScreen(struct Glow_Window *w){
    wglMakeCurrent(w->dc, w->ctx.ctx);
    glFinish();
    wglSwapLayerBuffers(w->dc, WGL_SWAP_MAIN_PLANE);
    glClear(GL_COLOR_BUFFER_BIT);
}

/******************************************************************************/

void Glow_GetWindowSize(const struct Glow_Window *window,
    unsigned *out_w, unsigned *out_h){
    RECT rect;
    GetWindowRect(window->win, &rect);
    *out_w = rect.right - rect.left;
    *out_h = rect.bottom - rect.top;
}

/******************************************************************************/

static BOOL glow_translate_event(const MSG *msg, struct Glow_Window *window,
    struct Glow_Event *out_event){
    BOOL pressed = FALSE;
    switch(msg->message){
        case WM_KEYDOWN:
            pressed = TRUE;
            /* FALLTHROUGH */
        case WM_KEYUP:
            out_event->type = pressed ?
                eGlowKeyboardPressed : eGlowKeyboardReleased;
            {
                const char c = glow_get_key_char(msg->wParam),
                    *c_str;
                unsigned len;
                if(c){
                    out_event->value.key[0] = c;
                    out_event->value.key[1] = '\0';
                    return TRUE;
                }
                else if ((c_str = glow_get_key_string(msg->wParam, &len))){
                    assert(len + 1 < GLOW_MAX_KEY_NAME_SIZE);
                    memcpy(out_event->value.key, c_str, len);
                    out_event->value.key[len] = '\0';
                    return TRUE;
                }
                else /* Drop input */
                    return FALSE;
            }
        case WM_LBUTTONDOWN: 
        case WM_RBUTTONDOWN:
            pressed = TRUE;
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
            glow_translate_local_mouse_pos(&msg->pt,
                window, out_event->value.mouse.xy);
                
            out_event->value.mouse.button =
                (msg->message == WM_LBUTTONUP ||
                    msg->message == WM_LBUTTONDOWN) ?
                eGlowLeft : eGlowRight;
            out_event->type = pressed ?
                eGlowMousePressed : eGlowMouseReleased;
            return TRUE;
        case WM_DESTROY:
        case WM_CLOSE:
        case WM_QUIT:
            out_event->type = eGlowQuit;
            return TRUE;
        default:
            return FALSE;
    }
}

/******************************************************************************/

unsigned Glow_GetEvent(struct Glow_Window *window,
    struct Glow_Event *out_event){
    MSG msg;
    do{
        if(!PeekMessage(&msg, window->win, 0, 0, PM_REMOVE))
            return 0;
        DispatchMessage(&msg);
    }while(!glow_translate_event(&msg, window, out_event));

    return 1;
}

/******************************************************************************/

void Glow_WaitEvent(struct Glow_Window *window,
    struct Glow_Event *out_event){
    MSG msg;
    do{
        GetMessage(&msg, window->win, 0, 0);
    }while(!glow_translate_event(&msg, window, out_event));
}

/******************************************************************************/

unsigned Glow_ContextStructSize(){
    return sizeof(struct Glow_Context);
}

/******************************************************************************/

struct Glow_Context *Glow_GetContext(
    struct Glow_Window *window){
    return &window->ctx;
}

/******************************************************************************/

void Glow_MakeCurrent(struct Glow_Context *ctx){
    wglMakeCurrent(ctx->dc, ctx->ctx);
}

/******************************************************************************/

int Glow_CreateContext(struct Glow_Window *window,
    struct Glow_Context *opt_share,
    unsigned major, unsigned minor,
    struct Glow_Context *out){

    (void)opt_share;
    (void)major;
    (void)minor;
    (void)out;

    /* TODO!!! */
    out->dc = window->ctx.dc;
    out->ctx = window->ctx.ctx;
    return 0;
}

/******************************************************************************/

void Glow_CreateLegacyContext(struct Glow_Window *window,
    struct Glow_Context *out){
    out->dc = window->ctx.dc;
    out->ctx = window->ctx.ctx;
}

/******************************************************************************/

struct Glow_Window *Glow_CreateLegacyWindow(unsigned w, unsigned h,
    const char *title) {

    /* Put the window and CTX in one location so that free() will get them both. */
    struct Glow_Window *const window =
        malloc(Glow_WindowStructSize() + Glow_ContextStructSize());
    struct Glow_Context *const ctx = (struct Glow_Context *)(window + 1);
    Glow_CreateWindow(window, w, h, title, 0);
    Glow_CreateContext(window, NULL, 2, 1, ctx);
    Glow_MakeCurrent(ctx);
    return window;
}
