#ifndef LIBGT_H
#define LIBGT_H

// Inclusions standard C
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // uint8_t, uint32_t, int64_t, etc.
#include <stddef.h>    // size_t, NULL
#include <math.h>      // sqrtf, sinf, cosf, etc.

/* =========================================================================
* LIBGT - Bibliothèque graphique 2D légère pour Windows (GDI / futur D2D)
* =========================================================================
* Philosophie :
*   - API simple, proche du matériel (framebuffer CPU)
*   - Header-only : il suffit d'inclure ce fichier et de définir LIBGT_IMPLEMENTATION
*     dans UN seul fichier .c pour obtenir l'implémentation
*   - Zéro dépendance externe (juste Windows SDK)
*   - Compatible C11 (static inline, types fixes, designated initializers)
*
* Utilisation typique :
*   #define LIBGT_IMPLEMENTATION
*   #include "libGT.h"
*   int main() {
*       GtWindow* win = gtCreateWindow("Mon jeu", 800, 600);
*       while (!gtShouldClose(win)) {
*           gtBeginFrame();              // Met à jour delta time
*           gtEventsWindow(win);         // Traite les messages Windows
*           // ... logique de jeu ...
*           gtClearWindow(win, GT_BLACK);
*           gtDrawRect(win, 100, 100, 50, 50, GT_RED);
*           gtUpdateWindow(win);         // Affiche le framebuffer
*       }
*       gtDestroyWindow(win);
*   }
* ========================================================================= */

/* =========================================================================
* API PUBLIQUE - GESTION DE FENÊTRE
* ========================================================================= */

// Structure opaque représentant la fenêtre et son tampon de pixels (framebuffer)
// Les champs internes ne sont accessibles que dans la section IMPLEMENTATION
typedef struct GtWindow GtWindow;

// Crée une fenêtre avec titre, largeur et hauteur en pixels
// Retourne NULL en cas d'erreur (mémoire, classe fenêtre, création HWND)
GtWindow* gtCreateWindow(const char* title, int width, int height);

// Détruit la fenêtre, libère le framebuffer et la classe fenêtre Win32
void      gtDestroyWindow(GtWindow* window);

// Traite la file de messages Windows (clavier, souris, resize, close)
// À appeler une fois par frame dans la boucle principale
// Retourne false si WM_QUIT reçu (fenêtre doit fermer)
bool      gtEventsWindow(GtWindow* window);

// Copie le framebuffer RAM vers l'écran (blitting GDI via StretchDIBits)
// À appeler APRÈS tous les dessins de la frame
void      gtUpdateWindow(GtWindow* window);

// Remplit tout le framebuffer avec une couleur unie (format 0xAARRGGBB)
void      gtClearWindow(GtWindow* window, uint32_t color);

// Vérifie si l'utilisateur a demandé la fermeture (bouton X, Alt+F4, etc.)
bool      gtShouldClose(GtWindow* window);

// Accesseurs dimensions zone cliente (utilisables pour ratio aspect, etc.)
int       gtGetWidth(GtWindow* window);
int       gtGetHeight(GtWindow* window);

/* =========================================================================
* API PUBLIQUE - PRIMITIVES DE DESSIN 2D (sur GtWindow directement)
* =========================================================================
* Ces fonctions dessinent directement dans le framebuffer de la fenêtre.
* Elles font du clipping automatique (ignorer les pixels hors écran).
* Coordonnées : origine (0,0) en HAUT-GAUCHE, X vers la droite, Y vers le bas.
* Couleur : format 0xAARRGGBB (alpha dans les 8 bits de poids fort).
* ========================================================================= */

void gtDrawPixel(GtWindow* window, int x, int y, uint32_t color);
void gtDrawRect(GtWindow* window, int x, int y, int w, int h, uint32_t color);
void gtDrawRectLines(GtWindow* window, int x, int y, int w, int h, uint32_t color);
void gtDrawLine(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color);
void gtDrawCircleLines(GtWindow* window, int cx, int cy, int radius, uint32_t color);
void gtDrawCircle(GtWindow* window, int cx, int cy, int radius, uint32_t color);

/* =========================================================================
* VECTEURS 2D (GtVec2)
* =========================================================================
* Structure simple pour positions, vitesses, directions en 2D.
* Toutes les opérations sont static inline → zéro overhead, inlinées par le compilateur.
* Utilisation :
*   GtVec2 pos = gtVec2(100.0f, 200.0f);
*   GtVec2 vel = gtVec2Mul(gtVec2Norm(dir), speed * dt);
*   pos = gtVec2Add(pos, vel);
* ========================================================================= */
typedef struct GtVec2 {
    float x, y;   // Composantes du vecteur
} GtVec2;

// Constructeur : crée un vecteur depuis deux floats
static inline GtVec2 gtVec2(float x, float y) {
    GtVec2 v = {x, y};
    return v;
}

// Addition vectorielle : a + b
static inline GtVec2 gtVec2Add(GtVec2 a, GtVec2 b) { return gtVec2(a.x + b.x, a.y + b.y); }
// Soustraction vectorielle : a - b
static inline GtVec2 gtVec2Sub(GtVec2 a, GtVec2 b) { return gtVec2(a.x - b.x, a.y - b.y); }
// Multiplication par un scalaire : v * s
static inline GtVec2 gtVec2Mul(GtVec2 a, float s)  { return gtVec2(a.x * s, a.y * s); }
// Division par un scalaire : v / s
static inline GtVec2 gtVec2Div(GtVec2 a, float s)  { return gtVec2(a.x / s, a.y / s); }
// Produit scalaire (dot product) : a.x*b.x + a.y*b.y
// Utile pour : projection, angle entre vecteurs, test orthogonalité
static inline float  gtVec2Dot(GtVec2 a, GtVec2 b)  { return a.x * b.x + a.y * b.y; }
// Longueur au carré (évite sqrt) : ||v||²
// Préférer Len2() pour comparaisons de distance (ex: if (gtVec2Len2(a-b) < r*r))
static inline float  gtVec2Len2(GtVec2 v)          { return gtVec2Dot(v, v); }
// Longueur (norme euclidienne) : ||v|| = sqrt(x²+y²)
static inline float  gtVec2Len(GtVec2 v)           { return sqrtf(gtVec2Len2(v)); }
// Normalise le vecteur (longueur = 1), gère le cas longueur nulle
// Retourne (0,0) si v est nul pour éviter division par zéro
static inline GtVec2 gtVec2Norm(GtVec2 v) {
    float l = gtVec2Len(v);
    return (l > 0) ? gtVec2Div(v, l) : gtVec2(0, 0);
}
// Interpolation linéaire : a + t * (b - a), t dans [0,1]
// Utile pour : mouvement fluide, transitions, lerp de couleur
static inline GtVec2 gtVec2Lerp(GtVec2 a, GtVec2 b, float t) { return gtVec2Add(a, gtVec2Mul(gtVec2Sub(b, a), t)); }

