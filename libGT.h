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
 * Depile et traite tous les événements système Windows en attente (non bloquant).
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
 * IMPLÉMENTATION NATIVE WIN32
 * ========================================================================= */
#ifdef LIBGT_IMPLEMENTATION

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

// Définition interne de la structure MyWindow
struct MyWindow {
    HWND hwnd;              // Handle (identifiant unique) de la fenêtre fourni par l'OS
    HINSTANCE hInstance;    // Pointeur vers l'instance de l'exécutable en mémoire
    int width;              // Largeur de la zone cliente (zone d'affichage utile)
    int height;             // Hauteur de la zone cliente
    bool should_close;      // Drapeau indiquant si la fenêtre a reçu un ordre de fermeture

    uint32_t* buffer;       // Tampon 1D de pixels en RAM (format ARGB 32-bit, taille = width * height)
    BITMAPINFO bmi;         // Metadonnées décrivant la structure du buffer pour Win32 GDI
};

/**
 * Procédure de fenêtre (Callback interne exécuté par le noyau Windows)
 * Traite les événements bas niveau envoyés par l'OS à notre fenêtre.
 */
static LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Récupération du pointeur MyWindow lié à cet HWND (stocké via SetWindowLongPtr)
    MyWindow* win = (MyWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CLOSE:
            // L'utilisateur a cliqué sur la croix ou fait Alt+F4
            if (win) win->should_close = true;
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            // La fenêtre est totalement détruite, on signale la fin de la boucle d'événements
            PostQuitMessage(0);
            break;

        default:
            // Déléguer les centaines d'autres messages système à la procédure par défaut
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

MyWindow* createWin(const char* title, int width, int height) {
    // 1. Allocation de la structure englobante en RAM
    MyWindow* win = (MyWindow*)malloc(sizeof(MyWindow));
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
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // Curseur pointeur standard

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
    }

    // Libération de la mémoire RAM allocation
    if (window->buffer) {
        free(window->buffer);
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
        TranslateMessage(&msg); // Traduction des codes de touches clavier bruts en caractères ASCII
        DispatchMessageA(&msg); // Envoie du message à notre MyWndProc
    }

    return !window->should_close;
}

void updateWin(MyWindow* window) {
    if (!window || !window->hwnd || !window->buffer) return;

    // Récupération du Device Context (contexte de dessin de la fenêtre)
    HDC hdc = GetDC(window->hwnd);

    // Blitting (copie mémoire RAM vers VRAM/Écran)
    StretchDIBits(
        hdc,
        0, 0, window->width, window->height, // Zone de destination dans la fenêtre
        0, 0, window->width, window->height, // Zone source dans le buffer
        window->buffer,
        &window->bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );

    // Libération impérative du Device Context pour éviter les fuites de ressources Win32
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

    // CLIPPING: Évite d'écrire hors mémoire si (x,y) est en dehors de la surface cliente
    if (x < 0 || x >= window->width || y < 0 || y >= window->height) return;

    // Conversion des coordonnées 2D (x, y) en un index 1D linéaire : index = (y * largeur) + x
    window->buffer[y * window->width + x] = color;
}

void drawRect(MyWindow* window, int x, int y, int w, int h, uint32_t color) {
    if (!window || !window->buffer) return;

    // Double boucle de remplissage ligne par ligne
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            drawPixel(window, col, row, color);
        }
    }
}

#endif // LIBGT_IMPLEMENTATION
#endif // LIBGT_H
