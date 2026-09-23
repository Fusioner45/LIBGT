    #ifndef LIBGT_H
    #define LIBGT_H

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>

    /* =========================================================================
    * API PUBLIQUE (Déclarations)
    * ========================================================================= */

    // Structure opaque représentant la fenêtre et son tampon de pixels (framebuffer)
    typedef struct GtWindow GtWindow;

    // Fonctions de gestion du cycle de vie de la fenêtre
    GtWindow* gtCreateWindow(const char* title, int width, int height);
    void      gtDestroyWindow(GtWindow* window);
    bool      gtEventsWindow(GtWindow* window);
    void      gtUpdateWindow(GtWindow* window);
    void      gtClearWindow(GtWindow* window, uint32_t color);

    // Fonctions de rendu graphique (primitives 2D)
    void gtDrawPixel(GtWindow* window, int x, int y, uint32_t color);
    void gtDrawRect(GtWindow* window, int x, int y, int w, int h, uint32_t color);
    void gtDrawRectLines(GtWindow* window, int x, int y, int w, int h, uint32_t color);
    void gtDrawLine(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color);
    void gtDrawCircleLines(GtWindow* window, int cx, int cy, int radius, uint32_t color);
    void gtDrawCircle(GtWindow* window, int cx, int cy, int radius, uint32_t color);

    /* =========================================================================
    * COULEURS DE BASE (Format HEXADÉCIMAL 0x00RRGGBB)
    * ========================================================================= */
    #define GT_BLACK    0x00000000
    #define GT_WHITE    0x00FFFFFF
    #define GT_RED      0x00F43F5E
    #define GT_GREEN    0x0022C55E
    #define GT_BLUE     0x003B82F6
    #define GT_YELLOW   0x00EAB308
    #define GT_DARKGRAY 0x000F172A

    /* =========================================================================
    * GESTION DES ENTRÉES (Clavier & Souris)
    * ========================================================================= */
    #define GT_MOUSE_BUTTON_LEFT   0
    #define GT_MOUSE_BUTTON_RIGHT  1
    #define GT_MOUSE_BUTTON_MIDDLE 2

    // Virtual-Key codes Windows courants (VK_*)
    #define GT_KEY_SPACE  0x20
    #define GT_KEY_LEFT   0x25
    #define GT_KEY_UP     0x26
    #define GT_KEY_RIGHT  0x27
    #define GT_KEY_DOWN   0x28
    #define GT_KEY_A      'A'
    #define GT_KEY_D      'D'
    #define GT_KEY_S      'S'
    #define GT_KEY_W      'W'
    #define GT_KEY_ESCAPE 0x1B

    bool gtIsKeyDown(GtWindow* window, int keycode);
    bool gtIsMouseButtonDown(GtWindow* window, int button);
    void gtGetMousePos(GtWindow* window, int* out_x, int* out_y);

    /* =========================================================================
    * IMPLÉMENTATION NATIVE WIN32
    * ========================================================================= */
    #ifdef LIBGT_IMPLEMENTATION

    #include <windows.h>
    #include <stdlib.h>
    #include <string.h>

    // Définition interne de la structure de fenêtre
    struct GtWindow {
        HWND      hwnd;          // Identifiant unique de la fenêtre Win32
        HINSTANCE hInstance;     // Handle de l'instance du programme
        int       width;         // Largeur de la surface utile (zone cliente)
        int       height;        // Hauteur de la surface utile
        bool      should_close;  // Drapeaux de demande de fermeture

        uint32_t* buffer;        // Framebuffer 1D en RAM (taille = width * height)
        BITMAPINFO bmi;          // Structure de description du format d'image pour GDI

        bool keys[256];          // État binaire des 256 touches du clavier
        bool mouse_buttons[3];   // État des 3 boutons de la souris
        int  mouse_x;            // Position X de la souris relative à la zone cliente
        int  mouse_y;            // Position Y de la souris relative à la zone cliente
    };

    /* -------------------------------------------------------------------------
    * RASTERISATION BAS NIVEAU (Accès direct en mémoire, sans contrôle de bornes)
    * Attention : l'appelant doit garantir que les coordonnées sont valides !
    * ------------------------------------------------------------------------- */

    // Écrit un pixel unique directement dans le buffer
    static inline void putPixelUnchecked(GtWindow* window, int x, int y, uint32_t color) {
        window->buffer[(size_t)y * (size_t)window->width + (size_t)x] = color;
    }

    // Trace une ligne horizontale continue en mémoire
    static inline void rasterizeHLineUnchecked(GtWindow* window, int y, int x1, int x2, uint32_t color) {
        size_t row_start = (size_t)y * (size_t)window->width;
        for (int x = x1; x <= x2; x++) {
            window->buffer[row_start + (size_t)x] = color;
        }
    }

    // Trace une ligne verticale continue (saut de ligne par 'stride')
    static inline void rasterizeVLineUnchecked(GtWindow* window, int x, int y1, int y2, uint32_t color) {
        size_t stride = (size_t)window->width;
        size_t index = (size_t)y1 * stride + (size_t)x;
        for (int y = y1; y <= y2; y++) {
            window->buffer[index] = color;
            index += stride;
        }
    }

    // Algorithme de ligne de Bresenham (calculs 100 % entiers)
    static inline void rasterizeBresenhamUnchecked(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (1) {
            putPixelUnchecked(window, x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    /* -------------------------------------------------------------------------
    * CLIPPING DE SEGMENTS (Algorithme Cohen-Sutherland)
    * Découpe une ligne pour qu'elle s'insère dans [0, w-1] x [0, h-1].
    * ------------------------------------------------------------------------- */
    #define CS_INSIDE 0 // 0000 : Le point est dans l'écran
    #define CS_LEFT   1 // 0001 : À gauche
    #define CS_RIGHT  2 // 0010 : À droite
    #define CS_BOTTOM 4 // 0100 : En bas
    #define CS_TOP    8 // 1000 : En haut

    // Calcule le code de région (OutCode) pour un point donné
    static int computeCSCode(int x, int y, int w, int h) {
        int code = CS_INSIDE;
        if (x < 0)       code |= CS_LEFT;
        else if (x >= w) code |= CS_RIGHT;
        if (y < 0)       code |= CS_BOTTOM;
        else if (y >= h) code |= CS_TOP;
        return code;
    }

    // Recadre les coordonnées (x1, y1)-(x2, y2). Renvoie false si hors champ.
    static bool clipLineSegment(int* x1, int* y1, int* x2, int* y2, int w, int h) {
        int code1 = computeCSCode(*x1, *y1, w, h);
        int code2 = computeCSCode(*x2, *y2, w, h);
        bool accept = false;

        while (1) {
            if ((code1 | code2) == 0) {
                accept = true; // Totalement visible
                break;
            } 
            else if (code1 & code2) {
                break; // Totalement hors champ
            } 
            else {
                int x = 0, y = 0;
                int code_out = code1 ? code1 : code2;

                int64_t x1_64 = *x1, y1_64 = *y1;
                int64_t x2_64 = *x2, y2_64 = *y2;

                int64_t dx = x2_64 - x1_64;
                int64_t dy = y2_64 - y1_64;

                if (code_out & CS_TOP) {
                    x = (dy != 0) ? (int)(x1_64 + dx * ((int64_t)h - 1 - y1_64) / dy) : (int)x1_64;
                    y = h - 1;
                } else if (code_out & CS_BOTTOM) {
                    x = (dy != 0) ? (int)(x1_64 + dx * (0 - y1_64) / dy) : (int)x1_64;
                    y = 0;
                } else if (code_out & CS_RIGHT) {
                    y = (dx != 0) ? (int)(y1_64 + dy * ((int64_t)w - 1 - x1_64) / dx) : (int)y1_64;
                    x = w - 1;
                } else if (code_out & CS_LEFT) {
                    y = (dx != 0) ? (int)(y1_64 + dy * (0 - x1_64) / dx) : (int)y1_64;
                    x = 0;
                }

                if (code_out == code1) {
                    *x1 = x;
                    *y1 = y;
                    code1 = computeCSCode(*x1, *y1, w, h);
                } else {
                    *x2 = x;
                    *y2 = y;
                    code2 = computeCSCode(*x2, *y2, w, h);
                }
            }
        }
        return accept;
    }

    /* -------------------------------------------------------------------------
    * HELPERS DE SÉCURITÉ ET D'EXÉCUTION (Calculs 64 bits)
    * ------------------------------------------------------------------------- */

    // Écrit un pixel après vérification stricte des limites de la fenêtre
    static inline void drawPixelClipped64(GtWindow* window, int64_t x, int64_t y, uint32_t color) {
        if (!window || !window->buffer) return;
        if (x < 0 || x >= window->width || y < 0 || y >= window->height) return;
        putPixelUnchecked(window, (int)x, (int)y, color);
    }

    // Tronque un segment horizontal aux limites physiques de la fenêtre
    static void drawHLineClipped(GtWindow* window, int64_t y, int64_t x1, int64_t x2, uint32_t color) {
        if (y < 0 || y >= window->height) return;
        if (x1 > x2) { int64_t tmp = x1; x1 = x2; x2 = tmp; }
        if (x2 < 0 || x1 >= window->width) return;

        int clipped_x1 = (x1 < 0) ? 0 : (int)x1;
        int clipped_x2 = (x2 >= window->width) ? (window->width - 1) : (int)x2;

        rasterizeHLineUnchecked(window, (int)y, clipped_x1, clipped_x2, color);
    }

    // Tronque un segment vertical aux limites physiques de la fenêtre
    static void drawVLineClipped(GtWindow* window, int64_t x, int64_t y1, int64_t y2, uint32_t color) {
        if (x < 0 || x >= window->width) return;
        if (y1 > y2) { int64_t tmp = y1; y1 = y2; y2 = tmp; }
        if (y2 < 0 || y1 >= window->height) return;

        int clipped_y1 = (y1 < 0) ? 0 : (int)y1;
        int clipped_y2 = (y2 >= window->height) ? (window->height - 1) : (int)y2;

        rasterizeVLineUnchecked(window, (int)x, clipped_y1, clipped_y2, color);
    }

    /* -------------------------------------------------------------------------
    * GESTION FENÊTRE & PROCÉDURE WNDPROC WIN32
    * ------------------------------------------------------------------------- */
    static LRESULT CALLBACK GtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        GtWindow* win = (GtWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (msg) {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (win && wParam < 256) win->keys[wParam] = true;
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (win && wParam < 256) win->keys[wParam] = false;
                break;

            case WM_LBUTTONDOWN:
                if (win) {
                    win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = true;
                    SetCapture(hwnd);
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

            case WM_MOUSEMOVE:
                if (win) {
                    win->mouse_x = (int)(short)LOWORD(lParam);
                    win->mouse_y = (int)(short)HIWORD(lParam);
                }
                break;

            case WM_KILLFOCUS:
                if (win) {
                    memset(win->keys, 0, sizeof(win->keys));
                    memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons));
                }
                break;

            case WM_CLOSE:
                if (win) win->should_close = true;
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    // Instanciation de la fenêtre et allocation mémoire
    GtWindow* gtCreateWindow(const char* title, int width, int height) {
        if (width <= 0 || height <= 0) return NULL;
        if ((size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) return NULL;

        GtWindow* win = (GtWindow*)calloc(1, sizeof(GtWindow));
        if (!win) return NULL;

        win->width = width;
        win->height = height;
        win->should_close = false;
        win->hInstance = GetModuleHandle(NULL);

        WNDCLASSA wc = {0};
        wc.lpfnWndProc   = GtWndProc;
        wc.hInstance     = win->hInstance;
        wc.lpszClassName = "LIBGTWindowClass";
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

        if (!RegisterClassA(&wc)) {
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                free(win);
                return NULL;
            }
        }

        RECT rect = {0, 0, width, height};
        if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
            free(win);
            return NULL;
        }

        win->hwnd = CreateWindowExA(
            0, "LIBGTWindowClass", title,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            NULL, NULL, win->hInstance, NULL
        );

        if (!win->hwnd) {
            free(win);
            return NULL;
        }

        
        win->buffer = (uint32_t*)calloc(
        (size_t)width * (size_t)height,
        sizeof(uint32_t)
        );
        if (!win->buffer) {
            DestroyWindow(win->hwnd);
            free(win);
            return NULL;
        }

        SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);

        win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        win->bmi.bmiHeader.biWidth = width;
        win->bmi.bmiHeader.biHeight = -height; // Inversion Y pour repère top-left (0,0)
        win->bmi.bmiHeader.biPlanes = 1;
        win->bmi.bmiHeader.biBitCount = 32;
        win->bmi.bmiHeader.biCompression = BI_RGB;

        return win;
    }

    // Destruction de la fenêtre et libération mémoire
    void gtDestroyWindow(GtWindow* window) {
        if (!window) return;
        if (window->hwnd) DestroyWindow(window->hwnd);
        if (window->buffer) free(window->buffer);
        free(window);
    }

    // Traitement de la file de messages Windows
    bool gtEventsWindow(GtWindow* window) {
        if (!window) return false;
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) window->should_close = true;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        return !window->should_close;
    }

    // Copie du buffer RAM vers l'écran (Blitting)
    void gtUpdateWindow(GtWindow* window) {
        if (!window || !window->hwnd || !window->buffer) return;
        HDC hdc = GetDC(window->hwnd);
        if (!hdc) return;

        StretchDIBits(
            hdc, 0, 0, window->width, window->height,
            0, 0, window->width, window->height,
            window->buffer, &window->bmi, DIB_RGB_COLORS, SRCCOPY
        );

        ReleaseDC(window->hwnd, hdc);
    }

    // Effacement de l'écran avec une couleur unie
    void gtClearWindow(GtWindow* window, uint32_t color) {
        if (!window || !window->buffer) return;
        size_t total_pixels = (size_t)window->width * (size_t)window->height;
        for (size_t i = 0; i < total_pixels; i++) {
            window->buffer[i] = color;
        }
    }

    /* -------------------------------------------------------------------------
    * PRIMITIVES PUBLIQUES DE DESSIN 2D
    * ------------------------------------------------------------------------- */

    void gtDrawPixel(GtWindow* window, int x, int y, uint32_t color) {
        drawPixelClipped64(window, (int64_t)x, (int64_t)y, color);
    }

    void gtDrawRect(GtWindow* window, int x, int y, int w, int h, uint32_t color) {
        if (!window || !window->buffer || w <= 0 || h <= 0) return;

        int64_t xw = (int64_t)x + w;
        int64_t yh = (int64_t)y + h;

        int x1 = (x < 0) ? 0 : x;
        int y1 = (y < 0) ? 0 : y;
        int x2 = (xw > (int64_t)window->width) ? window->width : (int)xw;
        int y2 = (yh > (int64_t)window->height) ? window->height : (int)yh;

        if (x1 >= x2 || y1 >= y2) return;

        size_t stride = (size_t)window->width;
        for (int row = y1; row < y2; row++) {
            uint32_t* row_ptr = &window->buffer[(size_t)row * stride + (size_t)x1];
            int count = x2 - x1;
            for (int col = 0; col < count; col++) {
                row_ptr[col] = color;
            }
        }
    }

    void gtDrawRectLines(GtWindow* window, int x, int y, int w, int h, uint32_t color) {
        if (!window || !window->buffer || w <= 0 || h <= 0) return;

        int64_t x2_64 = (int64_t)x + (int64_t)w - 1;
        int64_t y2_64 = (int64_t)y + (int64_t)h - 1;

        int x2 = (x2_64 >= (int64_t)window->width)
            ? window->width - 1
            : (int)x2_64;

        int y2 = (y2_64 >= (int64_t)window->height)
            ? window->height - 1
            : (int)y2_64;

        /* Bords horizontaux : ils dessinent déjà les coins. */
        drawHLineClipped(window, y, x, x2, color);

        if (h > 1) {
            drawHLineClipped(window, y2, x, x2, color);
        }

        /*
        * Bords verticaux :
        * on exclut volontairement les deux extrémités,
        * car elles ont déjà été dessinées par les horizontales.
        */
        int64_t vertical_y1 = (int64_t)y + 1;
        int64_t vertical_y2 = (int64_t)y2 - 1;

        if (vertical_y1 <= vertical_y2) {
            drawVLineClipped(window, x, vertical_y1, vertical_y2, color);

            if (w > 1) {
                drawVLineClipped(window, x2, vertical_y1, vertical_y2, color);
            }
        }
    }

    void gtDrawLine(GtWindow* window, int x1, int y1, int x2, int y2, uint32_t color) {
        if (!window || !window->buffer) return;

        if (!clipLineSegment(&x1, &y1, &x2, &y2, window->width, window->height)) {
            return;
        }

        rasterizeBresenhamUnchecked(window, x1, y1, x2, y2, color);
    }

    void gtDrawCircleLines(GtWindow* window, int cx, int cy, int radius, uint32_t color) {
        if (!window || !window->buffer || radius < 0) return;

        int64_t cx64 = cx;
        int64_t cy64 = cy;
        int64_t r64 = radius;

        /*
        * Cas dégénéré : un cercle de rayon 0 est un seul pixel.
        */
        if (r64 == 0) {
            drawPixelClipped64(window, cx64, cy64, color);
            return;
        }

        bool fully_inside =
            (cx64 - r64 >= 0 &&
            cy64 - r64 >= 0 &&
            cx64 + r64 < window->width &&
            cy64 + r64 < window->height);

        int64_t x = r64;
        int64_t y = 0;
        int64_t err = 0;

        while (x >= y) {
            if (fully_inside) {
                /*
                * Ces 4 points sont toujours distincts
                * pour radius > 0, même lorsque :
                *   - y == 0
                *   - x == y
                */
                putPixelUnchecked(
                    window, (int)(cx64 + x), (int)(cy64 + y), color
                );

                putPixelUnchecked(
                    window, (int)(cx64 - y), (int)(cy64 + x), color
                );

                putPixelUnchecked(
                    window, (int)(cx64 - x), (int)(cy64 - y), color
                );

                putPixelUnchecked(
                    window, (int)(cx64 + y), (int)(cy64 - x), color
                );

                /*
                * Les 4 autres ne sont nécessaires que
                * lorsqu'elles ne correspondent pas déjà
                * aux précédentes.
                */
                if (y != 0 && x != y) {
                    putPixelUnchecked(
                        window, (int)(cx64 + y), (int)(cy64 + x), color
                    );

                    putPixelUnchecked(
                        window, (int)(cx64 - x), (int)(cy64 + y), color
                    );

                    putPixelUnchecked(
                        window, (int)(cx64 - y), (int)(cy64 - x), color
                    );

                    putPixelUnchecked(
                        window, (int)(cx64 + x), (int)(cy64 - y), color
                    );
                }
            } else {
                /*
                * Même logique, mais avec clipping.
                */
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

            if (err <= 0) {
                y += 1;
                err += 2 * y + 1;
            }

            if (err > 0) {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
    }

    void gtDrawCircle(GtWindow* window, int cx, int cy, int radius, uint32_t color) {
        if (!window || !window->buffer || radius < 0) return;

        int64_t cx64 = cx;
        int64_t cy64 = cy;
        int64_t x = radius;
        int64_t y = 0;
        int64_t err = 0;

        while (x >= y) {
            /* Première ligne : toujours nécessaire. */
            drawHLineClipped(
                window,
                cy64 + y,
                cx64 - x,
                cx64 + x,
                color
            );

            /*
            * Lorsque y == 0, cy + y == cy - y :
            * ne pas dessiner deux fois la ligne centrale.
            */
            if (y != 0) {
                drawHLineClipped(
                    window,
                    cy64 - y,
                    cx64 - x,
                    cx64 + x,
                    color
                );
            }

            /*
            * Lorsque x == y, les lignes basées sur x
            * sont identiques à celles déjà tracées avec y.
            */
            if (x != y) {
                drawHLineClipped(
                    window,
                    cy64 + x,
                    cx64 - y,
                    cx64 + y,
                    color
                );

                drawHLineClipped(
                    window,
                    cy64 - x,
                    cx64 - y,
                    cx64 + y,
                    color
                );
            }

            if (err <= 0) {
                y += 1;
                err += 2 * y + 1;
            }

            if (err > 0) {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
    }

    /* -------------------------------------------------------------------------
    * ENTRÉES UTILISATEUR
    * ------------------------------------------------------------------------- */

    bool gtIsKeyDown(GtWindow* window, int keycode) {
        if (!window || keycode < 0 || keycode >= 256) return false;
        return window->keys[keycode];
    }

    bool gtIsMouseButtonDown(GtWindow* window, int button) {
        if (!window || button < 0 || button >= 3) return false;
        return window->mouse_buttons[button];
    }

    void gtGetMousePos(GtWindow* window, int* out_x, int* out_y) {
        if (!window) return;
        if (out_x) *out_x = window->mouse_x;
        if (out_y) *out_y = window->mouse_y;
    }

    #endif // LIBGT_IMPLEMENTATION
    #endif // LIBGT_H