/* =========================================================================
* COULEURS (GtColor + constructeurs RGBA)
* =========================================================================
* Format interne : 32 bits ARGB (Alpha-Red-Green-Blue) = 0xAARRGGBB
*   - Bits 31-24 : Alpha (0 = transparent, 255 = opaque)
*   - Bits 23-16 : Rouge
*   - Bits 15-8  : Vert
*   - Bits 7-0   : Bleu
* Ce format correspond au format natif GDI (BI_RGB, 32bpp) utilisé par StretchDIBits.
*
* Pourquoi des fonctions au lieu de #define ?
*   - gtColorRGBA/gtColorRGB sont type-safe (vérification uint8_t à la compilation)
*   - Évite les erreurs de décalage de bits manuels
*   - Les constantes GT_RED, etc. restent des #define pour compatibilité
* ========================================================================= */
typedef uint32_t GtColor;

// Construit une couleur depuis 4 composantes 0-255 (RGBA)
// Ordre des paramètres : R, G, B, A (alpha en dernier, défaut 255 si omis via gtColorRGB)
static inline GtColor gtColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// Constructeur RGB seul (alpha = 255 / opaque)
static inline GtColor gtColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return gtColorRGBA(r, g, b, 255);
}

/* Couleurs prédéfinies (remplacent les anciens #define GT_RED bruts) */
/* Format : 0xRRGGBB → gtColorRGB le convertit en 0xFFRRGGBB (opaque) */
#define GT_BLACK    gtColorRGB(0x00, 0x00, 0x00)
#define GT_WHITE    gtColorRGB(0xFF, 0xFF, 0xFF)
#define GT_RED      gtColorRGB(0xF4, 0x3F, 0x5E)  // Rose/rouge moderne (Tailwind red-500)
#define GT_GREEN    gtColorRGB(0x22, 0xC5, 0x5E)  // Vert moderne (Tailwind green-500)
#define GT_BLUE     gtColorRGB(0x3B, 0x82, 0xF6)  // Bleu moderne (Tailwind blue-500)
#define GT_YELLOW   gtColorRGB(0xEA, 0xB3, 0x08)  // Jaune moderne (Tailwind yellow-500)
#define GT_DARKGRAY gtColorRGB(0x0F, 0x17, 0x2A)  // Ardoise très foncée (Tailwind slate-900)

/* =========================================================================
* GESTION DES ENTRÉES (Clavier & Souris)
* =========================================================================
* Codes de boutons souris (indices dans window->mouse_buttons[])
* Correspondent aux messages Windows WM_LBUTTONDOWN, etc.
* ========================================================================= */
#define GT_MOUSE_BUTTON_LEFT   0   // Clic gauche
#define GT_MOUSE_BUTTON_RIGHT  1   // Clic droit
#define GT_MOUSE_BUTTON_MIDDLE 2   // Molette / clic milieu

// Virtual-Key codes Windows courants (VK_*)
// Voir : https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// Utilisation : if (gtIsKeyDown(win, GT_KEY_W)) ...
#define GT_KEY_SPACE  0x20         // VK_SPACE
#define GT_KEY_LEFT   0x25         // VK_LEFT  (flèche gauche)
#define GT_KEY_UP     0x26         // VK_UP    (flèche haut)
#define GT_KEY_RIGHT  0x27         // VK_RIGHT (flèche droite)
#define GT_KEY_DOWN   0x28         // VK_DOWN  (flèche bas)
#define GT_KEY_A      'A'          // Touche 'A' (code ASCII = VK_A)
#define GT_KEY_D      'D'          // Touche 'D'
#define GT_KEY_S      'S'          // Touche 'S'
#define GT_KEY_W      'W'          // Touche 'W'
#define GT_KEY_ESCAPE 0x1B         // VK_ESCAPE

// Vérifie si une touche est actuellement enfoncée (état maintenu, pas événement)
// keycode : un des GT_KEY_* ci-dessus ou n'importe quel code VK_* Windows
bool gtIsKeyDown(GtWindow* window, int keycode);

// Vérifie si un bouton souris est enfoncé
// button : GT_MOUSE_BUTTON_LEFT, RIGHT ou MIDDLE
bool gtIsMouseButtonDown(GtWindow* window, int button);

// Récupère la position actuelle de la souris (coordonnées client : 0,0 en haut-gauche)
// out_x, out_y : pointeurs vers int pour recevoir les coordonnées (peuvent être NULL)
void gtGetMousePos(GtWindow* window, int* out_x, int* out_y);

/* =========================================================================
* TEMPS (Delta time pour boucle de jeu)
* =========================================================================
* Implémentation basée sur QueryPerformanceCounter (QPC) :
*   - Précision nanoseconde (bien meilleure que timeGetTime ou GetTickCount)
*   - Fréquence fixe au boot (QueryPerformanceFrequency)
*   - Pas de dérive, monotone
*
* Utilisation standard dans la boucle de jeu :
*   gtBeginFrame();                    // 1. Au TOUT DÉBUT de la frame
*   float dt = gtGetDeltaTime();       // 2. Récupère dt en secondes
*   // ... logique avec dt (vitesse * dt, etc.)
*   gtEventsWindow(win);               // 3. Traite les événements
*   // ... rendu ...
*   gtUpdateWindow(win);               // 4. Affiche
*
* Pourquoi gtBeginFrame() séparé de gtGetDeltaTime() ?
*   - gtBeginFrame() fait la mesure (QPC now - lastFrame)
*   - gtGetDeltaTime() ne fait que lire la variable mise à jour
*   - Permet d'appeler gtGetDeltaTime() plusieurs fois par frame sans recalculer
*   - gtBeginFrame() doit être appelé UNE SEULE FOIS par frame
* ========================================================================= */

// Temps total écoulé depuis le premier appel à gtBeginFrame() (secondes, double précision)
// Utile pour : animations basées sur le temps absolu, timestamps, profiling
double gtGetTime(void);

// Delta time de la frame COURANTE (secondes, float)
// Mis à jour par gtBeginFrame() - ne pas appeler avant le premier gtBeginFrame()
float  gtGetDeltaTime(void);

// À appeler AU DÉBUT de chaque frame (avant logique + événements)
// Calcule : delta = (now - lastFrame) / frequency
// Met à jour g_gtLastFrame = now pour la frame suivante
void   gtBeginFrame(void);

/* =========================================================================
* RENDERER ABSTRACTION (Backend GDI + préparation D2D / Vulkan / etc.)
* =========================================================================
* Pourquoi une abstraction Renderer ?
*   - Découple l'API de dessin du backend (GDI aujourd'hui, D2D/Vulkan demain)
*   - Permet de changer de backend sans toucher au code du jeu
*   - Prépare l'architecture pour : anti-aliasing, batching, shaders, 3D
*   - GDI = fallback simple, CPU-only, compatible partout (WinXP+)
*   - D2D = GPU-accéléré, AA natif, texte qualité, mais plus complexe
*
* Architecture :
*   GtRenderer (opaque) contient :
*     - window : pointeur vers la fenêtre cible
*     - type   : GT_RENDERER_GDI ou GT_RENDERER_D2D
*     - hdc    : Device Context GDI (pour blitting final)
*     - (champs D2D réservés pour futur)
*
* Flux de rendu typique :
*   GtRenderer* r = gtCreateRenderer(win, GT_RENDERER_GDI);
*   while (!gtShouldClose(win)) {
*       gtBeginFrame();
*       gtEventsWindow(win);
*
*       gtRendererBegin(r);           // Prépare le frame (D2D: BeginDraw)
*       gtRendererClear(r, GT_BLACK); // Efface le framebuffer
*       gtRendererDrawRect(r, ...);   // Dessine via API Renderer
*       gtRendererDrawCircle(r, ...);
*       gtRendererEnd(r);             // Affiche (GDI: StretchDIBits, D2D: EndDraw+Present)
*   }
*   gtDestroyRenderer(r);
*
* Note : Les anciennes fonctions gtDraw* sur GtWindow restent disponibles
*        pour compatibilité et prototypage rapide.
* ========================================================================= */

