#ifndef LIBGT_H
#define LIBGT_H

#include <stdbool.h>
#include <stdint.h>

/* =========================================================================
 * API PUBLIQUE (Déclarations)
 * ========================================================================= */

// Structure opaque représentant l'état de la fenêtre et son tampon d'affichage
typedef struct MyWindow MyWindow;

/**
 * Crée et affiche une fenêtre native avec son tampon de pixels (framebuffer).
 * @param title  Titre de la fenêtre dans la barre de tâches/titre.
 * @param width  Largeur souhaitée pour la zone d'affichage (en pixels).
 * @param height Hauteur souhaitée pour la zone d'affichage (en pixels).
 * @return Pointeur vers MyWindow si succès, NULL en cas d'échec d'allocation ou Win32.
 */
MyWindow* createWin(const char* title, int width, int height);

/**
 * Libère proprement toutes les ressources système et mémoire (HWND, buffer, structure).
 * @param window Pointeur vers la structure de la fenêtre à détruire.
 */
void destroyWin(MyWindow* window);

/**
 * Dépile et traite tous les événements système Windows en attente (non bloquant).
 * @param window Pointeur vers la fenêtre.
 * @return true si la fenêtre doit rester ouverte, false si la fermeture a été demandée.
 */
bool eventsWin(MyWindow* window);

/**
 * Envoie le tampon de pixels de la RAM vers l'écran via l'API Win32 GDI.
 * @param window Pointeur vers la fenêtre.
 */
void updateWin(MyWindow* window);

/**
 * Réinitialise l'ensemble des pixels de la fenêtre avec une couleur unie.
 * @param window Pointeur vers la fenêtre.
 * @param color  Couleur au format hexadécimal ARGB (ex: 0x00RRGGBB).
 */
void clearWin(MyWindow* window, uint32_t color);

/**
 * Dessine un pixel unique avec contrôle de débordement (clipping).
 * @param window Pointeur vers la fenêtre.
 * @param x      Coordonnée X à partir du bord gauche (0).
 * @param y      Coordonnée Y à partir du bord supérieur (0).
 * @param color  Couleur au format hexadécimal ARGB.
 */
void drawPixel(MyWindow* window, int x, int y, uint32_t color);

/**
 * Dessine un rectangle plein dans le tampon de pixels.
 * @param window Pointeur vers la fenêtre.
 * @param x      Coordonnée X du coin supérieur gauche.
 * @param y      Coordonnée Y du coin supérieur gauche.
 * @param w      Largeur du rectangle en pixels.
 * @param h      Hauteur du rectangle en pixels.
 * @param color  Couleur au format hexadécimal ARGB.
 */
void drawRect(MyWindow* window, int x, int y, int w, int h, uint32_t color);

/* =========================================================================
 * COULEURS DE BASE (Format 0x00RRGGBB)
 * ========================================================================= */
#define GT_BLACK     0x00000000
#define GT_WHITE     0x00FFFFFF
#define GT_RED       0x00F43F5E
#define GT_GREEN     0x0022C55E
#define GT_BLUE      0x003B82F6
#define GT_YELLOW    0x00EAB308
#define GT_DARKGRAY  0x000F172A

/* =========================================================================
 * GESTION DES ENTRÉES (Clavier & Souris)
 * ========================================================================= */

// Codes des boutons de la souris
#define GT_MOUSE_BUTTON_LEFT   0
#define GT_MOUSE_BUTTON_RIGHT  1
#define GT_MOUSE_BUTTON_MIDDLE 2

// Raccourcis pour les touches virtuelles Windows courants (VK_*)
#define GT_KEY_SPACE     0x20
#define GT_KEY_LEFT      0x25
#define GT_KEY_UP        0x26
#define GT_KEY_RIGHT     0x27
#define GT_KEY_DOWN      0x28
#define GT_KEY_A         'A'
#define GT_KEY_D         'D'
#define GT_KEY_S         'S'
#define GT_KEY_W         'W'
#define GT_KEY_ESCAPE    0x1B

/**
 * Vérifie si une touche du clavier est actuellement enfoncée.
 * @param window Pointeur vers la fenêtre.
 * @param keycode Code de la touche (ex: GT_KEY_SPACE ou 'A').
 */
bool isKeyDown(MyWindow* window, int keycode);

/**
 * Vérifie si un bouton de la souris est actuellement enfoncé.
 * @param window Pointeur vers la fenêtre.
 * @param button GT_MOUSE_BUTTON_LEFT, GT_MOUSE_BUTTON_RIGHT ou GT_MOUSE_BUTTON_MIDDLE.
 */
