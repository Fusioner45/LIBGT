#ifndef LIBGT_H
#define LIBGT_H

#include <stdbool.h>

// Déclaration de l'API publique
typedef struct MyWindow MyWindow;

MyWindow* createWin(const char* title, int width, int height);
void destroyWin(MyWindow* window);
bool eventsWin(MyWindow* window);

#ifdef LIBGT_IMPLEMENTATION

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

struct MyWindow {
    HWND hwnd;
    HINSTANCE hInstance;
    int width;
    int height;
    bool should_close;
};

// Fonction de callback des messages Windows (WndProc)
static LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MyWindow* win = (MyWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CLOSE:
            if (win) win->should_close = true;
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

MyWindow* createWin(const char* title, int width, int height) {
    MyWindow* win = (MyWindow*)malloc(sizeof(MyWindow));
    if (!win) return NULL;

    win->width = width;
    win->height = height;
    win->should_close = false;
    win->hInstance = GetModuleHandle(NULL);

    // 1. Enregistrement de la classe de fenêtre
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = MyWndProc;
    wc.hInstance     = win->hInstance;
    wc.lpszClassName = "LIBGTWindowClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);
        RECT rect = {0, 0, width, height};

    //permet de bien mettre la cadre pour qu'il fasse la bonne taille
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int win_width = rect.right - rect.left;
    int win_height = rect.bottom - rect.top;


    // 2. Création de la fenêtre
   win->hwnd = CreateWindowExA(
    0, "LIBGTWindowClass", title,
    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, win_width, win_height, // <--- Utiliser les dimensions ajustées
    NULL, NULL, win->hInstance, NULL
    );
    
    if (!win->hwnd) {
        free(win);
        return NULL;
    }

    // Associer la structure C au HWND de Windows
    SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);

    return win;
}

void destroyWin(MyWindow* window) {
    if (window) {
        if (window->hwnd) {
            DestroyWindow(window->hwnd);
        }
        free(window);
    }
}

bool eventsWin(MyWindow* window) {
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            window->should_close = true;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !window->should_close;
}

#endif // libGT_IMPLEMENTATION
#endif // libGT_H