// Type de backend de rendu
typedef enum {
    GT_RENDERER_GDI,   // GDI software (framebuffer CPU + StretchDIBits) - défaut, toujours dispo
    GT_RENDERER_D2D    // Direct2D (GPU, hardware-accéléré) - pas encore implémenté, fallback GDI
} GtRendererType;

// Structure opaque du renderer (détails dans section IMPLEMENTATION)
typedef struct GtRenderer GtRenderer;

// Crée un renderer pour une fenêtre donnée
// type : GT_RENDERER_GDI (recommandé) ou GT_RENDERER_D2D (fallback GDI si indisponible)
// Retourne NULL si échec (fenêtre invalide, GetDC échoue, OOM)
GtRenderer* gtCreateRenderer(GtWindow* window, GtRendererType type);

// Détruit le renderer et libère ses ressources (ReleaseDC pour GDI)
void        gtDestroyRenderer(GtRenderer* renderer);

// =========================================================================
// API DE DESSIN VIA RENDERER (remplace gtDraw* sur GtWindow)
// =========================================================================

// Début de frame : prépare le backend (GDI = nop, D2D = BeginDraw)
// À appeler AVANT tout dessin dans la frame
void gtRendererBegin(GtRenderer* renderer);

// Fin de frame : présente le résultat à l'écran
// GDI = StretchDIBits (blit framebuffer -> window)
// D2D = EndDraw() + Present()
void gtRendererEnd(GtRenderer* renderer);

// Efface le framebuffer avec une couleur (délègue à gtClearWindow)
void gtRendererClear(GtRenderer* renderer, GtColor color);

// Primitives de dessin (délèguent aux gtDraw* de GtWindow pour GDI)
// Pour D2D futur : appelleront ID2D1RenderTarget::DrawRectangle, etc.
void gtRendererDrawPixel(GtRenderer* renderer, int x, int y, GtColor color);
void gtRendererDrawRect(GtRenderer* renderer, int x, int y, int w, int h, GtColor color);
void gtRendererDrawRectLines(GtRenderer* renderer, int x, int y, int w, int h, GtColor color);
void gtRendererDrawLine(GtRenderer* renderer, int x1, int y1, int x2, int y2, GtColor color);
void gtRendererDrawCircle(GtRenderer* renderer, int cx, int cy, int radius, GtColor color);
void gtRendererDrawCircleLines(GtRenderer* renderer, int cx, int cy, int radius, GtColor color);

/* =========================================================================
* IMPLÉMENTATION NATIVE WIN32
* =========================================================================
* Cette section n'est compilée QUE si LIBGT_IMPLEMENTATION est défini
* (dans UN seul fichier .c du projet)
* ========================================================================= */
#ifdef LIBGT_IMPLEMENTATION

// API Windows
#include <windows.h>      // HWND, HDC, MSG, WNDCLASSA, etc.
#include <stdlib.h>       // malloc, free, calloc, realloc
#include <string.h>       // memset, memcpy
#include <math.h>         // sqrtf, sinf, cosf (pour gtVec2Len, cercles)

// Variable globale : évite d'enregistrer la classe fenêtre plusieurs fois
static bool g_gtWindowClassRegistered = false;

/* -------------------------------------------------------------------------
* STRUCTURE INTERNE DE FENÊTRE (GtWindow)
* -------------------------------------------------------------------------
* Tous les champs sont privés - l'API publique utilise des accesseurs.
* Le framebuffer est en RAM (CPU) : uint32_t* buffer [width * height]
* Format : 32bpp ARGB (0xAARRGGBB), top-down (biHeight négatif)
* ------------------------------------------------------------------------- */
struct GtWindow {
    HWND      hwnd;          // Handle Win32 de la fenêtre (identifiant unique)
    HINSTANCE hInstance;     // Instance du module (GetModuleHandle(NULL))
    int       width;         // Largeur zone cliente (pixels)
    int       height;        // Hauteur zone cliente (pixels)
    bool      should_close;  // Mis à true par WM_CLOSE / WM_QUIT

    uint32_t* buffer;        // Framebuffer 1D : buffer[y * width + x] = couleur ARGB
    BITMAPINFO bmi;          // Description du format bitmap pour StretchDIBits (GDI)

    bool keys[256];          // État clavier : true = enfoncée (index = code VK)
    bool mouse_buttons[3];   // État souris : [0]=gauche, [1]=droit, [2]=milieu
    int  mouse_x;            // Position X souris relative zone cliente
    int  mouse_y;            // Position Y souris relative zone cliente
    bool mouse_tracking;     // Suivi WM_MOUSELEAVE actif (TrackMouseEvent)
};

/* -------------------------------------------------------------------------
* RASTERISATION BAS NIVEAU (Accès direct mémoire, SANS clipping)
* -------------------------------------------------------------------------
* ATTENTION : L'appelant DOIT garantir que les coordonnées sont valides !
* Ces fonctions sont utilisées internement par les primitives avec clipping.
* ------------------------------------------------------------------------- */

// Écrit un pixel unique directement dans le buffer (sans vérification)
static inline void putPixelUnchecked(GtWindow* window, int x, int y, uint32_t color) {
    // Index 1D = y * stride + x  (stride = width pour framebuffer packé)
    window->buffer[(size_t)y * (size_t)window->width + (size_t)x] = color;
}

// Trace une ligne horizontale continue en mémoire (optimisée : pas de recalcul d'index)
static inline void rasterizeHLineUnchecked(GtWindow* window, int y, int x1, int x2, uint32_t color) {
    size_t row_start = (size_t)y * (size_t)window->width;  // Début de la ligne en mémoire
    for (int x = x1; x <= x2; x++) {
        window->buffer[row_start + (size_t)x] = color;
    }
}

// Trace une ligne verticale continue (saut de ligne par 'stride' = width)
static inline void rasterizeVLineUnchecked(GtWindow* window, int x, int y1, int y2, uint32_t color) {
    size_t stride = (size_t)window->width;
    size_t index = (size_t)y1 * stride + (size_t)x;  // Index du premier pixel
    for (int y = y1; y <= y2; y++) {
        window->buffer[index] = color;
        index += stride;  // Passe à la ligne suivante (y+1)
    }
}