bool isMouseButtonDown(MyWindow* window, int button);

/**
 * Récupère la position actuelle du curseur relative à la surface cliente de la fenêtre.
 * @param window Pointeur vers la fenêtre.
 * @param out_x Pointeur de sortie pour X (peut être NULL).
 * @param out_y Pointeur de sortie pour Y (peut être NULL).
 */
void getMousePos(MyWindow* window, int* out_x, int* out_y);


/* =========================================================================
 * IMPLÉMENTATION NATIVE WIN32
 * ========================================================================= */
#ifdef LIBGT_IMPLEMENTATION

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Définition interne de la structure MyWindow
struct MyWindow {
    // --- BASE FENÊTRE ---
    HWND hwnd;              // Handle (identifiant unique) de la fenêtre fourni par l'OS
    HINSTANCE hInstance;    // Pointeur vers l'instance de l'exécutable en mémoire
    int width;              // Largeur de la zone cliente (zone d'affichage utile)
    int height;             // Hauteur de la zone cliente
    bool should_close;      // Drapeau indiquant si la fenêtre a reçu un ordre de fermeture

    // --- DESSIN ---
    uint32_t* buffer;       // Tampon 1D de pixels en RAM (format ARGB 32-bit, taille = width * height)
    BITMAPINFO bmi;         // Métadonnées décrivant la structure du buffer pour Win32 GDI

    // --- ÉTAT DES ENTRÉES ---
    bool keys[256];         // État de 256 touches virtuelles Windows
    bool mouse_buttons[3];  // 0: Gauche, 1: Droit, 2: Milieu
    int mouse_x;            // Position X du curseur dans la fenêtre
    int mouse_y;            // Position Y du curseur dans la fenêtre
};

/**
 * Procédure de fenêtre (Callback interne exécuté par le noyau Windows)
 * Traite les événements bas niveau envoyés par l'OS à notre fenêtre.
 */
static LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Récupération du pointeur MyWindow lié à cet HWND (stocké via SetWindowLongPtr)
    MyWindow* win = (MyWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        // --- CLAVIER ---
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (win && wParam < 256) {
                win->keys[wParam] = true;
            }
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (win && wParam < 256) {
                win->keys[wParam] = false;
            }
            break;

        // --- SOURIS (Boutons) ---
        case WM_LBUTTONDOWN:
            if (win) {
                win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = true;
                SetCapture(hwnd); // Capture pour garder le contrôle hors fenêtre
            }
            break;
        case WM_LBUTTONUP:
            if (win) {
                win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = false;
                ReleaseCapture();
            }
            break;

        case WM_RBUTTONDOWN:
            if (win) win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = true;
            break;
        case WM_RBUTTONUP:
            if (win) win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = false;
            break;

        case WM_MBUTTONDOWN:
            if (win) win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = true;
            break;
        case WM_MBUTTONUP:
            if (win) win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = false;
            break;

        // --- SOURIS (Mouvement) ---
        case WM_MOUSEMOVE:
            if (win) {
                win->mouse_x = (int)(short)LOWORD(lParam);
                win->mouse_y = (int)(short)HIWORD(lParam);
            }
            break;

        // --- PERTE DU FOCUS (Réinitialisation des touches / boutons) ---
        case WM_KILLFOCUS:
            if (win) {
                memset(win->keys, 0, sizeof(win->keys));
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons));
            }
            break;

        // --- FERMETURE ---
        case WM_CLOSE:
            if (win) win->should_close = true;
            // Ne pas appeler DestroyWindow ici pour éviter une double destruction dans destroyWin
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
    // 1. Allocation de la structure englobante en RAM
    MyWindow* win = (MyWindow*)calloc(1, sizeof(MyWindow));
    if (!win) {
        fprintf(stderr, "[LIBGT ERROR] Échec d'allocation mémoire pour MyWindow.\n");
        return NULL;
    }

    win->width = width;
    win->height = height;
    win->should_close = false;
    win->hInstance = GetModuleHandle(NULL);

    // 2. Configuration et enregistrement du modèle de fenêtre (WNDCLASS) auprès de l'OS
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = MyWndProc;
    wc.hInstance     = win->hInstance;
    wc.lpszClassName = "LIBGTWindowClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        // Ignoré si la classe a déjà été enregistrée par un appel précédent
    }

    // 3. Ajustement de la taille de fenêtre globale pour obtenir la bonne surface cliente
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int win_width = rect.right - rect.left;
    int win_height = rect.bottom - rect.top;

    // 4. Instanciation physique de la fenêtre auprès du sous-système Win32
    win->hwnd = CreateWindowExA(
        0, 
        "LIBGTWindowClass", 
        title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        win_width, win_height,
        NULL, NULL, win->hInstance, NULL
    );

    if (!win->hwnd) {
        fprintf(stderr, "[LIBGT ERROR] Échec de CreateWindowExA.\n");
        free(win);
        return NULL;
    }

    // 5. Association du pointeur C `win` avec le `HWND` Windows (nécessaire pour la WndProc)
    SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);

    // 6. Allocation du framebuffer (4 octets par pixel : Alpha, Rouge, Vert, Bleu)
    win->buffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if (!win->buffer) {
        fprintf(stderr, "[LIBGT ERROR] Échec d'allocation du tampon de pixels (RAM).\n");
        DestroyWindow(win->hwnd);
        free(win);
        return NULL;
    }

    // 7. Initialisation des métadonnées BITMAPINFO pour le rendu via StretchDIBits
    win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win->bmi.bmiHeader.biWidth = width;
    win->bmi.bmiHeader.biHeight = -height; // Hauteur négative pour forcer l'origine (0,0) en haut à gauche
    win->bmi.bmiHeader.biPlanes = 1;
    win->bmi.bmiHeader.biBitCount = 32;     // 32 bits = 8 bits x 4 canaux
    win->bmi.bmiHeader.biCompression = BI_RGB;

    return win;
}