// Algorithme de ligne de Bresenham (calculs 100% entiers, pas de float)
// Trace une ligne entre (x1,y1) et (x2,y2) pixel par pixel
static inline void rasterizeBresenhamUnchecked(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1);        // Distance horizontale
    int dy = abs(y2 - y1);        // Distance verticale
    int sx = (x1 < x2) ? 1 : -1;  // Pas en X (+1 ou -1)
    int sy = (y1 < y2) ? 1 : -1;  // Pas en Y (+1 ou -1)
    int err = dx - dy;            // Erreur accumulée (décision diagonale vs axis)

    while (1) {
        putPixelUnchecked(window, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;  // Arrivé au bout

        int64_t e2 = (int64_t)err * 2;    // *2 pour éviter divisions
        if (e2 > -dy) {                   // Décision : avancer en X
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {                    // Décision : avancer en Y
            err += dx;
            y1 += sy;
        }
    }
}

/* -------------------------------------------------------------------------
* CLIPPING DE SEGMENTS (Algorithme de Cohen-Sutherland)
* -------------------------------------------------------------------------
* Découpe un segment pour qu'il s'insère dans le rectangle [0, w-1] x [0, h-1].
* Renvoie false si le segment est totalement hors écran (rejet trivial).
* Utilise des codes de région 4 bits (OutCodes) : LEFT=1, RIGHT=2, BOTTOM=4, TOP=8.
* ------------------------------------------------------------------------- */
#define CS_INSIDE 0 // 0000 : Point à l'intérieur
#define CS_LEFT   1 // 0001 : À gauche de l'écran
#define CS_RIGHT  2 // 0010 : À droite
#define CS_BOTTOM 4 // 0100 : En bas (Y < 0)
#define CS_TOP    8 // 1000 : En haut (Y >= height)

// Calcule le code de région (OutCode) pour un point (x,y)
static int computeCSCode(int x, int y, int w, int h) {
    int code = CS_INSIDE;
    if (x < 0)       code |= CS_LEFT;
    else if (x >= w) code |= CS_RIGHT;
    if (y < 0)       code |= CS_BOTTOM;
    else if (y >= h) code |= CS_TOP;
    return code;
}

// Recadre le segment (x1,y1)-(x2,y2) aux bornes [0,w-1]x[0,h-1]
// Modifie les coordonnées sur place via pointeurs
// Retourne true si une partie du segment est visible, false si totalement hors champ
static bool clipLineSegment(int* x1, int* y1, int* x2, int* y2, int w, int h) {
    // Protection contre overflow int64_t dans les calculs d'interpolation
    // Rejeter les coordonnées extrêmes (> 1M pixels) qui causeraient dx*h > 2^63
    const int64_t MAX_COORD = 1000000;
    int64_t x1_64 = *x1, y1_64 = *y1;
    int64_t x2_64 = *x2, y2_64 = *y2;
    if (llabs(x1_64) > MAX_COORD || llabs(y1_64) > MAX_COORD ||
        llabs(x2_64) > MAX_COORD || llabs(y2_64) > MAX_COORD) {
        return false;  // Coordonnées hors limites raisonnables
    }

    int code1 = computeCSCode(*x1, *y1, w, h);
    int code2 = computeCSCode(*x2, *y2, w, h);
    bool accept = false;

    while (1) {
        if ((code1 | code2) == 0) {       // Les deux codes = 0 → totalement visible
            accept = true;
            break;
        }
        else if (code1 & code2) {         // Codes partagent un bit → totalement hors champ (même côté)
            break;
        }
        else {                            // Partiellement visible : clipper contre un bord
            int x = 0, y = 0;
            int code_out = code1 ? code1 : code2;  // Prend un point hors champ

            // Interpolation linéaire pour trouver intersection avec le bord
            int64_t dx = x2_64 - x1_64;
            int64_t dy = y2_64 - y1_64;

            if (code_out & CS_TOP) {      // Intersection avec bord haut (y = h-1)
                x = (dy != 0) ? (int)(x1_64 + dx * ((int64_t)h - 1 - y1_64) / dy) : (int)x1_64;
                y = h - 1;
            } else if (code_out & CS_BOTTOM) { // Bord bas (y = 0)
                x = (dy != 0) ? (int)(x1_64 + dx * (0 - y1_64) / dy) : (int)x1_64;
                y = 0;
            } else if (code_out & CS_RIGHT) {  // Bord droit (x = w-1)
                y = (dx != 0) ? (int)(y1_64 + dy * ((int64_t)w - 1 - x1_64) / dx) : (int)y1_64;
                x = w - 1;
            } else if (code_out & CS_LEFT) {   // Bord gauche (x = 0)
                y = (dx != 0) ? (int)(y1_64 + dy * (0 - x1_64) / dx) : (int)y1_64;
                x = 0;
            }

            // Remplace le point hors champ par l'intersection
            if (code_out == code1) {
                *x1 = x; *y1 = y;
                code1 = computeCSCode(*x1, *y1, w, h);
            } else {
                *x2 = x; *y2 = y;
                code2 = computeCSCode(*x2, *y2, w, h);
            }
        }
    }
    return accept;
}

/* -------------------------------------------------------------------------
* HELPERS DE DESSIN AVEC CLIPPING (Calculs 64 bits pour éviter overflow)
* -------------------------------------------------------------------------
* Ces fonctions sont les primitives "sûres" utilisées par l'API publique.
* Elles vérifient les bornes avant d'appeler les versions Unchecked.
* ------------------------------------------------------------------------- */

// Dessine un pixel avec vérification stricte des limites
static inline void drawPixelClipped64(GtWindow* window, int64_t x, int64_t y, uint32_t color) {
    if (!window || !window->buffer) return;
    if (x < 0 || x >= window->width || y < 0 || y >= window->height) return;
    putPixelUnchecked(window, (int)x, (int)y, color);
}

// Trace une ligne horizontale clippée aux bornes de la fenêtre
static void drawHLineClipped(GtWindow* window, int64_t y, int64_t x1, int64_t x2, uint32_t color) {
    if (y < 0 || y >= window->height) return;          // Y hors écran
    if (x1 > x2) { int64_t tmp = x1; x1 = x2; x2 = tmp; } // Assure x1 <= x2
    if (x2 < 0 || x1 >= window->width) return;         // Totalement hors X

    int clipped_x1 = (x1 < 0) ? 0 : (int)x1;                    // Clamp gauche
    int clipped_x2 = (x2 >= window->width) ? (window->width - 1) : (int)x2; // Clamp droit

    rasterizeHLineUnchecked(window, (int)y, clipped_x1, clipped_x2, color);
}

// Trace une ligne verticale clippée aux bornes de la fenêtre
static void drawVLineClipped(GtWindow* window, int64_t x, int64_t y1, int64_t y2, uint32_t color) {
    if (x < 0 || x >= window->width) return;             // X hors écran
    if (y1 > y2) { int64_t tmp = y1; y1 = y2; y2 = tmp; } // Assure y1 <= y2
    if (y2 < 0 || y1 >= window->height) return;          // Totalement hors Y

    int clipped_y1 = (y1 < 0) ? 0 : (int)y1;
    int clipped_y2 = (y2 >= window->height) ? (window->height - 1) : (int)y2;

    rasterizeVLineUnchecked(window, (int)x, clipped_y1, clipped_y2, color);
}

/* -------------------------------------------------------------------------
* GESTION FENÊTRE & PROCÉDURE WNDPROC WIN32
* -------------------------------------------------------------------------
* GtWndProc est la procédure de fenêtre Windows standard.
* Elle reçoit tous les messages pour les fenêtres de classe "LIBGTWindowClass".
* GWLP_USERDATA stocke le pointeur GtWindow* pour récupérer l'instance.
* ------------------------------------------------------------------------- */
static LRESULT CALLBACK GtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Récupère le pointeur GtWindow* stocké lors de CreateWindowEx
    GtWindow* win = (GtWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    // ===== CLAVIER =====
    case WM_KEYDOWN:      // Touche enfoncée (répétition auto si maintenue)
    case WM_SYSKEYDOWN:   // Touche système (Alt+...) enfoncée
        if (win && wParam < 256) win->keys[wParam] = true;
        break;

    case WM_KEYUP:        // Touche relâchée
    case WM_SYSKEYUP:     // Touche système relâchée
        if (win && wParam < 256) win->keys[wParam] = false;
        break;

    // ===== SOURIS - BOUTONS =====
    case WM_LBUTTONDOWN:  // Clic gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = true;
            SetCapture(hwnd);  // Capture la souris (reçoit événements même hors fenêtre)
        }
        break;
    case WM_LBUTTONUP:    // Relâche gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = false;
            ReleaseCapture();  // Libère la capture
        }
        break;

    case WM_RBUTTONDOWN:  // Clic droit
        if (win) win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = true;
        break;
    case WM_RBUTTONUP:
        if (win) win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = false;
        break;

    case WM_MBUTTONDOWN:  // Clic molette
        if (win) win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = true;
        break;
    case WM_MBUTTONUP:
        if (win) win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = false;
        break;

    // ===== SOURIS - MOUVEMENT =====
        case WM_MOUSEMOVE:
            if (win) {
                // LOWORD/HIWORD extraient les coordonnées signées 16 bits depuis lParam
                // (short) cast nécessaire car LOWORD/HIWORD retournent unsigned
                win->mouse_x = (int)(short)LOWORD(lParam);
                win->mouse_y = (int)(short)HIWORD(lParam);

                // Demander notification WM_MOUSELEAVE si pas déjà en cours
                if (!win->mouse_tracking) {
                    TRACKMOUSEEVENT tme;
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    tme.dwHoverTime = HOVER_DEFAULT;
                    if (TrackMouseEvent(&tme)) {
                        win->mouse_tracking = true;
                    }
                }
            }
            break;

        // ===== SOURIS - SORTIE FENÊTRE =====
        case WM_MOUSELEAVE:
            if (win) {
                win->mouse_x = -1;
                win->mouse_y = -1;
                win->mouse_tracking = false;
            }
            break;

    // ===== REDIMENSIONNEMENT =====
        case WM_SIZE:
            if (win) {
                int new_width = LOWORD(lParam);   // Nouvelle largeur zone cliente
                int new_height = HIWORD(lParam);  // Nouvelle hauteur zone cliente

                // Ignorer minimisation (0x0) — on garde l'ancien buffer
                if (new_width <= 0 || new_height <= 0) {
                    win->mouse_tracking = false;  // Tracking invalide après minimisation
                    break;
                }

                // Realloue seulement si taille réellement changée
                if (new_width != win->width || new_height != win->height) {
                    // Protection overflow taille
                    if ((size_t)new_width > SIZE_MAX / (size_t)new_height / sizeof(uint32_t)) break;

                    uint32_t* new_buffer = (uint32_t*)realloc(
                        win->buffer,
                        (size_t)new_width * (size_t)new_height * sizeof(uint32_t)
                    );
                    if (!new_buffer) break;  // Échec realloc : on garde ancien buffer + dimensions

                    win->buffer = new_buffer;
                    win->width = new_width;
                    win->height = new_height;

                    // Met à jour BITMAPINFO pour StretchDIBits (top-down DIB = biHeight négatif)
                    win->bmi.bmiHeader.biWidth = new_width;
                    win->bmi.bmiHeader.biHeight = -new_height;

                    // Clear immédiat pour éviter l'effet "cisaillé" (ancien stride vs nouveau stride)
                    size_t total = (size_t)new_width * new_height;
                    for (size_t i = 0; i < total; i++) win->buffer[i] = 0;
                }

                // Tracking souris invalide après resize (Windows l'annule)
                win->mouse_tracking = false;
            }
            break;

    // ===== PERTE DE FOCUS / CAPTURE =====
        case WM_CANCELMODE:   // Annulation mode (ex: menu ouvert, perte capture)
            if (win) {
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons));
                if (GetCapture() == hwnd) ReleaseCapture();
            }
            break;

        case WM_CAPTURECHANGED: // Capture perdue (autre fenêtre l'a prise)
            if (win) {
                win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = false;
                win->mouse_tracking = false; // Tracking annulé avec la capture
            }
            break;

        case WM_KILLFOCUS:    // Fenêtre perd le focus clavier
            if (win) {
                memset(win->keys, 0, sizeof(win->keys));           // Relâche toutes touches
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons)); // Relâche souris
                win->mouse_x = -1;  // Souris considérée comme hors fenêtre
                win->mouse_y = -1;
                win->mouse_tracking = false;
                if (GetCapture() == hwnd) ReleaseCapture();
            }
            break;

    // ===== FERMETURE =====
    case WM_CLOSE:        // Utilisateur clique sur X ou Alt+F4
        if (win) win->should_close = true;  // Marque pour fermeture, ne détruit PAS encore
        break;

    case WM_DESTROY:      // Fenêtre en train d'être détruite
        PostQuitMessage(0);  // Poste WM_QUIT dans la file du thread (sort GetMessage/PeekMessage)
        break;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam); // Traitement par défaut Windows
    }
    return 0; // Message traité
}

// Instanciation de la fenêtre et allocation mémoire
// title : titre affiché dans la barre de fenêtre
// width, height : dimensions de la ZONE CLIENTE (sans bordures/titre)
// Retourne : pointeur GtWindow* opaque, ou NULL si erreur
GtWindow* gtCreateWindow(const char* title, int width, int height) {
    bool registered_class = false;
    // Validation basique des paramètres
    if (width <= 0 || height <= 0) return NULL;
    // Protection contre overflow size_t (width * height * 4 bytes)
    if ((size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) return NULL;

    // Alloue la structure fenêtre (zéro-initialisée via calloc)
    GtWindow* win = (GtWindow*)calloc(1, sizeof(GtWindow));
    if (!win) return NULL;

    win->width = width;
    win->height = height;
    win->should_close = false;
    win->hInstance = GetModuleHandle(NULL);  // Instance du processus courant

    // Enregistre la classe fenêtre Win32 (une seule fois par process)
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = GtWndProc;              // Notre procédure de fenêtre
    wc.hInstance     = win->hInstance;
    wc.lpszClassName = "LIBGTWindowClass";     // Nom de classe unique
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // Curseur flèche standard

    if (!RegisterClassA(&wc)) {
        // Si classe déjà existante (deuxième fenêtre), c'est OK
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            free(win);
            return NULL;
        }
        // Class already exists, that's fine - we can use it
    }
    g_gtWindowClassRegistered = true;

    // Calcule la taille fenêtre complète (avec bordures/titre) depuis zone cliente
    RECT rect = {0, 0, width, height};
    if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
        if (registered_class) {
            UnregisterClassA("LIBGTWindowClass", win->hInstance);
            g_gtWindowClassRegistered = false;
        }
        free(win);
        return NULL;
    }

    // Crée la fenêtre Windows réelle (HWND)
    win->hwnd = CreateWindowExA(
        0,                              // Styles étendus (aucun)
        "LIBGTWindowClass",             // Nom de classe enregistrée
        title,                          // Titre fenêtre
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Fenêtre standard + visible immédiatement
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position par défaut Windows
        rect.right - rect.left,         // Largeur totale (avec bordures)
        rect.bottom - rect.top,         // Hauteur totale (avec bordures)
        NULL, NULL, win->hInstance, NULL
    );

    if (!win->hwnd) {
        if (registered_class) {
            UnregisterClassA("LIBGTWindowClass", win->hInstance);
            g_gtWindowClassRegistered = false;
        }
        free(win);
        return NULL;
    }

    // Alloue le framebuffer (zone mémoire pour les pixels)
    // calloc = zéro-initialisé (écran noir au démarrage)
    win->buffer = (uint32_t*)calloc(
        (size_t)width * (size_t)height,
        sizeof(uint32_t)
    );
    if (!win->buffer) {
        DestroyWindow(win->hwnd);
        if (registered_class) {
            UnregisterClassA("LIBGTWindowClass", win->hInstance);
            g_gtWindowClassRegistered = false;
        }
        free(win);
        return NULL;
    }

    // Lie le pointeur GtWindow* à la fenêtre Win32 (récupérable dans WndProc)
    SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);

    // Configure BITMAPINFO pour StretchDIBits (format DIB 32bpp ARGB, top-down)
    win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win->bmi.bmiHeader.biWidth = width;
    win->bmi.bmiHeader.biHeight = -height; // Négatif = top-down (origine en haut)
    win->bmi.bmiHeader.biPlanes = 1;
    win->bmi.bmiHeader.biBitCount = 32;
    win->bmi.bmiHeader.biCompression = BI_RGB; // Pas de compression, données brutes

    return win;
}