void destroyWin(MyWindow* window) {
    if (!window) return;

    // Nettoyage Win32
    if (window->hwnd) {
        DestroyWindow(window->hwnd);
        window->hwnd = NULL;
    }

    // Libération de la mémoire RAM
    if (window->buffer) {
        free(window->buffer);
        window->buffer = NULL;
    }

    free(window);
}

bool eventsWin(MyWindow* window) {
    if (!window) return false;

    MSG msg;
    // Dépilement non bloquant de tous les messages en attente dans la file du thread
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            window->should_close = true;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return !window->should_close;
}

void updateWin(MyWindow* window) {
    if (!window || !window->hwnd || !window->buffer) return;

    HDC hdc = GetDC(window->hwnd);

    StretchDIBits(
        hdc,
        0, 0, window->width, window->height,
        0, 0, window->width, window->height,
        window->buffer,
        &window->bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );

    ReleaseDC(window->hwnd, hdc);
}

void clearWin(MyWindow* window, uint32_t color) {
    if (!window || !window->buffer) return;

    int total_pixels = window->width * window->height;
    for (int i = 0; i < total_pixels; i++) {
        window->buffer[i] = color;
    }
}

void drawPixel(MyWindow* window, int x, int y, uint32_t color) {
    if (!window || !window->buffer) return;

    if (x < 0 || x >= window->width || y < 0 || y >= window->height) return;

    window->buffer[y * window->width + x] = color;
}

// Version optimisée de drawRect avec clipping global
void drawRect(MyWindow* window, int x, int y, int w, int h, uint32_t color) {
    if (!window || !window->buffer || w <= 0 || h <= 0) return;

    // Clipping unique aux bornes de la fenêtre
    int x1 = (x < 0) ? 0 : x;
    int y1 = (y < 0) ? 0 : y;
    int x2 = (x + w > window->width) ? window->width : (x + w);
    int y2 = (y + h > window->height) ? window->height : (y + h);

    if (x1 >= x2 || y1 >= y2) return; // Hors écran

    int row_stride = window->width;
    for (int row = y1; row < y2; row++) {
        uint32_t* row_ptr = &window->buffer[row * row_stride + x1];
        int count = x2 - x1;
        for (int col = 0; col < count; col++) {
            row_ptr[col] = color;
        }
    }
}

bool isKeyDown(MyWindow* window, int keycode) {
    if (!window || keycode < 0 || keycode >= 256) return false;
    return window->keys[keycode];
}

bool isMouseButtonDown(MyWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return window->mouse_buttons[button];
}

void getMousePos(MyWindow* window, int* out_x, int* out_y) {
    if (!window) return;
    if (out_x) *out_x = window->mouse_x;
    if (out_y) *out_y = window->mouse_y;
}

#endif // LIBGT_IMPLEMENTATION
#endif // LIBGT_H