// Destruction de la fenêtre et libération mémoire
// Ordre important : HWND → buffer → structure → classe fenêtre
void gtDestroyWindow(GtWindow* window) {
    if (!window) return;

    HINSTANCE hInstance = window->hInstance;

    // 1. Détruit la fenêtre Windows (envoie WM_DESTROY, WM_NCDESTROY)
    if (window->hwnd) {
        DestroyWindow(window->hwnd);
    }

    // 2. Libère le framebuffer
    if (window->buffer) {
        free(window->buffer);
    }

    // 3. Libère la structure GtWindow
    free(window);

    // 4. Désenregistre la classe fenêtre (si on l'avait enregistrée)
    if (g_gtWindowClassRegistered) {
        UnregisterClassA("LIBGTWindowClass", hInstance);
        g_gtWindowClassRegistered = false;
    }
}

// Traite la file de messages Windows (non-bloquant)
// À appeler une fois par frame dans la boucle principale
// Utilise PeekMessage (pas GetMessage) pour ne pas bloquer si pas de message
// Retourne false si WM_QUIT reçu (application doit quitter)
bool gtEventsWindow(GtWindow* window) {
    if (!window) return false;
    MSG msg;
    // PM_REMOVE = retire le message de la file après lecture
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) window->should_close = true;
        TranslateMessage(&msg);  // Convertit WM_KEYDOWN en WM_CHAR pour input texte
        DispatchMessageA(&msg);  // Appelle GtWndProc
    }
    return !window->should_close;
}

// Copie le framebuffer RAM vers l'écran (Blitting GDI)
// Utilise StretchDIBits : copie mémoire -> device context fenêtre
// SRCCOPY = copie directe sans opération raster (ROP)
void gtUpdateWindow(GtWindow* window) {
    if (!window || !window->hwnd || !window->buffer) return;
    HDC hdc = GetDC(window->hwnd);  // Device context de la zone cliente
    if (!hdc) return;

    StretchDIBits(
        hdc,
        0, 0, window->width, window->height,  // Destination (écran)
        0, 0, window->width, window->height,  // Source (framebuffer)
        window->buffer, &window->bmi, DIB_RGB_COLORS, SRCCOPY
    );

    ReleaseDC(window->hwnd, hdc);  // Libère le DC (ressource limitée)
}

// Effacement de l'écran avec une couleur unie
// Parcourt tout le framebuffer (boucle simple, memset ne marche que pour 0x00/0xFF)
void gtClearWindow(GtWindow* window, uint32_t color) {
    if (!window || !window->buffer) return;
    size_t total_pixels = (size_t)window->width * (size_t)window->height;

    // Boucle pour toutes les couleurs (memset ne marche byte-wise que pour 0)
    for (size_t i = 0; i < total_pixels; i++) {
        window->buffer[i] = color;
    }
}

/* -------------------------------------------------------------------------
* PRIMITIVES PUBLIQUES DE DESSIN 2D (sur GtWindow directement)
* -------------------------------------------------------------------------
* Toutes ces fonctions font du clipping automatique.
* Coordonnées : (0,0) = coin haut-gauche de la zone cliente.
* ------------------------------------------------------------------------- */

// Dessine un seul pixel (avec clipping)
void gtDrawPixel(GtWindow* window, int x, int y, uint32_t color) {
    drawPixelClipped64(window, (int64_t)x, (int64_t)y, color);
}

// Dessine un rectangle plein (rempli)
// x,y = coin haut-gauche, w,h = largeur/hauteur en pixels
// Clipping : intersection avec [0,width-1] x [0,height-1]
void gtDrawRect(GtWindow* window, int x, int y, int w, int h, uint32_t color) {
    if (!window || !window->buffer || w <= 0 || h <= 0) return;

    // Coordonnées du coin bas-droit (exclusif)
    int64_t xw = (int64_t)x + w;
    int64_t yh = (int64_t)y + h;

    // Clamp aux bornes de la fenêtre
    int x1 = (x < 0) ? 0 : x;
    int y1 = (y < 0) ? 0 : y;
    int x2 = (xw > (int64_t)window->width) ? window->width : (int)xw;
    int y2 = (yh > (int64_t)window->height) ? window->height : (int)yh;

    // Rectangle vide après clipping ?
    if (x1 >= x2 || y1 >= y2) return;

    size_t stride = (size_t)window->width;
    int count = x2 - x1;  // Nombre de pixels par ligne

    // Parcours ligne par ligne (cache-friendly : accès séquentiel en mémoire)
    for (int row = y1; row < y2; row++) {
        uint32_t* row_ptr = &window->buffer[(size_t)row * stride + (size_t)x1];

        if (color == 0) {
            // Optimisation : memset plus rapide pour couleur noire (0x00000000)
            memset(row_ptr, 0, (size_t)count * sizeof(uint32_t));
        } else {
            // Boucle simple pour autres couleurs
            for (int col = 0; col < count; col++) {
                row_ptr[col] = color;
            }
        }
    }
}

// Dessine le contour d'un rectangle (4 lignes)
// x,y = coin haut-gauche, w,h = largeur/hauteur
// Les coins sont dessinés par les lignes horizontales (pas de double dessin)
void gtDrawRectLines(GtWindow* window, int x, int y, int w, int h, uint32_t color) {
    if (!window || !window->buffer || w <= 0 || h <= 0) return;

    // Coin bas-droit (inclusif pour les lignes)
    int64_t x2_64 = (int64_t)x + (int64_t)w - 1;
    int64_t y2_64 = (int64_t)y + (int64_t)h - 1;

    int x2 = (x2_64 >= (int64_t)window->width) ? window->width - 1 : (int)x2_64;
    int y2 = (y2_64 >= (int64_t)window->height) ? window->height - 1 : (int)y2_64;

    /* Bords horizontaux (haut et bas) : ils dessinent déjà les coins. */
    drawHLineClipped(window, y, x, x2, color);

    if (y2 != y) {  // Évite de redessiner la même ligne si h=1
        drawHLineClipped(window, y2, x, x2, color);
    }

    /* Bords verticaux (gauche et droite) :
     * On exclut les extrémités (y+1 à y2-1) car les coins
     * ont déjà été dessinés par les horizontales. */
    int64_t vertical_y1 = (int64_t)y + 1;
    int64_t vertical_y2 = (int64_t)y2 - 1;

    if (vertical_y1 <= vertical_y2) {
        drawVLineClipped(window, x, vertical_y1, vertical_y2, color);
        if (x2 != x) {  // Évite de redessiner si w=1
            drawVLineClipped(window, x2, vertical_y1, vertical_y2, color);
        }
    }
}

// Dessine une ligne entre deux points (algorithme de Bresenham + clipping Cohen-Sutherland)
// Clipping effectué AVANT rasterisation pour éviter calculs inutiles
void gtDrawLine(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color) {
    if (!window || !window->buffer) return;

    // Clip le segment aux bornes de la fenêtre
    if (!clipLineSegment(&x1, &y1, &x2, &y2, window->width, window->height)) {
        return;  // Totalement hors écran
    }

    // Rasterisation Bresenham (entiers, pas de float)
    rasterizeBresenhamUnchecked(window, x1, y1, x2, y2, color);
}

// Dessine le contour d'un cercle (algorithme du point milieu / midpoint circle)
// cx,cy = centre, radius = rayon en pixels
// Optimisation : si cercle entièrement dans l'écran, utilise putPixelUnchecked (sans clipping)
void gtDrawCircleLines(GtWindow* window, int cx, int cy, int radius, uint32_t color) {
    if (!window || !window->buffer || radius < 0) return;

    int64_t cx64 = cx;
    int64_t cy64 = cy;
    int64_t r64 = radius;

    // Cas dégénéré : rayon 0 = un seul pixel
    if (r64 == 0) {
        drawPixelClipped64(window, cx64, cy64, color);
        return;
    }

    // Test si cercle entièrement visible (optimisation : pas de clipping par pixel)
    bool fully_inside =
    (cx64 - r64 >= 0 &&
        cy64 - r64 >= 0 &&
        cx64 + r64 < window->width &&
        cy64 + r64 < window->height);

    // Algorithme du point milieu (Midpoint Circle Algorithm)
    // Parcourt 1/8 du cercle (octant) et symétrie 8 directions
    int64_t x = r64;
    int64_t y = 0;
    int64_t err = 0;

    while (x >= y) {
        if (fully_inside) {
            // Version rapide sans clipping (8-way symmetry)
            putPixelUnchecked(window, (int)(cx64 + x), (int)(cy64 + y), color);
            putPixelUnchecked(window, (int)(cx64 - y), (int)(cy64 + x), color);
            putPixelUnchecked(window, (int)(cx64 - x), (int)(cy64 - y), color);
            putPixelUnchecked(window, (int)(cx64 + y), (int)(cy64 - x), color);

            // Les 4 autres points symétriques (si distincts des précédents)
            if (y != 0 && x != y) {
                putPixelUnchecked(window, (int)(cx64 + y), (int)(cy64 + x), color);
                putPixelUnchecked(window, (int)(cx64 - x), (int)(cy64 + y), color);
                putPixelUnchecked(window, (int)(cx64 - y), (int)(cy64 - x), color);
                putPixelUnchecked(window, (int)(cx64 + x), (int)(cy64 - y), color);
            }
        } else {
            // Version avec clipping par pixel (plus lent mais sûr)
            drawPixelClipped64(window, cx64 + x, cy64 + y, color);
            drawPixelClipped64(window, cx64 - y, cy64 + x, color);
            drawPixelClipped64(window, cx64 - x, cy64 - y, color);
            drawPixelClipped64(window, cx64 + y, cy64 - x, color);

            if (y != 0 && x != y) {
                drawPixelClipped64(window, cx64 + y, cy64 + x, color);
                drawPixelClipped64(window, cx64 - x, cy64 + y, color);
                drawPixelClipped64(window, cx64 - y, cy64 - x, color);
                drawPixelClipped64(window, cx64 + x, cy64 - y, color);
            }
        }

        // Mise à jour variables du point milieu
        if (err <= 0) { y += 1; err += 2 * y + 1; }
        if (err > 0)  { x -= 1; err -= 2 * x + 1; }
    }
}

// Dessine un cercle plein (disque) par balayage horizontal (scanlines)
// Plus efficace que point-milieu pour remplissage : trace des lignes horizontales
void gtDrawCircle(GtWindow* window, int cx, int cy, int radius, uint32_t color) {
    if (!window || !window->buffer || radius < 0) return;

    int64_t cx64 = cx;
    int64_t cy64 = cy;
    int64_t x = radius;
    int64_t y = 0;
    int64_t err = 0;

    while (x >= y) {
        // Ligne horizontale principale (y offset depuis centre)
        drawHLineClipped(window, cy64 + y, cx64 - x, cx64 + x, color);

        // Ligne symétrique en bas (si y != 0 pour éviter double tracé centre)
        if (y != 0) {
            drawHLineClipped(window, cy64 - y, cx64 - x, cx64 + x, color);
        }

        // Lignes pour l'autre octant (x et y échangés)
        if (x != y) {
            drawHLineClipped(window, cy64 + x, cx64 - y, cx64 + y, color);
            drawHLineClipped(window, cy64 - x, cx64 - y, cx64 + y, color);
        }

        if (err <= 0) { y += 1; err += 2 * y + 1; }
        if (err > 0)  { x -= 1; err -= 2 * x + 1; }
    }
}

/* -------------------------------------------------------------------------
* ENTRÉES UTILISATEUR (Implémentation)
* ------------------------------------------------------------------------- */

// Vérifie si une touche est enfoncée (état maintenu, pas événement unique)
// keycode : code VK_* Windows (ex: GT_KEY_W, VK_SPACE, 'A', etc.)
bool gtIsKeyDown(GtWindow* window, int keycode) {
    if (!window || keycode < 0 || keycode >= 256) return false;
    return window->keys[keycode];
}

// Vérifie si un bouton souris est enfoncé
// button : GT_MOUSE_BUTTON_LEFT (0), RIGHT (1), MIDDLE (2)
bool gtIsMouseButtonDown(GtWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return window->mouse_buttons[button];
}

// Récupère la position souris (coordonnées client : 0,0 = haut-gauche)
// out_x, out_y : pointeurs vers int pour recevoir les valeurs (peuvent être NULL)
void gtGetMousePos(GtWindow* window, int* out_x, int* out_y) {
    if (!window) return;
    if (out_x) *out_x = window->mouse_x;
    if (out_y) *out_y = window->mouse_y;
}

// Vérifie si la fenêtre doit fermer (WM_CLOSE reçu ou WM_QUIT)
bool gtShouldClose(GtWindow* window) {
    if (!window) return true;
    return window->should_close;
}

// Accesseurs dimensions
int gtGetWidth(GtWindow* window) { if (!window) return 0; return window->width; }
int gtGetHeight(GtWindow* window) { if (!window) return 0; return window->height; }

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : TEMPS (QueryPerformanceCounter pour haute précision)
* -------------------------------------------------------------------------
* QPC (QueryPerformanceCounter) est le timer haute résolution Windows.
* Fréquence fixe au boot (QueryPerformanceFrequency), typiquement ~3-4 MHz.
* Précision bien meilleure que timeGetTime (1ms) ou GetTickCount (10-15ms).
* Pas de dérive, monotone (ne recule jamais).
* ------------------------------------------------------------------------- */

// Variables d'état globales (static = linkage interne, pas de conflit)
static LARGE_INTEGER g_gtFrequency = {0};   // Fréquence du compteur (ticks/seconde)
static LARGE_INTEGER g_gtStartTime = {0};   // Temps au premier gtBeginFrame()
static LARGE_INTEGER g_gtLastFrame = {0};   // Temps au début de la frame précédente
static float g_gtDeltaTime = 0.0f;          // Delta time frame courante (secondes)

// Initialisation paresseuse (lazy init) : appelée au premier gtGetTime/gtBeginFrame
static void gtTimeInit(void) {
    if (g_gtFrequency.QuadPart == 0) {      // Pas encore initialisé ?
        QueryPerformanceFrequency(&g_gtFrequency); // Récupère fréquence hardware
        QueryPerformanceCounter(&g_gtStartTime);   // Temps de référence = maintenant
        g_gtLastFrame = g_gtStartTime;             // Frame précédente = maintenant
    }
}

// Temps total écoulé depuis gtTimeInit() (secondes, double précision)
// Utile pour animations temps-réel, timestamps, profiling
double gtGetTime(void) {
    gtTimeInit();  // Init si premier appel
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    // (now - start) / frequency = secondes écoulées
    return (double)(now.QuadPart - g_gtStartTime.QuadPart) / (double)g_gtFrequency.QuadPart;
}

// Delta time de la frame COURANTE (secondes, float)
// Mis à jour UNE SEULE FOIS par frame par gtBeginFrame()
// Ne pas appeler avant le premier gtBeginFrame() (retournera 0)
float gtGetDeltaTime(void) {
    return g_gtDeltaTime;
}

// À appeler AU TOUT DÉBUT de chaque frame (avant logique, événements, rendu)
// Calcule : delta = (now - lastFrame) / frequency
// Met à jour g_gtLastFrame = now pour la frame suivante
void gtBeginFrame(void) {
    gtTimeInit();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    g_gtDeltaTime = (float)((double)(now.QuadPart - g_gtLastFrame.QuadPart) / (double)g_gtFrequency.QuadPart);
    g_gtLastFrame = now;
}

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : RENDERER ABSTRACTION (Backend GDI)
* -------------------------------------------------------------------------
* Structure interne du renderer (opaque pour l'utilisateur).
* Contient le backend-specific data (champs réservés pour D2D).
* Note: HDC n'est PAS stocké — obtenu via GetDC() à chaque frame dans gtRendererEnd
*       pour éviter invalidation après WM_SIZE, changement DPI, veille, etc.
* ------------------------------------------------------------------------- */
struct GtRenderer {
    GtWindow* window;          // Fenêtre cible
    GtRendererType type;       // Type de backend (GDI ou D2D)
    // D2D-specific (réservé pour implémentation future)
    void* d2d_factory;         // ID2D1Factory*
    void* d2d_render_target;   // ID2D1HwndRenderTarget*
    void* d2d_brush;           // ID2D1SolidColorBrush*
};

// Crée un renderer pour une fenêtre
// type : GT_RENDERER_GDI (recommandé, toujours dispo) ou GT_RENDERER_D2D (fallback GDI si indisponible)
GtRenderer* gtCreateRenderer(GtWindow* window, GtRendererType type) {
    if (!window) return NULL;

    GtRenderer* renderer = (GtRenderer*)calloc(1, sizeof(GtRenderer));
    if (!renderer) return NULL;

    renderer->window = window;
    renderer->type = type;

    if (type == GT_RENDERER_D2D) {
        // D2D non implémenté : fallback silencieux vers GDI
        renderer->type = GT_RENDERER_GDI;
    }
    // Pas de GetDC() ici — HDC obtenu à chaque frame dans gtRendererEnd

    return renderer;
}

// Détruit le renderer et libère ses ressources
void gtDestroyRenderer(GtRenderer* renderer) {
    if (!renderer) return;
    // Pas de ReleaseDC() — HDC n'est pas stocké
    free(renderer);
}

// Début de frame : prépare le backend pour le dessin
// GDI : rien à faire (dessine direct dans framebuffer RAM)
// D2D futur : appellera ID2D1RenderTarget::BeginDraw()
void gtRendererBegin(GtRenderer* renderer) {
    (void)renderer;  // Évite warning "unused parameter" pour GDI
}

// Fin de frame : présente le résultat à l'écran
// GDI : appelle gtUpdateWindow (GetDC/StretchDIBits/ReleaseDC à chaque frame)
// D2D futur : EndDraw() + Present()
void gtRendererEnd(GtRenderer* renderer) {
    if (!renderer || !renderer->window) return;

    if (renderer->type == GT_RENDERER_GDI) {
        gtUpdateWindow(renderer->window);  // Unique point de blit (HDC frais à chaque appel)
    }
    // Pour D2D : EndDraw() + Present()
}

// Efface le framebuffer (délègue à gtClearWindow)
void gtRendererClear(GtRenderer* renderer, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtClearWindow(renderer->window, color);
}

// Primitives de dessin (délèguent aux fonctions GtWindow pour GDI)
// Pour D2D futur : utiliseront ID2D1RenderTarget::DrawRectangle, FillEllipse, etc.
void gtRendererDrawPixel(GtRenderer* renderer, int x, int y, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawPixel(renderer->window, x, y, color);
}
void gtRendererDrawRect(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawRect(renderer->window, x, y, w, h, color);
}
void gtRendererDrawRectLines(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawRectLines(renderer->window, x, y, w, h, color);
}
void gtRendererDrawLine(GtRenderer* renderer, int x1, int y1, int x2, int y2, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawLine(renderer->window, x1, y1, x2, y2, color);
}
void gtRendererDrawCircle(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawCircle(renderer->window, cx, cy, radius, color);
}
void gtRendererDrawCircleLines(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window) return;
    gtDrawCircleLines(renderer->window, cx, cy, radius, color);
}

#endif // LIBGT_IMPLEMENTATION
#endif // LIBGT_H
