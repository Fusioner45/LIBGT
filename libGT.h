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

// Remplit tout le framebuffer avec une couleur unie (format 0xAARRGGBB).
// Le canal alpha est conservé dans le framebuffer ; les primitives de dessin
// utilisent ensuite un compositing software source-over.
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

// Vérifie si une touche vient d'être pressée CETTE frame (edge detection)
// Retourne true seulement sur la frame où la touche passe de relâchée -> enfoncée
bool gtWasKeyPressed(GtWindow* window, int keycode);

// Vérifie si une touche vient d'être relâchée CETTE frame (edge detection)
// Retourne true seulement sur la frame où la touche passe de enfoncée -> relâchée
bool gtWasKeyReleased(GtWindow* window, int keycode);

// Vérifie si un bouton souris est enfoncé
// button : GT_MOUSE_BUTTON_LEFT, RIGHT ou MIDDLE
bool gtIsMouseButtonDown(GtWindow* window, int button);

// Vérifie si un bouton souris vient d'être pressé CETTE frame
bool gtWasMouseButtonPressed(GtWindow* window, int button);

// Vérifie si un bouton souris vient d'être relâché CETTE frame
bool gtWasMouseButtonReleased(GtWindow* window, int button);

// Récupère le delta de la molette souris depuis la dernière frame
// Positif = scroll vers le haut (loin de l'utilisateur), Négatif = scroll vers le bas
// Remet à zéro après lecture (consommer l'événement)
int gtGetMouseWheelDelta(GtWindow* window);

// Récupère la position actuelle de la souris (coordonnées client : 0,0 en haut-gauche)
// out_x, out_y : pointeurs vers int pour recevoir les coordonnées (peuvent être NULL)
void gtGetMousePos(GtWindow* window, int* out_x, int* out_y);

/* =========================================================================
* CHARGEMENT D'IMAGES (stb_image : PNG, BMP, JPG, TGA, PSD, GIF, HDR, PIC)
* =========================================================================
* Structure opaque pour une image chargée en mémoire (pixels ARGB 32bpp).
* L'utilisateur ne manipule que le pointeur GtImage* retourné par gtLoadImage.
* ========================================================================= */
typedef struct GtImage GtImage;

// Charge une image depuis un fichier (PNG, BMP, JPG, TGA, PSD, GIF, HDR, PIC)
// Retourne NULL en cas d'erreur (fichier introuvable, format invalide, OOM)
// L'image est convertie en ARGB 32bpp (4 canaux) peu importe le format source.
// width/height (sortie) reçoivent les dimensions en pixels.
GtImage* gtLoadImage(const char* filepath, int* out_width, int* out_height);

// Libère une image chargée par gtLoadImage
void gtFreeImage(GtImage* image);

// Accesseurs sur l'image
int       gtImageGetWidth(GtImage* image);
int       gtImageGetHeight(GtImage* image);
uint32_t* gtImageGetPixels(GtImage* image);  // Buffer ARGB 32bpp (top-down, stride = width*4)

/* =========================================================================
* RENDU DE TEXTE (Bitmap font 8x8 intégrée)
* =========================================================================
* Police bitmap monospace 8x8 (ASCII 32-126) intégrée dans la lib.
* Zéro dépendance externe, zéro allocation, dessin direct dans framebuffer.
* ========================================================================= */

// Dessine du texte à une position donnée (coin haut-gauche du premier caractère)
// color : format 0xAARRGGBB
void gtDrawText(GtWindow* window, int x, int y, const char* text, uint32_t color);

// Dessine du texte avec options avancées
// scale : facteur d'échelle (1.0 = 8x8 pixels par caractère, 2.0 = 16x16, etc.)
// spacing : espacement supplémentaire entre caractères (en pixels, peut être négatif)
// wrap_width : largeur max en pixels avant retour à la ligne (0 = pas de wrap)
void gtDrawTextEx(GtWindow* window, int x, int y, const char* text, uint32_t color,
                  float scale, int spacing, int wrap_width);

// Mesure la taille en pixels qu'occuperait un texte (sans le dessiner)
// out_width, out_height : dimensions du rectangle englobant
// Prend en compte scale, spacing, wrap_width comme gtDrawTextEx
void gtMeasureText(const char* text, float scale, int spacing, int wrap_width,
                   int* out_width, int* out_height);

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
* TIMERS / COROUTINES LÉGERS (gtSetTimeout / gtSetInterval)
* =========================================================================
* Timers pilotés par le JEU (delta time), pas par l'horloge réelle :
*   - Pause du jeu = pause automatique des cooldowns/animations.
*   - Déterministes : mêmes updates = même comportement (replays, tests).
*   - Zéro thread, zéro API Windows : pur C, utilisable sans fenêtre.
*
* Utilisation (dans la boucle de jeu, APRÈS gtBeginFrame) :
*   gtBeginFrame();
*   float dt = gtGetDeltaTime();
*   gtTimersUpdate(dt);   // ⚠ OBLIGATOIRE, 1x/frame : sans ça, RIEN ne se déclenche
*
*   gtSetTimeout(explode, 2.0f, bombe);              // une fois dans 2 s
*   uint32_t id = gtSetInterval(spawnEnemy, 1.5f, ctx); // toutes les 1.5 s
*   gtClearTimer(id);                               // annule (no-op si déjà mort)
*   gtClearAllTimers();                             // tout annuler (changement de scène)
*
* Garanties (codées et testées dans l'implémentation) :
*   - Un callback peut créer/annuler des timers PENDANT gtTimersUpdate.
*   - Un timer créé pendant une update ne se déclenche jamais dans la MÊME
*     update (il attend la suivante).
*   - Au plus 1 déclenchement par timer et par update, même après un gros
*     dt (pas de tempête de callbacks) ; le surplus est reporté (carry)
*     pour préserver la cadence de long terme.
*   - Les ids ne sont jamais recyclés : annuler un id périmé est un no-op.
*   - 0 = id invalide (création refusée : func NULL, délai négatif/NaN, pool plein).
*
* Capacité : GT_TIMERS_MAX (256 par défaut, surchargeable avant inclusion :
*   #define GT_TIMERS_MAX 512)
*
* Cas d'usage : spawn périodique, cooldown de tir, respawn différé,
* séquences d'animation, événements scriptés. C'est le mécanisme
* "coroutine" du gameplay, sans threads ni ucontext.
* ========================================================================= */

// Fonction appelée à l'échéance du timer.
// timer_id permet au callback de s'annuler lui-même : gtClearTimer(timer_id).
typedef void (*GtTimerFunc)(void* user_data, uint32_t timer_id);

// Programme UN déclenchement dans 'delay_seconds' (temps jeu, >= 0).
uint32_t gtSetTimeout(GtTimerFunc func, float delay_seconds, void* user_data);

// Programme un déclenchement toutes les 'interval_seconds', jusqu'à annulation.
uint32_t gtSetInterval(GtTimerFunc func, float interval_seconds, void* user_data);

// Annule un timer. true s'il était actif, false sinon (no-op sûr).
bool gtClearTimer(uint32_t timer_id);

// Annule TOUS les timers (changement de scène / fin de partie).
void gtClearAllTimers(void);

// Le timer est-il encore programmé ? (false pour 0 ou timer mort)
// Idiome cooldown : gtTimerActive(id) == "rechargement en cours ?"
bool gtTimerActive(uint32_t timer_id);

// Avance tous les timers de dt et déclenche les échéances.
// À appeler UNE fois par frame, juste après gtGetDeltaTime().
// Retourne le nombre de déclenchements de cette update (debug/tests).
int gtTimersUpdate(float dt);

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

// Dessin de texte via renderer
void gtRendererDrawText(GtRenderer* renderer, int x, int y, const char* text, GtColor color);
void gtRendererDrawTextEx(GtRenderer* renderer, int x, int y, const char* text, GtColor color,
                          float scale, int spacing, int wrap_width);

// Dessin d'image via renderer
// image : pointeur GtImage* retourné par gtLoadImage
// x, y : position coin haut-gauche
// tint : couleur multiplicative (GT_WHITE = pas de teinte)
void gtRendererDrawImage(GtRenderer* renderer, GtImage* image, int x, int y, GtColor tint);
void gtRendererDrawImageEx(GtRenderer* renderer, GtImage* image, int x, int y, int w, int h,
                           float rot, GtVec2 origin, bool flip_x, bool flip_y, GtColor tint);

/* =========================================================================
* INPUT MAPPING (Actions → Bindings, Gamepad, Profils Joueur, Rebinding)
* =========================================================================
* Architecture :
*   - Action : nom logique ("move_left", "jump", "shoot", "pause")
*   - Binding : lien Action ←→ Entrée physique (touche, souris, gamepad btn/axis)
*   - ActionMap : ensemble de bindings pour un contexte (gameplay, menu, UI)
*   - PlayerProfile : configuration complète d'un joueur (maps + sensibilités)
*   - InputContext : stack de maps actives (ex: Gameplay + PauseOverlay)
*
* Types d'entrées supportés :
*   - Clavier : codes VK_* (via window->keys[256])
*   - Souris : 3 boutons + position + molette (via window->mouse_*)
*   - Gamepad (XInput) : 4 manettes max, 14 boutons + 6 axes (sticks, triggers)
*   - Composite : une action = plusieurs bindings (OU logique)
* ========================================================================= */

// Types d'entrée physiques
typedef enum {
    GT_INPUT_NONE = 0,
    GT_INPUT_KEYBOARD,       // VK_* code dans data.keyboard.vk
    GT_INPUT_MOUSE_BUTTON,   // 0=left, 1=right, 2=middle dans data.mouse.button
    GT_INPUT_MOUSE_WHEEL,    // data.mouse.wheel_delta (positif = nord/haut)
    GT_INPUT_GAMEPAD_BUTTON, // data.gamepad.button (XINPUT_GAMEPAD_*)
    GT_INPUT_GAMEPAD_AXIS,   // data.gamepad.axis + seuil (0=LX,1=LY,2=RX,3=RY,4=LT,5=RT)
} GtInputType;

// Descripteur d'une entrée physique unique
typedef struct GtInputBinding {
    GtInputType type;
    uint8_t player_index;   // 0-3 pour gamepad, ignoré sinon
    union {
        struct { uint16_t vk; } keyboard;
        struct { uint8_t button; } mouse_button;
        struct { int16_t wheel_delta; } mouse_wheel;
        struct { uint16_t button; } gamepad_button;  // XINPUT_GAMEPAD_A, etc.
        struct { uint8_t axis; float threshold; } gamepad_axis;
    } data;
} GtInputBinding;

// Une action = nom + liste de bindings (OU logique : un seul suffit pour activer)
#define GT_MAX_BINDINGS_PER_ACTION 8
typedef struct {
    const char* name;                    // "move_left", "jump", etc.
    GtInputBinding bindings[GT_MAX_BINDINGS_PER_ACTION];
    int binding_count;
    bool is_pressed;                     // État calculé cette frame
    bool was_pressed;                    // État frame précédente
    float value;                         // 0.0-1.0 pour axes, 0/1 pour boutons
    float prev_value;
} GtAction;

// Map d'actions (contexte : gameplay, menu, etc.)
#define GT_MAX_ACTIONS_PER_MAP 64
typedef struct {
    const char* name;                    // "gameplay", "menu", "debug"
    GtAction actions[GT_MAX_ACTIONS_PER_MAP];
    int action_count;
    bool enabled;
} GtActionMap;

// Profil joueur complet
#define GT_MAX_MAPS_PER_PROFILE 8
#define GT_MAX_PLAYERS 4
typedef struct {
    int player_index;                    // 0-3
    GtActionMap maps[GT_MAX_MAPS_PER_PROFILE];
    int map_count;
    int active_map_stack[GT_MAX_MAPS_PER_PROFILE];
    int active_map_count;
    
    // Gamepad settings
    float stick_deadzone;                // 0.15f défaut (radial deadzone)
    float trigger_threshold;             // 0.1f défaut
    float vibration_strength;            // 1.0f défaut
    bool gamepad_connected;
    
    // Sensibilité souris (pour FPS etc.)
    float mouse_sensitivity;
    bool invert_y;
} GtPlayerProfile;

// Système d'input global (opaque, implémentation dans section LIBGT_IMPLEMENTATION)
typedef struct GtInputSystem GtInputSystem;

/* =========================================================================
* API PUBLIQUE - INPUT MAPPING
* ========================================================================= */

// Cycle de vie
GtInputSystem* gtInputCreate(void);
void           gtInputDestroy(GtInputSystem* input);

// Mise à jour (appeler APRÈS gtEventsWindow, AVANT logique de jeu)
void           gtInputUpdate(GtInputSystem* input, GtWindow* window, float dt);

// Profils joueurs
GtPlayerProfile* gtInputGetProfile(GtInputSystem* input, int player_index);
void             gtInputSetProfile(GtInputSystem* input, int player_index, const GtPlayerProfile* profile);

// Action Maps
GtActionMap*     gtInputCreateMap(const char* name);
void             gtInputDestroyMap(GtActionMap* map);
void             gtInputAddAction(GtActionMap* map, const char* action_name);
void             gtInputBindKey(GtActionMap* map, const char* action, uint16_t vk);
void             gtInputBindMouseBtn(GtActionMap* map, const char* action, uint8_t btn);
void             gtInputBindGamepadBtn(GtActionMap* map, const char* action, int player, uint16_t btn);
void             gtInputBindGamepadAxis(GtActionMap* map, const char* action, int player, uint8_t axis, float threshold);

// Stack de maps actives (push/pop pour overlay menu/pause)
void             gtInputPushMap(GtPlayerProfile* profile, GtActionMap* map);
void             gtInputPopMap(GtPlayerProfile* profile);
void             gtInputSetActiveMap(GtPlayerProfile* profile, GtActionMap* map); // clear + push

// Query état action (côté jeu)
bool             gtInputIsActionPressed(GtInputSystem* input, int player, const char* action);
bool             gtInputWasActionPressed(GtInputSystem* input, int player, const char* action);  // edge: pressed this frame
bool             gtInputWasActionReleased(GtInputSystem* input, int player, const char* action); // edge: released this frame
float            gtInputGetActionValue(GtInputSystem* input, int player, const char* action);    // valeur normalisée [-1,1] ou [0,1]
GtVec2           gtInputGetActionVec2(GtInputSystem* input, int player, const char* action_x, const char* action_y); // move 2D

// Rebinding runtime
bool             gtInputRebindAction(GtActionMap* map, const char* action, const GtInputBinding* new_binding);
void             gtInputClearActionBindings(GtActionMap* map, const char* action);

// Sérialisation (save/load profils - format binaire simple, sans dépendance)
bool             gtInputSaveProfile(const GtPlayerProfile* profile, const char* filepath);
bool             gtInputLoadProfile(GtPlayerProfile* profile, const char* filepath);

// Gamepad bas niveau (pour vibration, détection)
bool             gtInputIsGamepadConnected(int player_index);
void             gtInputSetVibration(int player_index, float left_motor, float right_motor, float duration);

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
#include <stdio.h>        // fopen, fclose, fread (pour stb_image)

// Le nom de classe est partagé par toutes les fenêtres du processus.
// On accepte ERROR_CLASS_ALREADY_EXISTS lors des créations suivantes.

/* -------------------------------------------------------------------------
* STB_IMAGE - Chargement d'images (PNG, BMP, JPG, TGA, PSD, GIF, HDR, PIC)
* -------------------------------------------------------------------------
* Implémentation intégrée (header-only) - zéro dépendance externe.
* Conversion automatique en ARGB 32bpp top-down.
* ------------------------------------------------------------------------- */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO  // On gère l'ouverture de fichier nous-mêmes
#define STBI_MALLOC(sz)       malloc(sz)
#define STBI_REALLOC(p, sz)   realloc(p, sz)
#define STBI_FREE(p)          free(p)
#include "stb_image.h"

/* -------------------------------------------------------------------------
* STRUCTURE INTERNE IMAGE (GtImage)
* ------------------------------------------------------------------------- */
struct GtImage {
    int width;
    int height;
    int channels;        // Toujours 4 (ARGB) après chargement
    uint32_t* pixels;    // Buffer ARGB 32bpp, top-down (stride = width * 4)
};

/* -------------------------------------------------------------------------
* POLICE BITMAP 8x8 INTÉGRÉE (ASCII 32-126)
* -------------------------------------------------------------------------
* Police monospace 8x8, 95 caractères (espace à ~).
* Stockée comme 95x8 octets (1 bit par pixel, 8 lignes par char).
* Zéro allocation, zéro dépendance.
* ------------------------------------------------------------------------- */
static const uint8_t gt_font8x8[95 * 8] = {
    // ' ' (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // '!' (33)
    0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00,
    // '"' (34)
    0x6C, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00,
    // '#' (35)
    0x18, 0x7E, 0x7E, 0x18, 0x7E, 0x7E, 0x18, 0x00,
    // '$' (36)
    0x3C, 0x66, 0x3C, 0x7E, 0x66, 0x3C, 0x66, 0x3C,
    // '%' (37)
    0x00, 0x63, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x63,
    // '&' (38)
    0x3C, 0x66, 0x6E, 0x3A, 0x4E, 0x66, 0x3A, 0x00,
    // ''' (39)
    0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    // '(' (40)
    0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00,
    // ')' (41)
    0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00,
    // '*' (42)
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    // '+' (43)
    0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00,
    // ',' (44)
    0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00,
    // '-' (45)
    0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00,
    // '.' (46)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
    // '/' (47)
    0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00,
    // '0' (48)
    0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00,
    // '1' (49)
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00,
    // '2' (50)
    0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00,
    // '3' (51)
    0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00,
    // '4' (52)
    0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00,
    // '5' (53)
    0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00,
    // '6' (54)
    0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00,
    // '7' (55)
    0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00,
    // '8' (56)
    0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00,
    // '9' (57)
    0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00,
    // ':' (58)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
    // ';' (59)
    0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30,
    // '<' (60)
    0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00,
    // '=' (61)
    0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00,
    // '>' (62)
    0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x00,
    // '?' (63)
    0x3C, 0x66, 0x06, 0x1C, 0x18, 0x00, 0x18, 0x00,
    // '@' (64)
    0x3C, 0x66, 0x6E, 0x7A, 0x7E, 0x60, 0x3C, 0x00,
    // 'A' (65)
    0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00,
    // 'B' (66)
    0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00,
    // 'C' (67)
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00,
    // 'D' (68)
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00,
    // 'E' (69)
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00,
    // 'F' (70)
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00,
    // 'G' (71)
    0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00,
    // 'H' (72)
    0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
    // 'I' (73)
    0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // 'J' (74)
    0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00,
    // 'K' (75)
    0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00,
    // 'L' (76)
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,
    // 'M' (77)
    0x66, 0xEE, 0xFE, 0xFE, 0xD6, 0x66, 0x66, 0x00,
    // 'N' (78)
    0x66, 0xE6, 0xF6, 0xDE, 0xCE, 0x6E, 0x66, 0x00,
    // 'O' (79)
    0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,
    // 'P' (80)
    0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00,
    // 'Q' (81)
    0x3C, 0x66, 0x66, 0x66, 0x6E, 0x6C, 0x3A, 0x00,
    // 'R' (82)
    0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00,
    // 'S' (83)
    0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00,
    // 'T' (84)
    0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // 'U' (85)
    0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,
    // 'V' (86)
    0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00,
    // 'W' (87)
    0x66, 0x66, 0x66, 0xD6, 0xFE, 0xFE, 0x6C, 0x00,
    // 'X' (88)
    0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00,
    // 'Y' (89)
    0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x00,
    // 'Z' (90)
    0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00,
    // '[' (91)
    0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
    // '\' (92)
    0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00,
    // ']' (93)
    0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00,
    // '^' (94)
    0x18, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
    // '_' (95)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    // '`' (96)
    0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 'a' (97)
    0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00,
    // 'b' (98)
    0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00,
    // 'c' (99)
    0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x00,
    // 'd' (100)
    0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00,
    // 'e' (101)
    0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00,
    // 'f' (102)
    0x0C, 0x18, 0x7C, 0x18, 0x18, 0x18, 0x18, 0x00,
    // 'g' (103)
    0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x3C, 0x00,
    // 'h' (104)
    0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00,
    // 'i' (105)
    0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // 'j' (106)
    0x0C, 0x00, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x38,
    // 'k' (107)
    0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00,
    // 'l' (108)
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00,
    // 'm' (109)
    0x00, 0x00, 0xAC, 0xD6, 0xD6, 0xD6, 0xD6, 0x00,
    // 'n' (110)
    0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00,
    // 'o' (111)
    0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00,
    // 'p' (112)
    0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60,
    // 'q' (113)
    0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06,
    // 'r' (114)
    0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00,
    // 's' (115)
    0x00, 0x00, 0x3C, 0x60, 0x3C, 0x06, 0x7C, 0x00,
    // 't' (116)
    0x18, 0x18, 0x7C, 0x18, 0x18, 0x1A, 0x0C, 0x00,
    // 'u' (117)
    0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00,
    // 'v' (118)
    0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00,
    // 'w' (119)
    0x00, 0x00, 0x66, 0x66, 0xD6, 0xFE, 0x6C, 0x00,
    // 'x' (120)
    0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00,
    // 'y' (121)
    0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C,
    // 'z' (122)
    0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00,
    // '{' (123)
    0x0C, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0C, 0x00,
    // '|' (124)
    0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
    // '}' (125)
    0x30, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x30, 0x00,
    // '~' (126)
    0x00, 0x00, 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00,
};

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

    bool keys[256];              // État clavier : true = enfoncée (index = code VK)
    bool keys_prev[256];         // État clavier frame précédente (pour edge detection)
    bool mouse_buttons[3];       // État souris : [0]=gauche, [1]=droit, [2]=milieu
    bool mouse_buttons_prev[3];  // État souris frame précédente (pour edge detection)
    int  mouse_x;                // Position X souris relative zone cliente
    int  mouse_y;                // Position Y souris relative zone cliente
    int  mouse_wheel_delta;      // Delta molette accumulé cette frame (reset après lecture)
    bool mouse_tracking;         // Suivi WM_MOUSELEAVE actif (TrackMouseEvent)
};

/* -------------------------------------------------------------------------
* RASTERISATION BAS NIVEAU (Accès direct mémoire, SANS clipping)
* -------------------------------------------------------------------------
* ATTENTION : L'appelant DOIT garantir que les coordonnées sont valides !
* Ces fonctions sont utilisées internement par les primitives avec clipping.
* ------------------------------------------------------------------------- */

// Composite une couleur source ARGB sur une destination du framebuffer.
// Le framebuffer affiché par GDI est traité comme opaque : après compositing,
// le pixel de sortie a toujours alpha = 255.
static inline uint32_t gtBlendPixel(uint32_t dst, uint32_t src) {
    uint32_t sa = (src >> 24) & 0xFFu;
    if (sa == 0) return dst;
    if (sa == 255) return src | 0xFF000000u;

    uint32_t sr = (src >> 16) & 0xFFu;
    uint32_t sg = (src >> 8)  & 0xFFu;
    uint32_t sb =  src        & 0xFFu;

    uint32_t dr = (dst >> 16) & 0xFFu;
    uint32_t dg = (dst >> 8)  & 0xFFu;
    uint32_t db =  dst        & 0xFFu;

    uint32_t inv = 255u - sa;
    uint32_t r = (sr * sa + dr * inv + 127u) / 255u;
    uint32_t g = (sg * sa + dg * inv + 127u) / 255u;
    uint32_t b = (sb * sa + db * inv + 127u) / 255u;

    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

// Écrit un pixel unique directement dans le buffer (sans vérification).
static inline void putPixelUnchecked(GtWindow* window, int x, int y, uint32_t color) {
    size_t index = (size_t)y * (size_t)window->width + (size_t)x;
    window->buffer[index] = gtBlendPixel(window->buffer[index], color);
}

// Trace une ligne horizontale continue en mémoire (optimisée : pas de recalcul d'index)
static inline void rasterizeHLineUnchecked(GtWindow* window, int y, int x1, int x2, uint32_t color) {
    size_t row_start = (size_t)y * (size_t)window->width;
    if (((color >> 24) & 0xFFu) == 255u) {
        uint32_t opaque = color | 0xFF000000u;
        for (int x = x1; x <= x2; x++) {
            window->buffer[row_start + (size_t)x] = opaque;
        }
        return;
    }

    for (int x = x1; x <= x2; x++) {
        size_t index = row_start + (size_t)x;
        window->buffer[index] = gtBlendPixel(window->buffer[index], color);
    }
}

// Trace une ligne verticale continue (saut de ligne par 'stride' = width)
static inline void rasterizeVLineUnchecked(GtWindow* window, int x, int y1, int y2, uint32_t color) {
    size_t stride = (size_t)window->width;
    size_t index = (size_t)y1 * stride + (size_t)x;

    if (((color >> 24) & 0xFFu) == 255u) {
        uint32_t opaque = color | 0xFF000000u;
        for (int y = y1; y <= y2; y++) {
            window->buffer[index] = opaque;
            index += stride;
        }
        return;
    }

    for (int y = y1; y <= y2; y++) {
        window->buffer[index] = gtBlendPixel(window->buffer[index], color);
        index += stride;
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
        if (win && wParam < 256) win->keys[wParam] = true;
        break;

    case WM_SYSKEYDOWN:   // Touche système (Alt+...) enfoncée
        if (win && wParam < 256) win->keys[wParam] = true;
        // Laisser DefWindowProc gérer les raccourcis système (notamment Alt+F4).
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    case WM_KEYUP:        // Touche relâchée
        if (win && wParam < 256) win->keys[wParam] = false;
        break;

    case WM_SYSKEYUP:     // Touche système (Alt+...) relâchée
        if (win && wParam < 256) win->keys[wParam] = false;
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    // ===== SOURIS - BOUTONS =====
    case WM_LBUTTONDOWN:  // Clic gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = true;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_LBUTTONUP:    // Relâche gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = false;
            if (!win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] &&
                GetCapture() == hwnd) {
                ReleaseCapture();
            }
        }
        break;

    case WM_RBUTTONDOWN:  // Clic droit
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = true;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_RBUTTONUP:
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = false;
            if (!win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] &&
                GetCapture() == hwnd) {
                ReleaseCapture();
            }
        }
        break;

    case WM_MBUTTONDOWN:  // Clic molette
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = true;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_MBUTTONUP:
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = false;
            if (!win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] &&
                !win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] &&
                GetCapture() == hwnd) {
                ReleaseCapture();
            }
        }
        break;

    // ===== SOURIS - MOLETTE =====
    case WM_MOUSEWHEEL:
        if (win) {
            // HIWORD(wParam) contient le delta signé (positif = vers le haut/loin de l'utilisateur)
            // WHEEL_DELTA = 120 par crant
            win->mouse_wheel_delta += (short)HIWORD(wParam);
        }
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
                memset(win->mouse_buttons_prev, 0, sizeof(win->mouse_buttons_prev));
                if (GetCapture() == hwnd) ReleaseCapture();
            }
            break;

        case WM_CAPTURECHANGED: // Capture perdue (autre fenêtre l'a prise)
            if (win) {
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons));
                memset(win->mouse_buttons_prev, 0, sizeof(win->mouse_buttons_prev));
                win->mouse_tracking = false;
            }
            break;

        case WM_KILLFOCUS:    // Fenêtre perd le focus clavier
            if (win) {
                memset(win->keys, 0, sizeof(win->keys));            // Relâche toutes touches
                memset(win->keys_prev, 0, sizeof(win->keys_prev));  // Reset prev aussi
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons)); // Relâche souris
                memset(win->mouse_buttons_prev, 0, sizeof(win->mouse_buttons_prev));
                win->mouse_x = -1;  // Souris considérée comme hors fenêtre
                win->mouse_y = -1;
                win->mouse_wheel_delta = 0;
                win->mouse_tracking = false;
                if (GetCapture() == hwnd) ReleaseCapture();
            }
            break;

    // ===== FERMETURE =====
    case WM_CLOSE:        // Utilisateur clique sur X ou Alt+F4
        if (win) win->should_close = true;  // Destruction différée par l'appelant
        break;

    case WM_DESTROY:
        if (win) win->should_close = true;
        break;

    case WM_NCDESTROY:
        // Évite de laisser un pointeur dangling dans GWLP_USERDATA.
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        // Le framebuffer est entièrement contrôlé par libGT : pas besoin d'effacer
        // le fond avec GDI, ce qui évite le flicker lors des redimensionnements.
        return 1;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam); // Traitement par défaut Windows
    }
    return 0; // Message traité
}

// Enregistre la classe de fenêtre si nécessaire. La classe reste enregistrée
// jusqu'à la fin du processus pour supporter plusieurs GtWindow proprement.
static bool gtRegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = GtWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LIBGTWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (RegisterClassA(&wc)) return true;
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

// Instanciation de la fenêtre et allocation mémoire
// title : titre affiché dans la barre de fenêtre
// width, height : dimensions de la ZONE CLIENTE (sans bordures/titre)
// Retourne : pointeur GtWindow* opaque, ou NULL si erreur
GtWindow* gtCreateWindow(const char* title, int width, int height) {
    // Validation basique des paramètres.
    if (!title || width <= 0 || height <= 0) return NULL;
    // Protection contre overflow size_t (width * height * 4 bytes)
    if ((size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) return NULL;

    // Alloue la structure fenêtre (zéro-initialisée via calloc)
    GtWindow* win = (GtWindow*)calloc(1, sizeof(GtWindow));
    if (!win) return NULL;

    win->width = width;
    win->height = height;
    win->should_close = false;
    win->hInstance = GetModuleHandle(NULL);  // Instance du processus courant
    if (!win->hInstance) {
        free(win);
        return NULL;
    }

    // Enregistre la classe fenêtre Win32 si nécessaire.
    if (!gtRegisterWindowClass(win->hInstance)) {
        free(win);
        return NULL;
    }
    // Calcule la taille fenêtre complète (avec bordures/titre) depuis zone cliente
    RECT rect = {0, 0, width, height};
    if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
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

    // La classe Win32 reste enregistrée pendant la durée du processus.
    // Cela permet plusieurs fenêtres et évite de désenregistrer une classe
    // encore utilisée par une autre GtWindow.
}

// Traite la file de messages Windows (non-bloquant)
// À appeler une fois par frame dans la boucle principale
// Utilise PeekMessage (pas GetMessage) pour ne pas bloquer si pas de message
// Retourne false si WM_QUIT reçu (application doit quitter)
bool gtEventsWindow(GtWindow* window) {
    if (!window) return false;

    // Copie l'état courant vers prev AVANT de traiter les nouveaux messages
    // (pour edge detection : pressed/released cette frame)
    memcpy(window->keys_prev, window->keys, sizeof(window->keys));
    memcpy(window->mouse_buttons_prev, window->mouse_buttons, sizeof(window->mouse_buttons));

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

    int result = StretchDIBits(
        hdc,
        0, 0, window->width, window->height,  // Destination (écran)
        0, 0, window->width, window->height,  // Source (framebuffer)
        window->buffer, &window->bmi, DIB_RGB_COLORS, SRCCOPY
    );

    (void)result;
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

        if (((color >> 24) & 0xFFu) == 255u) {
            uint32_t opaque = color | 0xFF000000u;
            for (int col = 0; col < count; col++) {
                row_ptr[col] = opaque;
            }
        } else if (((color >> 24) & 0xFFu) != 0u) {
            for (int col = 0; col < count; col++) {
                row_ptr[col] = gtBlendPixel(row_ptr[col], color);
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

// Récupère la position actuelle de la souris (coordonnées client : 0,0 = haut-gauche)
// out_x, out_y : pointeurs vers int pour recevoir les coordonnées (peuvent être NULL)
void gtGetMousePos(GtWindow* window, int* out_x, int* out_y) {
    if (!window) return;
    if (out_x) *out_x = window->mouse_x;
    if (out_y) *out_y = window->mouse_y;
}

// Vérifie si une touche vient d'être pressée CETTE frame (edge detection)
// Retourne true seulement sur la frame où la touche passe de relâchée -> enfoncée
bool gtWasKeyPressed(GtWindow* window, int keycode) {
    if (!window || keycode < 0 || keycode >= 256) return false;
    return window->keys[keycode] && !window->keys_prev[keycode];
}

// Vérifie si une touche vient d'être relâchée CETTE frame (edge detection)
// Retourne true seulement sur la frame où la touche passe de enfoncée -> relâchée
bool gtWasKeyReleased(GtWindow* window, int keycode) {
    if (!window || keycode < 0 || keycode >= 256) return false;
    return !window->keys[keycode] && window->keys_prev[keycode];
}

// Vérifie si un bouton souris vient d'être pressé CETTE frame
bool gtWasMouseButtonPressed(GtWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return window->mouse_buttons[button] && !window->mouse_buttons_prev[button];
}

// Vérifie si un bouton souris vient d'être relâché CETTE frame
bool gtWasMouseButtonReleased(GtWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return !window->mouse_buttons[button] && window->mouse_buttons_prev[button];
}

// Récupère le delta de la molette souris depuis la dernière frame
// Positif = scroll vers le haut (loin de l'utilisateur), Négatif = scroll vers le bas
// Remet à zéro après lecture (consommer l'événement)
int gtGetMouseWheelDelta(GtWindow* window) {
    if (!window) return 0;
    int delta = window->mouse_wheel_delta;
    window->mouse_wheel_delta = 0;
    return delta;
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
static bool g_gtFirstFrame = false;         // true entre l'init et le 1er gtBeginFrame

// Initialisation paresseuse (lazy init) : appelée au premier gtGetTime/gtBeginFrame
static bool gtTimeInit(void) {
    if (g_gtFrequency.QuadPart != 0) return true;

    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        return false;
    }
    if (!QueryPerformanceCounter(&start)) {
        return false;
    }

    g_gtFrequency = frequency;
    g_gtStartTime = start;
    g_gtLastFrame = start;
    g_gtFirstFrame = true;
    return true;
}

// Temps total écoulé depuis gtTimeInit() (secondes, double précision)
// Utile pour animations temps-réel, timestamps, profiling
double gtGetTime(void) {
    if (!gtTimeInit()) return 0.0;
    LARGE_INTEGER now;
    if (!QueryPerformanceCounter(&now)) return 0.0;
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
    if (!gtTimeInit()) {
        g_gtDeltaTime = 0.0f;
        return;
    }

    LARGE_INTEGER now;
    if (!QueryPerformanceCounter(&now)) {
        g_gtDeltaTime = 0.0f;
        return;
    }

    // Première frame : pas de frame précédente -> dt = 0 (contrat documenté).
    // (Sans ce cas, dt vaudrait les quelques µs entre l'init et la mesure.)
    if (g_gtFirstFrame) {
        g_gtFirstFrame = false;
        g_gtDeltaTime = 0.0f;
        g_gtLastFrame = now;
        return;
    }

    double dt = (double)(now.QuadPart - g_gtLastFrame.QuadPart) / (double)g_gtFrequency.QuadPart;
    if (dt < 0.0) dt = 0.0;
    g_gtDeltaTime = (float)dt;
    g_gtLastFrame = now;
}

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : TIMERS (pool fixe, piloté par delta time)
* -------------------------------------------------------------------------
* Choix d'implémentation :
*   - Pool fixe (pas de malloc par timer) : cohérent avec la philosophie
*     "close du hardware" de libGT, et friendly cache pour l'update.
*   - born_tick : n° de l'update pendant laquelle un timer a été créé.
*     Sert à ignorer les timers créés PENDANT l'update courante (ils
*     n'ont pas encore "vécu" une frame complète).
*   - Ids strictement croissants, jamais recyclés : gtClearTimer() sur un
*     id périmé ne peut JAMAIS toucher un nouveau timer.
*   - Réentrance : les callbacks peuvent créer/annuler des timers pendant
*     l'itération. Sécurisé par (1) slots stables (pas de swap-remove
*     pendant la boucle), (2) re-vérification id+active après chaque
*     callback, (3) born_tick pour les nouveaux.
* ------------------------------------------------------------------------- */
#ifndef GT_TIMERS_MAX
#define GT_TIMERS_MAX 256
#endif

typedef struct {
    bool        active;       // Slot occupé
    bool        repeating;   // false = one-shot (timeout), true = interval
    float       interval;    // Délai/période en secondes (>= 0)
    float       elapsed;     // Temps accumulé depuis l'armement / dernier tir
    uint32_t    id;          // Identifiant unique, jamais recyclé
    uint32_t    born_tick;   // Update de création (anti déclenchement immédiat)
    GtTimerFunc func;
    void*       user_data;
} GtTimerSlot;

static GtTimerSlot g_gtTimers[GT_TIMERS_MAX];
static uint32_t    g_gtTimersNextId = 1;  // 0 réservé = id invalide
static uint32_t    g_gtTimersTick   = 0;  // Compteur d'updates écoulées

static uint32_t gtTimerCreate(GtTimerFunc func, float seconds, void* user_data, bool repeating) {
    if (!func || seconds < 0.0f || !isfinite(seconds)) return 0;
    for (int i = 0; i < GT_TIMERS_MAX; i++) {
        if (g_gtTimers[i].active) continue;
        GtTimerSlot* t = &g_gtTimers[i];
        t->active     = true;
        t->repeating  = repeating;
        t->interval   = seconds;
        t->elapsed    = 0.0f;
        t->id         = g_gtTimersNextId++;
        if (g_gtTimersNextId == 0) g_gtTimersNextId = 1;  // wrap 32 bits
        t->born_tick  = g_gtTimersTick;
        t->func       = func;
        t->user_data  = user_data;
        return t->id;
    }
    return 0;  // Pool plein
}

uint32_t gtSetTimeout(GtTimerFunc func, float delay_seconds, void* user_data) {
    return gtTimerCreate(func, delay_seconds, user_data, false);
}

uint32_t gtSetInterval(GtTimerFunc func, float interval_seconds, void* user_data) {
    return gtTimerCreate(func, interval_seconds, user_data, true);
}

// Recherche par id (uniquement parmi les slots ACTIFS)
static GtTimerSlot* gtTimerFind(uint32_t timer_id) {
    if (timer_id == 0) return NULL;
    for (int i = 0; i < GT_TIMERS_MAX; i++)
        if (g_gtTimers[i].active && g_gtTimers[i].id == timer_id)
            return &g_gtTimers[i];
    return NULL;
}

bool gtClearTimer(uint32_t timer_id) {
    GtTimerSlot* t = gtTimerFind(timer_id);
    if (!t) return false;
    t->active = false;  // Le slot sera réutilisé plus tard ; l'id, jamais.
    return true;
}

void gtClearAllTimers(void) {
    for (int i = 0; i < GT_TIMERS_MAX; i++) g_gtTimers[i].active = false;
}

bool gtTimerActive(uint32_t timer_id) {
    return gtTimerFind(timer_id) != NULL;
}

int gtTimersUpdate(float dt) {
    if (dt < 0.0f || !isfinite(dt)) dt = 0.0f;
    g_gtTimersTick++;  // Les timers créés pendant CETTE update naissent avec ce n°
    int fired = 0;

    for (int i = 0; i < GT_TIMERS_MAX; i++) {
        GtTimerSlot* t = &g_gtTimers[i];
        if (!t->active) continue;
        if (t->born_tick == g_gtTimersTick) continue;  // Né cette update : attend la suivante

        t->elapsed += dt;
        if (t->elapsed < t->interval) continue;

        fired++;
        uint32_t id = t->id;
        t->func(t->user_data, t->id);   // ⚠ Le callback peut tout casser ici :
                                        //   se clear, clearAll, créer des timers...
        // Re-vérification : le slot héberge-t-il ENCORE le même timer ?
        // (si le callback s'est cancellé puis le slot a été réutilisé
        // immédiatement par un gtSetTimeout, t->id a changé -> ne pas toucher)
        if (!t->active || t->id != id) continue;

        if (t->repeating) {
            t->elapsed -= t->interval;  // Carry : préserve la cadence de long terme
        } else {
            t->active = false;           // One-shot : terminé
        }
    }
    return fired;
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
    renderer->type = (type == GT_RENDERER_D2D || type == GT_RENDERER_GDI)
                   ? type : GT_RENDERER_GDI;

    if (renderer->type == GT_RENDERER_D2D) {
        // D2D non implémenté : fallback silencieux vers GDI.
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

// --- Primitives via le renderer ---
// Le backend D2D est actuellement rabattu vers GDI dans gtCreateRenderer(),
// donc toutes les opérations passent ici par le framebuffer CPU.
void gtRendererClear(GtRenderer* renderer, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtClearWindow(renderer->window, color);
    }
}

void gtRendererDrawPixel(GtRenderer* renderer, int x, int y, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawPixel(renderer->window, x, y, color);
    }
}

void gtRendererDrawRect(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawRect(renderer->window, x, y, w, h, color);
    }
}

void gtRendererDrawRectLines(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawRectLines(renderer->window, x, y, w, h, color);
    }
}

void gtRendererDrawLine(GtRenderer* renderer, int x1, int y1, int x2, int y2, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawLine(renderer->window, x1, y1, x2, y2, color);
    }
}

void gtRendererDrawCircle(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawCircle(renderer->window, cx, cy, radius, color);
    }
}

void gtRendererDrawCircleLines(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawCircleLines(renderer->window, cx, cy, radius, color);
    }
}

// Dessin de texte via renderer
void gtRendererDrawText(GtRenderer* renderer, int x, int y, const char* text, GtColor color) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawText(renderer->window, x, y, text, color);
    }
}

void gtRendererDrawTextEx(GtRenderer* renderer, int x, int y, const char* text, GtColor color,
                          float scale, int spacing, int wrap_width) {
    if (!renderer || !renderer->window) return;
    if (renderer->type == GT_RENDERER_GDI) {
        gtDrawTextEx(renderer->window, x, y, text, color, scale, spacing, wrap_width);
    }
}

// Dessin d'image via renderer
void gtRendererDrawImage(GtRenderer* renderer, GtImage* image, int x, int y, GtColor tint) {
    if (!renderer || !renderer->window || !image) return;
    if (renderer->type == GT_RENDERER_GDI) {
        // Pour GDI, on blit directement les pixels de l'image
        int img_w = gtImageGetWidth(image);
        int img_h = gtImageGetHeight(image);
        uint32_t* img_pixels = gtImageGetPixels(image);
        if (!img_pixels) return;

        GtWindow* win = renderer->window;
        // Clipping
        int x1 = x < 0 ? 0 : x;
        int y1 = y < 0 ? 0 : y;
        int x2 = (x + img_w > win->width) ? win->width : x + img_w;
        int y2 = (y + img_h > win->height) ? win->height : y + img_h;

        int src_x1 = x1 - x;
        int src_y1 = y1 - y;

        if (x1 >= x2 || y1 >= y2) return;

        size_t stride = (size_t)win->width;
        size_t img_stride = (size_t)img_w;

        // Alpha blending simple
        uint8_t ta = (tint >> 24) & 0xFFu;
        if (ta == 0) return; // Invisible

        for (int row = y1; row < y2; row++) {
            uint32_t* dst_row = &win->buffer[(size_t)row * stride + (size_t)x1];
            uint32_t* src_row = &img_pixels[(size_t)(src_y1 + row - y1) * img_stride + (size_t)src_x1];
            int count = x2 - x1;

            if (ta == 255) {
                // Pas de teinte, copie directe avec blend sur fond
                for (int col = 0; col < count; col++) {
                    uint32_t src = src_row[col];
                    uint32_t sa = (src >> 24) & 0xFFu;
                    if (sa == 255) {
                        dst_row[col] = src | 0xFF000000u;
                    } else if (sa > 0) {
                        dst_row[col] = gtBlendPixel(dst_row[col], src);
                    }
                }
            } else {
                // Avec teinte multiplicative
                for (int col = 0; col < count; col++) {
                    uint32_t src = src_row[col];
                    uint32_t sa = (src >> 24) & 0xFFu;
                    if (sa == 0) continue;

                    // Applique teinte sur la source
                    uint32_t sr = ((src >> 16) & 0xFFu) * ((tint >> 16) & 0xFFu) / 255;
                    uint32_t sg = ((src >> 8) & 0xFFu) * ((tint >> 8) & 0xFFu) / 255;
                    uint32_t sb = (src & 0xFFu) * (tint & 0xFFu) / 255;
                    uint32_t blended_src = (sa << 24) | (sr << 16) | (sg << 8) | sb;

                    dst_row[col] = gtBlendPixel(dst_row[col], blended_src);
                }
            }
        }
    }
}

void gtRendererDrawImageEx(GtRenderer* renderer, GtImage* image, int x, int y, int w, int h,
                           float rot, GtVec2 origin, bool flip_x, bool flip_y, GtColor tint) {
    // Pour l'instant, version simplifiée sans rotation/flip/origin
    // TODO: implémenter transformation complète
    (void)rot; (void)origin; (void)flip_x; (void)flip_y;
    if (!renderer || !renderer->window || !image) return;
    if (renderer->type == GT_RENDERER_GDI) {
        // Draw scaled
        int img_w = gtImageGetWidth(image);
        int img_h = gtImageGetHeight(image);
        uint32_t* img_pixels = gtImageGetPixels(image);
        if (!img_pixels) return;

        GtWindow* win = renderer->window;

        if (w <= 0 || h <= 0) return;

        // Simple nearest-neighbor scaling avec clipping
        for (int dy = 0; dy < h; dy++) {
            int src_y = (int)((float)dy * img_h / h);
            if (src_y < 0) src_y = 0;
            if (src_y >= img_h) src_y = img_h - 1;

            int dst_y = y + dy;
            if (dst_y < 0 || dst_y >= win->height) continue;

            uint32_t* dst_row = &win->buffer[(size_t)dst_y * (size_t)win->width];
            for (int dx = 0; dx < w; dx++) {
                int src_x = (int)((float)dx * img_w / w);
                if (src_x < 0) src_x = 0;
                if (src_x >= img_w) src_x = img_w - 1;

                int dst_x = x + dx;
                if (dst_x < 0 || dst_x >= win->width) continue;

                uint32_t src = img_pixels[(size_t)src_y * (size_t)img_w + (size_t)src_x];
                uint32_t sa = (src >> 24) & 0xFFu;
                if (sa == 0) continue;

                if (((tint >> 24) & 0xFFu) != 255) {
                    // Applique teinte
                    uint32_t sr = ((src >> 16) & 0xFFu) * ((tint >> 16) & 0xFFu) / 255;
                    uint32_t sg = ((src >> 8) & 0xFFu) * ((tint >> 8) & 0xFFu) / 255;
                    uint32_t sb = (src & 0xFFu) * (tint & 0xFFu) / 255;
                    src = (sa << 24) | (sr << 16) | (sg << 8) | sb;
                }

                dst_row[(size_t)dst_x] = gtBlendPixel(dst_row[(size_t)dst_x], src);
            }
        }
    }
}

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : CHARGEMENT D'IMAGES (stb_image)
* ------------------------------------------------------------------------- */
GtImage* gtLoadImage(const char* filepath, int* out_width, int* out_height) {
    if (!filepath) return NULL;

    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(f);
        return NULL;
    }

    unsigned char* file_data = (unsigned char*)malloc((size_t)file_size);
    if (!file_data) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(file_data, 1, (size_t)file_size, f);
    fclose(f);

    if (read_bytes != (size_t)file_size) {
        free(file_data);
        return NULL;
    }

    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(file_data, (int)file_size, &w, &h, &channels, 4);
    free(file_data);

    if (!pixels) return NULL;

    // Alloue GtImage
    GtImage* image = (GtImage*)malloc(sizeof(GtImage));
    if (!image) {
        stbi_image_free(pixels);
        return NULL;
    }

    // Convertit en ARGB 32bpp (stbi charge en RGBA)
    uint32_t* argb_pixels = (uint32_t*)malloc((size_t)w * (size_t)h * sizeof(uint32_t));
    if (!argb_pixels) {
        stbi_image_free(pixels);
        free(image);
        return NULL;
    }

    // stbi charge en RGBA (R,G,B,A), on veut ARGB (A,R,G,B) pour GDI
    for (int i = 0; i < w * h; i++) {
        uint8_t r = pixels[i * 4 + 0];
        uint8_t g = pixels[i * 4 + 1];
        uint8_t b = pixels[i * 4 + 2];
        uint8_t a = pixels[i * 4 + 3];
        argb_pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    stbi_image_free(pixels);

    image->width = w;
    image->height = h;
    image->channels = 4;
    image->pixels = argb_pixels;

    if (out_width) *out_width = w;
    if (out_height) *out_height = h;

    return image;
}

void gtFreeImage(GtImage* image) {
    if (!image) return;
    if (image->pixels) free(image->pixels);
    free(image);
}

int gtImageGetWidth(GtImage* image) {
    if (!image) return 0;
    return image->width;
}

int gtImageGetHeight(GtImage* image) {
    if (!image) return 0;
    return image->height;
}

uint32_t* gtImageGetPixels(GtImage* image) {
    if (!image) return NULL;
    return image->pixels;
}

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : RENDU DE TEXTE (Bitmap font 8x8)
* ------------------------------------------------------------------------- */
static inline void gtDrawChar(GtWindow* window, int x, int y, char c, uint32_t color, float scale) {
    if (c < 32 || c > 126) return;  // Hors police (ASCII 32-126)

    int idx = (int)c - 32;
    const uint8_t* glyph = &gt_font8x8[idx * 8];

    if (scale == 1.0f) {
        // Version optimisée 1x
        for (int row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            int py = y + row;
            if (py < 0 || py >= window->height) continue;

            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) {
                    int px = x + col;
                    if (px >= 0 && px < window->width) {
                        drawPixelClipped64(window, px, py, color);
                    }
                }
            }
        }
    } else {
        // Version mise à l'échelle
        for (int row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) {
                    int px_start = x + (int)(col * scale);
                    int px_end = x + (int)((col + 1) * scale);
                    int py_start = y + (int)(row * scale);
                    int py_end = y + (int)((row + 1) * scale);

                    for (int py = py_start; py < py_end; py++) {
                        if (py < 0 || py >= window->height) continue;
                        for (int px = px_start; px < px_end; px++) {
                            if (px >= 0 && px < window->width) {
                                drawPixelClipped64(window, px, py, color);
                            }
                        }
                    }
                }
            }
        }
    }
}

void gtDrawText(GtWindow* window, int x, int y, const char* text, uint32_t color) {
    gtDrawTextEx(window, x, y, text, color, 1.0f, 0, 0);
}

void gtDrawTextEx(GtWindow* window, int x, int y, const char* text, uint32_t color,
                  float scale, int spacing, int wrap_width) {
    if (!window || !window->buffer || !text) return;
    if (scale <= 0.0f) scale = 1.0f;

    int cur_x = x;
    int cur_y = y;
    int char_w = (int)(8 * scale);
    int line_h = (int)(8 * scale) + spacing;

    for (const char* p = text; *p; p++) {
        char c = *p;

        // Gestion saut de ligne explicite
        if (c == '\n') {
            cur_x = x;
            cur_y += line_h;
            continue;
        }

        // Word wrap
        if (wrap_width > 0 && c == ' ') {
            // Regarde si le mot suivant tient sur la ligne
            const char* next = p + 1;
            int word_w = 0;
            while (*next && *next != ' ' && *next != '\n') {
                word_w += char_w + spacing;
                next++;
            }
            if (cur_x + word_w > x + wrap_width) {
                cur_x = x;
                cur_y += line_h;
            }
        }

        gtDrawChar(window, cur_x, cur_y, c, color, scale);
        cur_x += char_w + spacing;
    }
}

void gtMeasureText(const char* text, float scale, int spacing, int wrap_width,
                   int* out_width, int* out_height) {
    if (!text) {
        if (out_width) *out_width = 0;
        if (out_height) *out_height = 0;
        return;
    }
    if (scale <= 0.0f) scale = 1.0f;

    int char_w = (int)(8 * scale);
    int line_h = (int)(8 * scale) + spacing;
    int cur_x = 0;
    int cur_y = 0;
    int max_w = 0;

    for (const char* p = text; *p; p++) {
        char c = *p;

        if (c == '\n') {
            if (cur_x > max_w) max_w = cur_x;
            cur_x = 0;
            cur_y += line_h;
            continue;
        }

        if (wrap_width > 0 && c == ' ') {
            const char* next = p + 1;
            int word_w = 0;
            while (*next && *next != ' ' && *next != '\n') {
                word_w += char_w + spacing;
                next++;
            }
            if (cur_x + word_w > wrap_width) {
                if (cur_x > max_w) max_w = cur_x;
                cur_x = 0;
                cur_y += line_h;
            }
        }

        cur_x += char_w + spacing;
    }

    if (cur_x > max_w) max_w = cur_x;
    if (cur_x > 0) cur_y += line_h;  // Dernière ligne

    if (out_width) *out_width = max_w;
    if (out_height) *out_height = cur_y;
}

/* =========================================================================
* IMPORTANT : fin du PREMIER bloc LIBGT_IMPLEMENTATION (fenêtre, dessin,
* input, temps, renderer). L'API ECS ci-dessous est PUBLIQUE : elle doit
* rester visible pour tous les fichiers qui incluent libGT.h sans définir
* LIBGT_IMPLEMENTATION (projet multi-fichiers).
* Un second bloc #ifdef LIBGT_IMPLEMENTATION (implémentation ECS) est
* ré ouvert plus bas.
* ========================================================================= */
#endif // LIBGT_IMPLEMENTATION

/* =========================================================================
* ECS MINIMAL (Entity Component System léger)
* =========================================================================
* Architecture "Archetype" simplifiée : tableaux de composants compacts (SoA).
* Entité = index + génération (pour réutilisation sûre après destruction).
* Systèmes = fonctions itérant sur les composants requis.
*
* Utilisation typique :
*   GtECS ecs = {0};
*   gtECSInit(&ecs, 1024);           // Capacité max entités
*   GtEntity player = gtECSCreateEntity(&ecs);
*   gtECSAddTransform(&ecs, player, gtVec2(400,300), 0, 1);
*   gtECSAddSprite(&ecs, player, myImage, GT_WHITE);
*   gtECSAddVelocity(&ecs, player, gtVec2(0,0));
*
*   while (running) {
*       gtECSSystemVelocity(&ecs, dt);      // Met à jour positions
*       gtECSSystemCollisions(&ecs);        // Détection collisions
*       gtECSSystemRender(&ecs, renderer);  // Dessin
*   }
*   gtECSDestroy(&ecs);
* ========================================================================= */

// --- Types de base ---

// Identifiant d'entité 64 bits : 32 bits d'index + 32 bits de génération.
// alive[] distingue l'absence d'entité d'une génération valide.
// La génération ne revient pratiquement jamais pendant la durée de vie
// normale d'un jeu (plus de 4 milliards de recyclages d'un même index).
typedef uint64_t GtEntity;
#define GT_ENTITY_NULL ((GtEntity)UINT64_MAX)
#define GT_ENTITY_INDEX(e) ((uint32_t)((e) & UINT64_C(0xFFFFFFFF)))
#define GT_ENTITY_GEN(e)   ((uint32_t)(((e) >> 32) & UINT64_C(0xFFFFFFFF)))
#define GT_ENTITY_MAKE(idx, gen) \
    ((((GtEntity)(uint32_t)(gen)) << 32) | (GtEntity)(uint32_t)(idx))

// Masques de composants (bitset 32 bits = max 32 types de composants)
typedef uint32_t GtComponentMask;
#define GT_COMP_NONE      0
#define GT_COMP_TRANSFORM (1u << 0)   // Position, rotation, scale
#define GT_COMP_SPRITE    (1u << 1)   // Image, couleur, layer
#define GT_COMP_VELOCITY  (1u << 2)   // Vitesse linéaire/angulaire
#define GT_COMP_COLLIDER  (1u << 3)   // Box/Cercle collision
#define GT_COMP_HEALTH    (1u << 4)   // PV, invincibilité
#define GT_COMP_PLAYER    (1u << 5)   // Contrôlé par joueur
#define GT_COMP_ENEMY     (1u << 6)   // IA simple
#define GT_COMP_LIFETIME  (1u << 7)   // Auto-destruction après temps
#define GT_COMP_CUSTOM_0  (1u << 8)   // Réservé utilisateur
#define GT_COMP_CUSTOM_1  (1u << 9)
#define GT_COMP_CUSTOM_2  (1u << 10)
#define GT_COMP_CUSTOM_3  (1u << 11)

// --- Composants (Structure of Arrays) ---

typedef struct {
    GtVec2 pos;      // Position monde (pixels)
    float rot;       // Rotation (radians)
    float scale;     // Échelle uniforme
} GtTransform;

typedef struct {
    GtImage* image;    // Image chargée via gtLoadImage
    GtColor tint;      // Teinte multiplicative
    int layer;         // Ordre de rendu (plus haut = devant)
    GtVec2 origin;     // Ancrage relatif [0..1] : (0,0)=haut-gauche, (0.5,0.5)=centre, (1,1)=bas-droite
    GtVec2 size;       // Taille source dans l'image (0,0 = image complète)
    bool flip_x;       // Miroir horizontal
    bool flip_y;       // Miroir vertical
} GtSprite;

typedef struct {
    GtVec2 linear;   // Vitesse pixels/seconde
    float angular;   // Vitesse angulaire rad/seconde
} GtVelocity;

typedef struct {
    enum { GT_COLLIDER_NONE, GT_COLLIDER_CIRCLE, GT_COLLIDER_BOX } type;
    union {
        struct { float radius; } circle;
        struct { GtVec2 half_extents; } box;
    };
    GtVec2 offset;   // Décalage relatif au transform
    uint32_t layer;  // Couche de collision (bitmask)
    uint32_t mask;   // Avec quelles couches collider
    bool trigger;    // Si true : détection seulement, pas de réponse physique
} GtCollider;

typedef struct {
    int current;
    int max;
    float invincible_timer;  // Secondes d'invincibilité après coup
} GtHealth;

typedef struct {
    float timer;     // Temps restant avant destruction
} GtLifetime;

// --- Stockage ECS ---

#define GT_ECS_MAX_ENTITIES_DEFAULT 4096

typedef struct GtECS {
    // Configuration
    int capacity;          // Max entités (puissance de 2 recommandé)
    
    // État entités
    uint32_t* generations; // Génération 32 bits par index [capacity]
    bool* alive;           // true si l'index correspond à une entité vivante
    int alive_count;        // Nombre d'entités vivantes
    int free_list_head;    // Tête liste libre (index next free, -1 = plein)
    int* free_list;        // Indices libres [capacity]
    
    // Masques composants par entité
    GtComponentMask* masks;  // [capacity]
    
    // Pools composants (SoA - Structure of Arrays)
    GtTransform* transforms;   // [capacity]
    GtSprite* sprites;         // [capacity]
    GtVelocity* velocities;    // [capacity]
    GtCollider* colliders;     // [capacity]
    GtHealth* healths;         // [capacity]
    GtLifetime* lifetimes;     // [capacity]
    
    // Callbacks optionnels (pour extension utilisateur)
    void (*on_entity_created)(struct GtECS*, GtEntity);
    void (*on_entity_destroyed)(struct GtECS*, GtEntity);
} GtECS;

// --- API Cycle de vie ECS ---

// Initialise l'ECS avec capacité donnée (arrondie à puissance de 2 sup)
void gtECSInit(GtECS* ecs, int capacity);

// Détruit l'ECS et libère toute la mémoire
void gtECSDestroy(GtECS* ecs);

// Crée une nouvelle entité, retourne GT_ENTITY_NULL si capacité atteinte
GtEntity gtECSCreateEntity(GtECS* ecs);

// Détruit une entité (libère composants, recycle index)
void gtECSDestroyEntity(GtECS* ecs, GtEntity entity);

// Vérifie si entité vivante et valide (génération correspond)
bool gtECSIsAlive(const GtECS* ecs, GtEntity entity);

// Récupère le masque de composants d'une entité
GtComponentMask gtECSGetMask(const GtECS* ecs, GtEntity entity);

// --- API Composants (Get/Add/Remove) ---

// Transform (requis pour la plupart des entités visibles)
GtTransform* gtECSGetTransform(const GtECS* ecs, GtEntity entity);
GtTransform* gtECSAddTransform(GtECS* ecs, GtEntity entity, GtVec2 pos, float rot, float scale);
void gtECSRemoveTransform(GtECS* ecs, GtEntity entity);

// Sprite
GtSprite* gtECSGetSprite(const GtECS* ecs, GtEntity entity);
GtSprite* gtECSAddSprite(GtECS* ecs, GtEntity entity, GtImage* image, GtColor tint, int layer);
void gtECSRemoveSprite(GtECS* ecs, GtEntity entity);

// Velocity
GtVelocity* gtECSGetVelocity(const GtECS* ecs, GtEntity entity);
GtVelocity* gtECSAddVelocity(GtECS* ecs, GtEntity entity, GtVec2 linear, float angular);
void gtECSRemoveVelocity(GtECS* ecs, GtEntity entity);

// Collider
GtCollider* gtECSGetCollider(const GtECS* ecs, GtEntity entity);
GtCollider* gtECSAddCircleCollider(GtECS* ecs, GtEntity entity, float radius, GtVec2 offset, uint32_t layer, uint32_t mask);
GtCollider* gtECSAddBoxCollider(GtECS* ecs, GtEntity entity, GtVec2 half_extents, GtVec2 offset, uint32_t layer, uint32_t mask);
void gtECSRemoveCollider(GtECS* ecs, GtEntity entity);

// Health
GtHealth* gtECSGetHealth(const GtECS* ecs, GtEntity entity);
GtHealth* gtECSAddHealth(GtECS* ecs, GtEntity entity, int max_hp);
void gtECSRemoveHealth(GtECS* ecs, GtEntity entity);

// Lifetime (auto-destruction)
GtLifetime* gtECSGetLifetime(const GtECS* ecs, GtEntity entity);
GtLifetime* gtECSAddLifetime(GtECS* ecs, GtEntity entity, float seconds);
void gtECSRemoveLifetime(GtECS* ecs, GtEntity entity);

// --- API Systèmes (itération optimisée) ---

// Fonction utilisateur appelée pour chaque entité ayant les composants requis
// mask = masque de composants REQUIS (tous doivent être présents)
// user_data = pointeur utilisateur passé à chaque appel
typedef void (*GtECSSystemFunc)(GtECS* ecs, GtEntity entity, void* user_data);

// Exécute un système sur toutes entités ayant 'mask'
void gtECSRunSystem(GtECS* ecs, GtComponentMask mask, GtECSSystemFunc func, void* user_data);

// Systèmes built-in communs
void gtECSSystemVelocity(GtECS* ecs, float dt);                    // pos += vel * dt
void gtECSSystemLifetime(GtECS* ecs, float dt);                    // Décrémente lifetime, détruit si <= 0
void gtECSSystemHealthInvincibility(GtECS* ecs, float dt);         // Décrémente invincibilité
void gtECSSystemRenderSprites(GtECS* ecs, GtRenderer* renderer);   // Dessine sprites avec transform

// Callback collision : deux entités + user_data
typedef void (*GtECSCollisionFunc)(GtECS* ecs, GtEntity a, GtEntity b, void* user_data);
void gtECSSystemCollisions(GtECS* ecs, GtECSCollisionFunc on_hit, void* user_data);  // Broad + narrow phase

// --- Utilitaires collision ---

// Test collision cercle-cercle (positions monde = transform.pos + collider.offset)
bool gtECSCheckCircleCircle(const GtECS* ecs, GtEntity a, GtEntity b);
// Test collision box-box (AABB)
bool gtECSCheckBoxBox(const GtECS* ecs, GtEntity a, GtEntity b);
// Test collision cercle-box
bool gtECSCheckCircleBox(const GtECS* ecs, GtEntity circle_e, GtEntity box_e);

// Dégâts génériques (gère invincibilité, callbacks)
bool gtECSDamageEntity(GtECS* ecs, GtEntity entity, int amount);

// --- Tags & recherche (rôles joueur/ennemi) ---
// Un "tag" est un composant SANS donnée : il ne sert qu'au filtrage.
// C'est ce qui rend GT_COMP_PLAYER / GT_COMP_ENEMY / GT_COMP_CUSTOM_* utilisables
// depuis le code jeu (gtECSAddComp est interne à l'implémentation).
//   gtECSAddTag(&ecs, player, GT_COMP_PLAYER);
//   gtECSRunSystem(&ecs, GT_COMP_ENEMY | GT_COMP_TRANSFORM, sysEnemyChase, &ctx);
bool gtECSAddTag(GtECS* ecs, GtEntity entity, GtComponentMask tag);
bool gtECSRemoveTag(GtECS* ecs, GtEntity entity, GtComponentMask tag);

// Première entité vivante possédant TOUS les bits de 'mask', sinon GT_ENTITY_NULL.
// Cas d'usage : retrouver le joueur pour l'IA/HUD sans stocker de variable globale.
GtEntity gtECSFindFirstByMask(const GtECS* ecs, GtComponentMask mask);

// Détruit toutes les entités vivantes dont la santé est <= 0.
// À appeler APRÈS gtECSSystemCollisions (les dégâts y mettent current à 0).
// Retourne le nombre d'entités détruites.
int gtECSSystemHealthDeath(GtECS* ecs);

/* =========================================================================
* IMPLÉMENTATION : ECS MINIMAL
* ========================================================================= */
#ifdef LIBGT_IMPLEMENTATION

// Arrondit à la puissance de 2 supérieure sans overflow.
static bool gtNextPow2(int v, int* out) {
    if (!out || v <= 0) return false;

    size_t n = 1;
    const size_t max_entities = 0x01000000u; // Limite pratique actuelle : 16M slots
    while (n < (size_t)v) {
        n <<= 1;
        if (n > max_entities) return false;
    }

    *out = (int)n;
    return true;
}

static bool gtECSAllocateArrays(GtECS* ecs, int capacity) {
    ecs->generations = (uint32_t*)calloc((size_t)capacity, sizeof(*ecs->generations));
    ecs->alive = (bool*)calloc((size_t)capacity, sizeof(*ecs->alive));
    ecs->free_list = (int*)malloc((size_t)capacity * sizeof(*ecs->free_list));
    ecs->masks = (GtComponentMask*)calloc((size_t)capacity, sizeof(*ecs->masks));
    ecs->transforms = (GtTransform*)calloc((size_t)capacity, sizeof(*ecs->transforms));
    ecs->sprites = (GtSprite*)calloc((size_t)capacity, sizeof(*ecs->sprites));
    ecs->velocities = (GtVelocity*)calloc((size_t)capacity, sizeof(*ecs->velocities));
    ecs->colliders = (GtCollider*)calloc((size_t)capacity, sizeof(*ecs->colliders));
    ecs->healths = (GtHealth*)calloc((size_t)capacity, sizeof(*ecs->healths));
    ecs->lifetimes = (GtLifetime*)calloc((size_t)capacity, sizeof(*ecs->lifetimes));

    if (!ecs->generations || !ecs->alive || !ecs->free_list || !ecs->masks ||
        !ecs->transforms || !ecs->sprites || !ecs->velocities ||
        !ecs->colliders || !ecs->healths || !ecs->lifetimes) {
        return false;
    }

    return true;
}

void gtECSInit(GtECS* ecs, int capacity) {
    if (!ecs) return;

    // Permet de réinitialiser proprement un ECS déjà initialisé.
    if (ecs->capacity > 0 && ecs->generations && ecs->alive && ecs->free_list) {
        gtECSDestroy(ecs);
    } else {
        memset(ecs, 0, sizeof(*ecs));
    }

    if (capacity <= 0) capacity = GT_ECS_MAX_ENTITIES_DEFAULT;

    int rounded_capacity = 0;
    if (!gtNextPow2(capacity, &rounded_capacity)) return;
    capacity = rounded_capacity;

    GtECS tmp = {0};
    tmp.capacity = capacity;
    tmp.alive_count = 0;
    tmp.free_list_head = 0;

    if (!gtECSAllocateArrays(&tmp, capacity)) {
        gtECSDestroy(&tmp);
        memset(ecs, 0, sizeof(*ecs));
        return;
    }

    for (int i = 0; i < capacity; i++) {
        tmp.free_list[i] = (i + 1 < capacity) ? (i + 1) : -1;
        tmp.generations[i] = 1u;
    }

    tmp.on_entity_created = NULL;
    tmp.on_entity_destroyed = NULL;
    *ecs = tmp;
}

void gtECSDestroy(GtECS* ecs) {
    if (!ecs) return;

    free(ecs->generations);
    free(ecs->alive);
    free(ecs->free_list);
    free(ecs->masks);
    free(ecs->transforms);
    free(ecs->sprites);
    free(ecs->velocities);
    free(ecs->colliders);
    free(ecs->healths);
    free(ecs->lifetimes);

    memset(ecs, 0, sizeof(*ecs));
}

GtEntity gtECSCreateEntity(GtECS* ecs) {
    if (!ecs || !ecs->generations || !ecs->alive || !ecs->free_list) return GT_ENTITY_NULL;
    if (ecs->free_list_head < 0 || ecs->alive_count >= ecs->capacity) return GT_ENTITY_NULL;

    int idx = ecs->free_list_head;
    if (idx < 0 || idx >= ecs->capacity) return GT_ENTITY_NULL;

    ecs->free_list_head = ecs->free_list[idx];
    ecs->alive[idx] = true;
    ecs->masks[idx] = GT_COMP_NONE;
    ecs->alive_count++;

    uint32_t gen = ecs->generations[idx];
    GtEntity entity = GT_ENTITY_MAKE(idx, gen);

    if (ecs->on_entity_created) ecs->on_entity_created(ecs, entity);
    return entity;
}

void gtECSDestroyEntity(GtECS* ecs, GtEntity entity) {
    if (!ecs || !ecs->generations || !ecs->alive || !ecs->free_list) return;

    int idx = GT_ENTITY_INDEX(entity);
    if (entity == GT_ENTITY_NULL || idx >= ecs->capacity) return;
    if (!ecs->alive[idx]) return;
    if (ecs->generations[idx] != GT_ENTITY_GEN(entity)) return;

    GtEntity destroyed_entity = entity;
    ecs->alive[idx] = false;
    ecs->masks[idx] = GT_COMP_NONE;

    // Incrémente la génération. En cas d'overflow 32 bits, on évite la valeur 0.
    // La présence du tableau alive[] empêche qu'une entité morte soit
    // considérée vivante même si sa génération coïncide avec une ancienne handle.
    uint32_t next_gen = ecs->generations[idx] + 1u;
    if (next_gen == 0) next_gen = 1;
    ecs->generations[idx] = next_gen;

    ecs->free_list[idx] = ecs->free_list_head;
    ecs->free_list_head = idx;
    if (ecs->alive_count > 0) ecs->alive_count--;

    if (ecs->on_entity_destroyed) ecs->on_entity_destroyed(ecs, destroyed_entity);
}

bool gtECSIsAlive(const GtECS* ecs, GtEntity entity) {
    if (!ecs || !ecs->generations || !ecs->alive) return false;
    if (entity == GT_ENTITY_NULL) return false;

    uint32_t idx_u = GT_ENTITY_INDEX(entity);
    if (idx_u >= (uint32_t)ecs->capacity) return false;

    int idx = (int)idx_u;
    return ecs->alive[idx] && ecs->generations[idx] == GT_ENTITY_GEN(entity);
}

GtComponentMask gtECSGetMask(const GtECS* ecs, GtEntity entity) {
    if (!gtECSIsAlive(ecs, entity) || !ecs->masks) return GT_COMP_NONE;
    return ecs->masks[GT_ENTITY_INDEX(entity)];
}

// --- Helpers internes ---

static inline bool gtECSValidateEntity(const GtECS* ecs, GtEntity entity, int* out_idx) {
    if (!gtECSIsAlive(ecs, entity)) return false;
    int idx = (int)GT_ENTITY_INDEX(entity);
    if (out_idx) *out_idx = idx;
    return true;
}

static inline bool gtECSHasComp(const GtECS* ecs, int idx, GtComponentMask comp) {
    return ecs && ecs->masks && idx >= 0 && idx < ecs->capacity &&
           (ecs->masks[idx] & comp) == comp;
}

static inline void gtECSAddComp(GtECS* ecs, int idx, GtComponentMask comp) {
    ecs->masks[idx] |= comp;
}

static inline void gtECSRemoveComp(GtECS* ecs, int idx, GtComponentMask comp) {
    ecs->masks[idx] &= ~comp;
}

// --- Composants : Transform ---

GtTransform* gtECSGetTransform(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_TRANSFORM)) return NULL;
    return &ecs->transforms[idx];
}

GtTransform* gtECSAddTransform(GtECS* ecs, GtEntity entity, GtVec2 pos, float rot, float scale) {
    if (!ecs) return NULL;
    if (!isfinite(pos.x) || !isfinite(pos.y) || !isfinite(rot) || !isfinite(scale)) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtTransform* t = &ecs->transforms[idx];
    t->pos = pos;
    t->rot = rot;
    t->scale = scale;
    gtECSAddComp(ecs, idx, GT_COMP_TRANSFORM);
    return t;
}

void gtECSRemoveTransform(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_TRANSFORM);
}

// --- Composants : Sprite ---

GtSprite* gtECSGetSprite(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_SPRITE)) return NULL;
    return &ecs->sprites[idx];
}

GtSprite* gtECSAddSprite(GtECS* ecs, GtEntity entity, GtImage* image, GtColor tint, int layer) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtSprite* s = &ecs->sprites[idx];
    s->image = image;
    s->tint = tint;
    s->layer = layer;
    s->origin = gtVec2(0.5f, 0.5f);  // Centre par défaut
    s->size = gtVec2(0, 0);          // Image complète
    s->flip_x = false;
    s->flip_y = false;
    gtECSAddComp(ecs, idx, GT_COMP_SPRITE);
    return s;
}

void gtECSRemoveSprite(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_SPRITE);
}

// --- Composants : Velocity ---

GtVelocity* gtECSGetVelocity(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_VELOCITY)) return NULL;
    return &ecs->velocities[idx];
}

GtVelocity* gtECSAddVelocity(GtECS* ecs, GtEntity entity, GtVec2 linear, float angular) {
    if (!ecs || !isfinite(linear.x) || !isfinite(linear.y) || !isfinite(angular)) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtVelocity* v = &ecs->velocities[idx];
    v->linear = linear;
    v->angular = angular;
    gtECSAddComp(ecs, idx, GT_COMP_VELOCITY);
    return v;
}

void gtECSRemoveVelocity(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_VELOCITY);
}

// --- Composants : Collider ---

GtCollider* gtECSGetCollider(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_COLLIDER)) return NULL;
    return &ecs->colliders[idx];
}

GtCollider* gtECSAddCircleCollider(GtECS* ecs, GtEntity entity, float radius, GtVec2 offset, uint32_t layer, uint32_t mask) {
    if (!ecs || radius < 0.0f || !isfinite(radius) || !isfinite(offset.x) || !isfinite(offset.y)) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtCollider* c = &ecs->colliders[idx];
    c->type = GT_COLLIDER_CIRCLE;
    c->circle.radius = radius;
    c->offset = offset;
    c->layer = layer;
    c->mask = mask;
    c->trigger = false;
    gtECSAddComp(ecs, idx, GT_COMP_COLLIDER);
    return c;
}

GtCollider* gtECSAddBoxCollider(GtECS* ecs, GtEntity entity, GtVec2 half_extents, GtVec2 offset, uint32_t layer, uint32_t mask) {
    if (!ecs || half_extents.x < 0.0f || half_extents.y < 0.0f ||
        !isfinite(half_extents.x) || !isfinite(half_extents.y) ||
        !isfinite(offset.x) || !isfinite(offset.y)) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtCollider* c = &ecs->colliders[idx];
    c->type = GT_COLLIDER_BOX;
    c->box.half_extents = half_extents;
    c->offset = offset;
    c->layer = layer;
    c->mask = mask;
    c->trigger = false;
    gtECSAddComp(ecs, idx, GT_COMP_COLLIDER);
    return c;
}

void gtECSRemoveCollider(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_COLLIDER);
}

// --- Composants : Health ---

GtHealth* gtECSGetHealth(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_HEALTH)) return NULL;
    return &ecs->healths[idx];
}

GtHealth* gtECSAddHealth(GtECS* ecs, GtEntity entity, int max_hp) {
    if (!ecs || max_hp < 0) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtHealth* h = &ecs->healths[idx];
    h->current = max_hp;
    h->max = max_hp;
    h->invincible_timer = 0.0f;
    gtECSAddComp(ecs, idx, GT_COMP_HEALTH);
    return h;
}

void gtECSRemoveHealth(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_HEALTH);
}

// --- Composants : Lifetime ---

GtLifetime* gtECSGetLifetime(const GtECS* ecs, GtEntity entity) {
    if (!ecs) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;
    if (!gtECSHasComp(ecs, idx, GT_COMP_LIFETIME)) return NULL;
    return &ecs->lifetimes[idx];
}

GtLifetime* gtECSAddLifetime(GtECS* ecs, GtEntity entity, float seconds) {
    if (!ecs || !isfinite(seconds)) return NULL;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return NULL;

    GtLifetime* l = &ecs->lifetimes[idx];
    l->timer = seconds;
    gtECSAddComp(ecs, idx, GT_COMP_LIFETIME);
    return l;
}

void gtECSRemoveLifetime(GtECS* ecs, GtEntity entity) {
    if (!ecs) return;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return;
    gtECSRemoveComp(ecs, idx, GT_COMP_LIFETIME);
}

// --- Système générique ---

void gtECSRunSystem(GtECS* ecs, GtComponentMask mask, GtECSSystemFunc func, void* user_data) {
    if (!ecs || !func) return;
    if (!ecs->generations || !ecs->alive || !ecs->masks) return;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        GtEntity entity = GT_ENTITY_MAKE(idx, ecs->generations[idx]);
        if ((ecs->masks[idx] & mask) == mask) {
            func(ecs, entity, user_data);
        }
    }
}

// --- Systèmes built-in ---

void gtECSSystemVelocity(GtECS* ecs, float dt) {
    if (!ecs) return;
    GtComponentMask mask = GT_COMP_TRANSFORM | GT_COMP_VELOCITY;
    if (!ecs->alive || !ecs->masks) return;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if ((ecs->masks[idx] & mask) == mask) {
            GtTransform* t = &ecs->transforms[idx];
            GtVelocity* v = &ecs->velocities[idx];
            t->pos = gtVec2Add(t->pos, gtVec2Mul(v->linear, dt));
            t->rot += v->angular * dt;
        }
    }
}

void gtECSSystemLifetime(GtECS* ecs, float dt) {
    if (!ecs) return;
    if (!ecs->alive || !ecs->masks) return;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if (gtECSHasComp(ecs, idx, GT_COMP_LIFETIME)) {
            GtLifetime* l = &ecs->lifetimes[idx];
            l->timer -= dt;
            if (l->timer <= 0.0f) {
                GtEntity entity = GT_ENTITY_MAKE(idx, ecs->generations[idx]);
                gtECSDestroyEntity(ecs, entity);
            }
        }
    }
}

void gtECSSystemHealthInvincibility(GtECS* ecs, float dt) {
    if (!ecs) return;
    if (!ecs->alive || !ecs->masks) return;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if (gtECSHasComp(ecs, idx, GT_COMP_HEALTH)) {
            GtHealth* h = &ecs->healths[idx];
            if (h->invincible_timer > 0.0f) {
                h->invincible_timer -= dt;
                if (h->invincible_timer < 0.0f) h->invincible_timer = 0.0f;
            }
        }
    }
}

// --- Tags, recherche & mort ---

bool gtECSAddTag(GtECS* ecs, GtEntity entity, GtComponentMask tag) {
    if (!ecs || tag == GT_COMP_NONE) return false;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return false;
    ecs->masks[idx] |= tag;
    return true;
}

bool gtECSRemoveTag(GtECS* ecs, GtEntity entity, GtComponentMask tag) {
    if (!ecs || tag == GT_COMP_NONE) return false;
    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return false;
    ecs->masks[idx] &= ~tag;
    return true;
}

GtEntity gtECSFindFirstByMask(const GtECS* ecs, GtComponentMask mask) {
    if (!ecs || !ecs->alive || !ecs->masks || !ecs->generations) return GT_ENTITY_NULL;
    if (mask == GT_COMP_NONE) return GT_ENTITY_NULL; // évite "n'importe quelle entité"
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (ecs->alive[idx] && (ecs->masks[idx] & mask) == mask) {
            return GT_ENTITY_MAKE(idx, ecs->generations[idx]);
        }
    }
    return GT_ENTITY_NULL;
}

int gtECSSystemHealthDeath(GtECS* ecs) {
    if (!ecs || !ecs->alive || !ecs->masks || !ecs->generations) return 0;
    int destroyed = 0;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if (!gtECSHasComp(ecs, idx, GT_COMP_HEALTH)) continue;
        if (ecs->healths[idx].current <= 0) {
            // Le handle porte la génération COURANTE lue à la volée : valide.
            gtECSDestroyEntity(ecs, GT_ENTITY_MAKE(idx, ecs->generations[idx]));
            destroyed++;
        }
    }
    return destroyed;
}

// Utilitaire : calcul rect monde d'un sprite
static inline void gtECSGetSpriteRect(const GtECS* ecs, int idx, float* out_x, float* out_y, float* out_w, float* out_h) {
    const GtTransform* t = &ecs->transforms[idx];
    const GtSprite* s = &ecs->sprites[idx];
    // Note: taille réelle nécessite GtImage, ici on suppose taille = 32x32 par défaut
    // L'utilisateur doit surcharger size dans le sprite s'il veut une taille précise
    float w = (s->size.x > 0) ? s->size.x : 32.0f;
    float h = (s->size.y > 0) ? s->size.y : 32.0f;
    float scale = fabsf(t->scale);
    w *= scale;
    h *= scale;
    *out_x = t->pos.x - w * s->origin.x;
    *out_y = t->pos.y - h * s->origin.y;
    *out_w = w;
    *out_h = h;
}

void gtECSSystemRenderSprites(GtECS* ecs, GtRenderer* renderer) {
    if (!ecs || !renderer || !ecs->alive || !ecs->masks ||
        !ecs->transforms || !ecs->sprites) return;
    GtComponentMask mask = GT_COMP_TRANSFORM | GT_COMP_SPRITE;

    // Collecte entités visibles pour tri par layer.
    // Allocation dynamique : aucun débordement même si capacity > 4096.
    typedef struct { int idx; int layer; } RenderItem;
    if (ecs->alive_count <= 0) return;

    RenderItem* items = (RenderItem*)malloc((size_t)ecs->alive_count * sizeof(*items));
    if (!items) return;

    int count = 0;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if ((ecs->masks[idx] & mask) == mask) {
            items[count].idx = idx;
            items[count].layer = ecs->sprites[idx].layer;
            count++;
        }
    }

    // Tri stable par insertion : suffisamment léger pour une petite liste et
    // conserve l'ordre des indices en cas de layer identique.
    for (int i = 1; i < count; i++) {
        RenderItem key = items[i];
        int j = i - 1;
        while (j >= 0 && items[j].layer > key.layer) {
            items[j + 1] = items[j];
            j--;
        }
        items[j + 1] = key;
    }

    // Dessine dans l'ordre
    for (int i = 0; i < count; i++) {
        int idx = items[i].idx;
        GtSprite* s = &ecs->sprites[idx];

        // Sans GtImage pour l'instant : dessine un rectangle coloré comme placeholder
        // L'utilisateur doit implémenter son propre système de rendu avec GtImage
        float x, y, w, h;
        gtECSGetSpriteRect(ecs, idx, &x, &y, &w, &h);

        GtColor color = s->tint;
        gtRendererDrawRect(renderer, (int)x, (int)y, (int)w, (int)h, color);
        gtRendererDrawRectLines(renderer, (int)x, (int)y, (int)w, (int)h, GT_WHITE);
    }

    free(items);
}

// --- Collision : utilitaires ---

static inline GtVec2 gtECSGetWorldPos(const GtECS* ecs, int idx) {
    GtVec2 pos = ecs->transforms[idx].pos;
    if (gtECSHasComp(ecs, idx, GT_COMP_COLLIDER)) {
        pos = gtVec2Add(pos, ecs->colliders[idx].offset);
    }
    return pos;
}

bool gtECSCheckCircleCircle(const GtECS* ecs, GtEntity a, GtEntity b) {
    if (!ecs) return false;
    int idx_a, idx_b;
    if (!gtECSValidateEntity(ecs, a, &idx_a) || !gtECSValidateEntity(ecs, b, &idx_b)) return false;
    if (!gtECSHasComp(ecs, idx_a, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (!gtECSHasComp(ecs, idx_b, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (ecs->colliders[idx_a].type != GT_COLLIDER_CIRCLE) return false;
    if (ecs->colliders[idx_b].type != GT_COLLIDER_CIRCLE) return false;

    GtVec2 pos_a = gtECSGetWorldPos(ecs, idx_a);
    GtVec2 pos_b = gtECSGetWorldPos(ecs, idx_b);
    float r_a = ecs->colliders[idx_a].circle.radius * fabsf(ecs->transforms[idx_a].scale);
    float r_b = ecs->colliders[idx_b].circle.radius * fabsf(ecs->transforms[idx_b].scale);
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist2 = dx * dx + dy * dy;
    float r_sum = r_a + r_b;
    return dist2 <= r_sum * r_sum;
}

bool gtECSCheckBoxBox(const GtECS* ecs, GtEntity a, GtEntity b) {
    if (!ecs) return false;
    int idx_a, idx_b;
    if (!gtECSValidateEntity(ecs, a, &idx_a) || !gtECSValidateEntity(ecs, b, &idx_b)) return false;
    if (!gtECSHasComp(ecs, idx_a, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (!gtECSHasComp(ecs, idx_b, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (ecs->colliders[idx_a].type != GT_COLLIDER_BOX) return false;
    if (ecs->colliders[idx_b].type != GT_COLLIDER_BOX) return false;

    GtVec2 pos_a = gtECSGetWorldPos(ecs, idx_a);
    GtVec2 pos_b = gtECSGetWorldPos(ecs, idx_b);
    GtVec2 he_a = ecs->colliders[idx_a].box.half_extents;
    GtVec2 he_b = ecs->colliders[idx_b].box.half_extents;
    float scale_a = fabsf(ecs->transforms[idx_a].scale);
    float scale_b = fabsf(ecs->transforms[idx_b].scale);
    he_a.x *= scale_a; he_a.y *= scale_a;
    he_b.x *= scale_b; he_b.y *= scale_b;

    // Le contact exact par le bord est considéré comme une collision,
    // comme pour cercle-cercle et cercle-box.
    return fabsf(pos_a.x - pos_b.x) <= (he_a.x + he_b.x) &&
           fabsf(pos_a.y - pos_b.y) <= (he_a.y + he_b.y);
}

bool gtECSCheckCircleBox(const GtECS* ecs, GtEntity circle_e, GtEntity box_e) {
    if (!ecs) return false;
    int idx_c, idx_b;
    if (!gtECSValidateEntity(ecs, circle_e, &idx_c) || !gtECSValidateEntity(ecs, box_e, &idx_b)) return false;
    if (!gtECSHasComp(ecs, idx_c, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (!gtECSHasComp(ecs, idx_b, GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) return false;
    if (ecs->colliders[idx_c].type != GT_COLLIDER_CIRCLE) return false;
    if (ecs->colliders[idx_b].type != GT_COLLIDER_BOX) return false;

    GtVec2 pos_c = gtECSGetWorldPos(ecs, idx_c);
    GtVec2 pos_b = gtECSGetWorldPos(ecs, idx_b);
    float r = ecs->colliders[idx_c].circle.radius * fabsf(ecs->transforms[idx_c].scale);
    GtVec2 he = ecs->colliders[idx_b].box.half_extents;
    float box_scale = fabsf(ecs->transforms[idx_b].scale);
    he.x *= box_scale;
    he.y *= box_scale;

    // Point le plus proche sur la box
    float closest_x = fmaxf(pos_b.x - he.x, fminf(pos_c.x, pos_b.x + he.x));
    float closest_y = fmaxf(pos_b.y - he.y, fminf(pos_c.y, pos_b.y + he.y));
    float dx = pos_c.x - closest_x;
    float dy = pos_c.y - closest_y;
    return (dx * dx + dy * dy) <= (r * r);
}

// Détection par paires brute-force O(n²), sans broad phase.
// (Une vraie broad phase — grille spatiale ou sort-and-sweep — serait une
// extension future ; le snapshot de handles ci-dessous reste nécessaire
// dans tous les cas pour la sécurité des callbacks.)
void gtECSSystemCollisions(GtECS* ecs, GtECSCollisionFunc on_hit, void* user_data) {
    if (!ecs || !on_hit || !ecs->alive || !ecs->masks ||
        !ecs->generations || !ecs->colliders || !ecs->transforms) return;
    if (ecs->alive_count < 2) return;

    // Snapshot des handles, pas seulement des indices : si le callback détruit
    // puis recrée une entité pendant la boucle, l'ancienne handle reste invalide.
    GtEntity* entities = (GtEntity*)malloc((size_t)ecs->alive_count * sizeof(*entities));
    if (!entities) return;

    int count = 0;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if ((ecs->masks[idx] & (GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) !=
            (GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) continue;
        entities[count++] = GT_ENTITY_MAKE(idx, ecs->generations[idx]);
    }

    for (int i = 0; i < count; i++) {
        GtEntity ent_a = entities[i];
        if (!gtECSIsAlive(ecs, ent_a)) continue;

        // Pré-check avant la boucle interne. Les pointeurs sont re-résolus
        // à CHAQUE itération ci-dessous, car le callback on_hit peut
        // détruire des entités ou retirer des composants en cours de route.
        if (!gtECSGetCollider(ecs, ent_a) || !gtECSGetTransform(ecs, ent_a)) continue;

        for (int j = i + 1; j < count; j++) {
            GtEntity ent_b = entities[j];
            if (!gtECSIsAlive(ecs, ent_a) || !gtECSIsAlive(ecs, ent_b)) continue;

            GtCollider* col_a = gtECSGetCollider(ecs, ent_a);
            GtCollider* col_b = gtECSGetCollider(ecs, ent_b);
            if (!col_a || !col_b) continue;
            if (!gtECSGetTransform(ecs, ent_a) || !gtECSGetTransform(ecs, ent_b)) continue;

            uint32_t layer_a = col_a->layer;
            uint32_t mask_a = col_a->mask;
            uint32_t layer_b = col_b->layer;
            uint32_t mask_b = col_b->mask;

            // Vérifie layers/masks dans les deux sens.
            if ((mask_a & layer_b) == 0 || (mask_b & layer_a) == 0) continue;

            bool hit = false;
            if (col_a->type == GT_COLLIDER_CIRCLE && col_b->type == GT_COLLIDER_CIRCLE) {
                hit = gtECSCheckCircleCircle(ecs, ent_a, ent_b);
            } else if (col_a->type == GT_COLLIDER_BOX && col_b->type == GT_COLLIDER_BOX) {
                hit = gtECSCheckBoxBox(ecs, ent_a, ent_b);
            } else if (col_a->type == GT_COLLIDER_CIRCLE && col_b->type == GT_COLLIDER_BOX) {
                hit = gtECSCheckCircleBox(ecs, ent_a, ent_b);
            } else if (col_a->type == GT_COLLIDER_BOX && col_b->type == GT_COLLIDER_CIRCLE) {
                hit = gtECSCheckCircleBox(ecs, ent_b, ent_a);
            }

            if (hit) {
                // Il n'y a pas encore de solver physique dans libGT : le callback
                // est donc le mécanisme de réaction, trigger ou collision solide.
                on_hit(ecs, ent_a, ent_b, user_data);
            }
        }
    }

    free(entities);
}

// --- Dégâts ---

bool gtECSDamageEntity(GtECS* ecs, GtEntity entity, int amount) {
    if (!ecs || amount <= 0) return false;

    int idx;
    if (!gtECSValidateEntity(ecs, entity, &idx)) return false;
    if (!gtECSHasComp(ecs, idx, GT_COMP_HEALTH)) return false;

    GtHealth* h = &ecs->healths[idx];
    if (h->invincible_timer > 0.0f) return false;

    if (amount >= h->current) {
        h->current = 0;
    } else {
        h->current -= amount;
    }

    h->invincible_timer = 0.5f;
    return h->current <= 0;
}

// =========================================================================
// IMPLÉMENTATION : INPUT MAPPING (XInput, bindings, actions, maps, profiles)
// =========================================================================

// Inclusion XInput (types XINPUT_STATE/XINPUT_VIBRATION + constantes uniquement)
#include <xinput.h>
#include <stdio.h>
#include <string.h>

/* --- Chargement dynamique de XInput ---------------------------------------
 * Pourquoi : l'édition de liens classique (-lxinput) crée un import STATIQUE
 * vers XINPUT1_3.dll (DirectX Runtime June 2010). Cette DLL n'est PAS fournie
 * avec Windows 10/11 (qui n'ont que xinput1_4.dll et XInput9_1_0.dll) : l'exe
 * refuse alors de démarrer avec STATUS_DLL_NOT_FOUND (exit 127 silencieux).
 * Solution zéro-dépendance : LoadLibrary au premier appel, avec fallback
 *   xinput1_4.dll (Win8+) -> XInput9_1_0.dll (Vista+) -> XINPUT1_3.dll (legacy)
 * -> xinputuap.dll. Sans manette ni DLL, les wrappers retournent
 * ERROR_DEVICE_NOT_CONNECTED (comme XInput le fait nativement). */
typedef struct {
    bool    tried;      // tentative de chargement déjà effectuée (une seule)
    HMODULE module;     // handle DLL (NULL si aucune trouvée)
    DWORD (WINAPI *get_state)(DWORD, XINPUT_STATE*);      // XInputGetState
    DWORD (WINAPI *set_state)(DWORD, XINPUT_VIBRATION*);  // XInputSetState
} GtXInputApi;

static GtXInputApi g_gtXInput = { false, NULL, NULL, NULL };

// Charge XInput une seule fois (lazy). Appeler depuis le thread principal du jeu.
static void gtXInputLoad(void) {
    if (g_gtXInput.tried) return;
    g_gtXInput.tried = true;

    static const char* k_names[] = {
        "xinput1_4.dll",    // Windows 8/10/11 (natif)
        "XInput9_1_0.dll",  // Windows Vista+ (existe aussi sur 10/11)
        "XINPUT1_3.dll",    // Legacy DirectX Runtime (si installé)
        "xinputuap.dll",    // Variante UWP (certains systèmes)
    };
    for (size_t i = 0; i < sizeof(k_names) / sizeof(k_names[0]); i++) {
        g_gtXInput.module = LoadLibraryA(k_names[i]);
        if (g_gtXInput.module) break;
    }
    if (g_gtXInput.module) {
        g_gtXInput.get_state = (DWORD (WINAPI*)(DWORD, XINPUT_STATE*))(void*)
            GetProcAddress(g_gtXInput.module, "XInputGetState");
        g_gtXInput.set_state = (DWORD (WINAPI*)(DWORD, XINPUT_VIBRATION*))(void*)
            GetProcAddress(g_gtXInput.module, "XInputSetState");
    }
}

// Wrappers : mêmes sémantiques que les fonctions XInput natives.
// Si aucune DLL XInput n'est disponible -> ERROR_DEVICE_NOT_CONNECTED
// (pas de crash, comportement identique à "aucune manette branchée").
static DWORD gtXInputGetState(DWORD user_index, XINPUT_STATE* state) {
    gtXInputLoad();
    if (!g_gtXInput.get_state) {
        if (state) memset(state, 0, sizeof(*state));
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    return g_gtXInput.get_state(user_index, state);
}

static DWORD gtXInputSetState(DWORD user_index, XINPUT_VIBRATION* vibration) {
    gtXInputLoad();
    if (!g_gtXInput.set_state) return ERROR_DEVICE_NOT_CONNECTED;
    return g_gtXInput.set_state(user_index, vibration);
}

// État interne du système d'input
struct GtInputSystem {
    GtPlayerProfile profiles[GT_MAX_PLAYERS];
    GtWindow* window;           // Fenêtre associée (pour clavier/souris)
    
    // État gamepad (XInput)
    XINPUT_STATE gamepad_state[GT_MAX_PLAYERS];
    XINPUT_STATE gamepad_prev_state[GT_MAX_PLAYERS];
    bool gamepad_connected[GT_MAX_PLAYERS];
    
    // Vibration timers
    struct {
        float timer;
        float left_motor;
        float right_motor;
    } vibration[GT_MAX_PLAYERS];
};

// Helper : normalise un axis gamepad avec deadzone radiale
static inline float gtInputNormalizeAxis(short raw, float deadzone) {
    // raw est dans [-32768, 32767]
    float val = (float)raw / 32767.0f;
    if (val < 0.0f) val = (float)raw / 32768.0f; // asymétrique pour -32768
    
    float abs_val = fabsf(val);
    if (abs_val <= deadzone) return 0.0f;
    
    // Re-mapping pour éviter le "trou" au centre
    float sign = (val >= 0.0f) ? 1.0f : -1.0f;
    return sign * (abs_val - deadzone) / (1.0f - deadzone);
}

// Helper : test un binding unique
static bool gtInputTestBinding(const GtInputSystem* input, const GtInputBinding* binding) {
    if (!input || !binding) return false;
    
    switch (binding->type) {
        case GT_INPUT_KEYBOARD: {
            uint16_t vk = binding->data.keyboard.vk;
            if (vk < 256 && input->window) {
                return input->window->keys[vk];
            }
            return false;
        }
        
        case GT_INPUT_MOUSE_BUTTON: {
            uint8_t btn = binding->data.mouse_button.button;
            if (btn < 3 && input->window) {
                return input->window->mouse_buttons[btn];
            }
            return false;
        }
        
        case GT_INPUT_MOUSE_WHEEL: {
            // Note: la molette est un événement, pas un état maintenu.
            // On ne peut pas "tenir" la molette. Retourne false pour état.
            // Pour la molette, utiliser gtGetMouseWheelDelta() côté utilisateur.
            (void)input;
            return false;
        }
        
        case GT_INPUT_GAMEPAD_BUTTON: {
            int player = binding->player_index;
            if (player < 0 || player >= GT_MAX_PLAYERS) return false;
            if (!input->gamepad_connected[player]) return false;
            uint16_t btn = binding->data.gamepad_button.button;
            return (input->gamepad_state[player].Gamepad.wButtons & btn) != 0;
        }
        
        case GT_INPUT_GAMEPAD_AXIS: {
            int player = binding->player_index;
            if (player < 0 || player >= GT_MAX_PLAYERS) return false;
            if (!input->gamepad_connected[player]) return false;
            
            uint8_t axis = binding->data.gamepad_axis.axis;
            float threshold = binding->data.gamepad_axis.threshold;
            float deadzone = input->profiles[player].stick_deadzone;
            if (threshold > deadzone) deadzone = threshold; // seuil custom > deadzone
            
            const XINPUT_GAMEPAD* gp = &input->gamepad_state[player].Gamepad;
            float val = 0.0f;
            
            switch (axis) {
                case 0: val = gtInputNormalizeAxis(gp->sThumbLX, deadzone); break; // LX
                case 1: val = -gtInputNormalizeAxis(gp->sThumbLY, deadzone); break; // LY (inversé: haut=positif)
                case 2: val = gtInputNormalizeAxis(gp->sThumbRX, deadzone); break; // RX
                case 3: val = -gtInputNormalizeAxis(gp->sThumbRY, deadzone); break; // RY
                case 4: val = (float)gp->bLeftTrigger / 255.0f; break;  // LT [0,1]
                case 5: val = (float)gp->bRightTrigger / 255.0f; break; // RT [0,1]
                default: return false;
            }
            
            return fabsf(val) > threshold;
        }
        
        default:
            return false;
    }
}

// Helper : obtient la valeur analogique d'un binding (pour axes)
static float gtInputGetBindingValue(const GtInputSystem* input, const GtInputBinding* binding) {
    if (!input || !binding) return 0.0f;
    
    switch (binding->type) {
        case GT_INPUT_KEYBOARD:
        case GT_INPUT_MOUSE_BUTTON:
        case GT_INPUT_GAMEPAD_BUTTON: {
            // Boutons = 0.0 ou 1.0
            return gtInputTestBinding(input, binding) ? 1.0f : 0.0f;
        }
        
        case GT_INPUT_GAMEPAD_AXIS: {
            int player = binding->player_index;
            if (player < 0 || player >= GT_MAX_PLAYERS) return 0.0f;
            if (!input->gamepad_connected[player]) return 0.0f;
            
            uint8_t axis = binding->data.gamepad_axis.axis;
            float deadzone = input->profiles[player].stick_deadzone;
            
            const XINPUT_GAMEPAD* gp = &input->gamepad_state[player].Gamepad;
            float val = 0.0f;
            
            switch (axis) {
                case 0: val = gtInputNormalizeAxis(gp->sThumbLX, deadzone); break;
                case 1: val = -gtInputNormalizeAxis(gp->sThumbLY, deadzone); break;
                case 2: val = gtInputNormalizeAxis(gp->sThumbRX, deadzone); break;
                case 3: val = -gtInputNormalizeAxis(gp->sThumbRY, deadzone); break;
                case 4: val = (float)gp->bLeftTrigger / 255.0f; break;
                case 5: val = (float)gp->bRightTrigger / 255.0f; break;
                default: return 0.0f;
            }
            return val; // [-1, 1] pour sticks, [0, 1] pour triggers
        }
        
        default:
            return 0.0f;
    }
}

// Trouve une action par nom dans une map
static GtAction* gtInputFindAction(GtActionMap* map, const char* name) {
    if (!map || !name) return NULL;
    for (int i = 0; i < map->action_count; i++) {
        if (strcmp(map->actions[i].name, name) == 0) {
            return &map->actions[i];
        }
    }
    return NULL;
}

// Crée le système d'input
GtInputSystem* gtInputCreate(void) {
    GtInputSystem* input = (GtInputSystem*)calloc(1, sizeof(GtInputSystem));
    if (!input) return NULL;
    
    // Initialise profils par défaut
    for (int i = 0; i < GT_MAX_PLAYERS; i++) {
        GtPlayerProfile* p = &input->profiles[i];
        p->player_index = i;
        p->stick_deadzone = 0.15f;
        p->trigger_threshold = 0.1f;
        p->vibration_strength = 1.0f;
        p->mouse_sensitivity = 1.0f;
        p->invert_y = false;
        p->gamepad_connected = false;
        p->map_count = 0;
        p->active_map_count = 0;
    }
    
    return input;
}

// Détruit le système d'input
void gtInputDestroy(GtInputSystem* input) {
    if (!input) return;
    // Arrête vibrations
    for (int i = 0; i < GT_MAX_PLAYERS; i++) {
        XINPUT_VIBRATION vib = {0, 0};
        gtXInputSetState(i, &vib);
    }
    free(input);
}

// Met à jour le système d'input (appeler chaque frame après gtEventsWindow)
void gtInputUpdate(GtInputSystem* input, GtWindow* window, float dt) {
    if (!input) return;
    input->window = window;
    
    // Sauvegarde état précédent gamepad
    for (int i = 0; i < GT_MAX_PLAYERS; i++) {
        input->gamepad_prev_state[i] = input->gamepad_state[i];
        
        // Poll XInput (chargement dynamique, sans import statique XINPUT1_3.dll)
        DWORD result = gtXInputGetState(i, &input->gamepad_state[i]);
        input->gamepad_connected[i] = (result == ERROR_SUCCESS);
        input->profiles[i].gamepad_connected = input->gamepad_connected[i];
        
        // Met à jour vibration
        if (input->vibration[i].timer > 0.0f) {
            input->vibration[i].timer -= dt;
            if (input->vibration[i].timer <= 0.0f) {
                XINPUT_VIBRATION vib = {0, 0};
                gtXInputSetState(i, &vib);
                input->vibration[i].timer = 0.0f;
            }
        }
    }
    
    // Met à jour toutes les actions de toutes les maps actives de tous les profils
    for (int p = 0; p < GT_MAX_PLAYERS; p++) {
        GtPlayerProfile* profile = &input->profiles[p];
        
        // Parcourt la stack de maps actives (du bas vers le haut pour priorité)
        for (int ms = 0; ms < profile->active_map_count; ms++) {
            int map_idx = profile->active_map_stack[ms];
            if (map_idx < 0 || map_idx >= profile->map_count) continue;
            GtActionMap* map = &profile->maps[map_idx];
            if (!map->enabled) continue;
            
            for (int a = 0; a < map->action_count; a++) {
                GtAction* action = &map->actions[a];
                
                // Sauvegarde état précédent
                action->was_pressed = action->is_pressed;
                action->prev_value = action->value;
                
                // Calcule nouvel état (OU logique sur tous les bindings)
                bool pressed = false;
                float max_value = 0.0f;
                
                for (int b = 0; b < action->binding_count; b++) {
                    const GtInputBinding* binding = &action->bindings[b];
                    if (gtInputTestBinding(input, binding)) {
                        pressed = true;
                    }
                    float val = gtInputGetBindingValue(input, binding);
                    if (fabsf(val) > fabsf(max_value)) {
                        max_value = val;
                    }
                }
                
                action->is_pressed = pressed;
                action->value = max_value;
            }
        }
    }
}

// Profils joueurs
GtPlayerProfile* gtInputGetProfile(GtInputSystem* input, int player_index) {
    if (!input || player_index < 0 || player_index >= GT_MAX_PLAYERS) return NULL;
    return &input->profiles[player_index];
}

void gtInputSetProfile(GtInputSystem* input, int player_index, const GtPlayerProfile* profile) {
    if (!input || player_index < 0 || player_index >= GT_MAX_PLAYERS || !profile) return;
    input->profiles[player_index] = *profile;
    input->profiles[player_index].player_index = player_index;
}

// Action Maps
GtActionMap* gtInputCreateMap(const char* name) {
    if (!name) return NULL;
    GtActionMap* map = (GtActionMap*)calloc(1, sizeof(GtActionMap));
    if (!map) return NULL;
    map->name = name; // L'utilisateur gère la durée de vie de la string
    map->enabled = true;
    return map;
}

void gtInputDestroyMap(GtActionMap* map) {
    if (!map) return;
    // Les actions contiennent juste des strings et des bindings (pas d'allocs)
    free(map);
}

void gtInputAddAction(GtActionMap* map, const char* action_name) {
    if (!map || !action_name) return;
    if (map->action_count >= GT_MAX_ACTIONS_PER_MAP) return;
    if (gtInputFindAction(map, action_name)) return; // Déjà existante
    
    GtAction* action = &map->actions[map->action_count];
    action->name = action_name; // L'utilisateur gère la durée de vie
    action->binding_count = 0;
    action->is_pressed = false;
    action->was_pressed = false;
    action->value = 0.0f;
    action->prev_value = 0.0f;
    map->action_count++;
}

void gtInputBindKey(GtActionMap* map, const char* action, uint16_t vk) {
    if (!map || !action) return;
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return;
    if (a->binding_count >= GT_MAX_BINDINGS_PER_ACTION) return;
    
    GtInputBinding* b = &a->bindings[a->binding_count];
    b->type = GT_INPUT_KEYBOARD;
    b->player_index = 0;
    b->data.keyboard.vk = vk;
    a->binding_count++;
}

void gtInputBindMouseBtn(GtActionMap* map, const char* action, uint8_t btn) {
    if (!map || !action) return;
    if (btn >= 3) return;
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return;
    if (a->binding_count >= GT_MAX_BINDINGS_PER_ACTION) return;
    
    GtInputBinding* b = &a->bindings[a->binding_count];
    b->type = GT_INPUT_MOUSE_BUTTON;
    b->player_index = 0;
    b->data.mouse_button.button = btn;
    a->binding_count++;
}

void gtInputBindGamepadBtn(GtActionMap* map, const char* action, int player, uint16_t btn) {
    if (!map || !action) return;
    if (player < 0 || player >= GT_MAX_PLAYERS) return;
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return;
    if (a->binding_count >= GT_MAX_BINDINGS_PER_ACTION) return;
    
    GtInputBinding* b = &a->bindings[a->binding_count];
    b->type = GT_INPUT_GAMEPAD_BUTTON;
    b->player_index = (uint8_t)player;
    b->data.gamepad_button.button = btn;
    a->binding_count++;
}

void gtInputBindGamepadAxis(GtActionMap* map, const char* action, int player, uint8_t axis, float threshold) {
    if (!map || !action) return;
    if (player < 0 || player >= GT_MAX_PLAYERS) return;
    if (axis > 5) return; // 0=LX,1=LY,2=RX,3=RY,4=LT,5=RT
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return;
    if (a->binding_count >= GT_MAX_BINDINGS_PER_ACTION) return;
    
    GtInputBinding* b = &a->bindings[a->binding_count];
    b->type = GT_INPUT_GAMEPAD_AXIS;
    b->player_index = (uint8_t)player;
    b->data.gamepad_axis.axis = axis;
    b->data.gamepad_axis.threshold = threshold;
    a->binding_count++;
}

// Stack de maps actives
void gtInputPushMap(GtPlayerProfile* profile, GtActionMap* map) {
    if (!profile || !map) return;

    // Déduplication PAR NOM (pas par pointeur !) : la map stockée dans le
    // profil est une COPIE, donc comparer les adresses ne détecte jamais
    // une map externe déjà poussée -> chaque push recopiait la map et
    // gonflait map_count (casserait save/load et la stack priorité).
    int map_idx = -1;
    for (int i = 0; i < profile->map_count; i++) {
        const char* stored = profile->maps[i].name;
        if (stored && map->name && strcmp(stored, map->name) == 0) {
            map_idx = i;
            break;
        }
    }

    if (map_idx == -1) {
        // Map pas encore dans le profil : l'ajoute
        if (profile->map_count >= GT_MAX_MAPS_PER_PROFILE) return;
        map_idx = profile->map_count;
        profile->map_count++;
    }

    // (Re)copie le contenu : re-pusher une map après rebinding rafraîchit
    // la copie stockée dans le profil.
    profile->maps[map_idx] = *map;

    // Déjà active ? -> pas de doublon dans la stack (une map présente deux
    // fois serait mise à jour 2x par frame et casserait l'edge detection).
    for (int s = 0; s < profile->active_map_count; s++) {
        if (profile->active_map_stack[s] == map_idx) return;
    }

    if (profile->active_map_count >= GT_MAX_MAPS_PER_PROFILE) return;
    profile->active_map_stack[profile->active_map_count] = map_idx;
    profile->active_map_count++;
}

void gtInputPopMap(GtPlayerProfile* profile) {
    if (!profile || profile->active_map_count <= 0) return;
    profile->active_map_count--;
}

// "Set" = EXCLUSIF : contrairement à Push (additif, pour overlays),
// SetActiveMap remplace TOUT le contenu du profil par cette seule map
// (vide la stack active ET le stockage de maps).
void gtInputSetActiveMap(GtPlayerProfile* profile, GtActionMap* map) {
    if (!profile || !map) return;
    profile->active_map_count = 0;
    profile->map_count = 0;
    gtInputPushMap(profile, map);
}

// Query état action
bool gtInputIsActionPressed(GtInputSystem* input, int player, const char* action) {
    if (!input || !action || player < 0 || player >= GT_MAX_PLAYERS) return false;
    GtPlayerProfile* profile = &input->profiles[player];
    
    // Cherche dans toutes les maps actives (priorité à la dernière pushée)
    for (int ms = profile->active_map_count - 1; ms >= 0; ms--) {
        int map_idx = profile->active_map_stack[ms];
        if (map_idx < 0 || map_idx >= profile->map_count) continue;
        GtActionMap* map = &profile->maps[map_idx];
        if (!map->enabled) continue;
        
        GtAction* a = gtInputFindAction(map, action);
        if (a) return a->is_pressed;
    }
    return false;
}

bool gtInputWasActionPressed(GtInputSystem* input, int player, const char* action) {
    if (!input || !action || player < 0 || player >= GT_MAX_PLAYERS) return false;
    GtPlayerProfile* profile = &input->profiles[player];
    
    for (int ms = profile->active_map_count - 1; ms >= 0; ms--) {
        int map_idx = profile->active_map_stack[ms];
        if (map_idx < 0 || map_idx >= profile->map_count) continue;
        GtActionMap* map = &profile->maps[map_idx];
        if (!map->enabled) continue;
        
        GtAction* a = gtInputFindAction(map, action);
        if (a) return a->is_pressed && !a->was_pressed;
    }
    return false;
}

bool gtInputWasActionReleased(GtInputSystem* input, int player, const char* action) {
    if (!input || !action || player < 0 || player >= GT_MAX_PLAYERS) return false;
    GtPlayerProfile* profile = &input->profiles[player];
    
    for (int ms = profile->active_map_count - 1; ms >= 0; ms--) {
        int map_idx = profile->active_map_stack[ms];
        if (map_idx < 0 || map_idx >= profile->map_count) continue;
        GtActionMap* map = &profile->maps[map_idx];
        if (!map->enabled) continue;
        
        GtAction* a = gtInputFindAction(map, action);
        if (a) return !a->is_pressed && a->was_pressed;
    }
    return false;
}

float gtInputGetActionValue(GtInputSystem* input, int player, const char* action) {
    if (!input || !action || player < 0 || player >= GT_MAX_PLAYERS) return 0.0f;
    GtPlayerProfile* profile = &input->profiles[player];
    
    for (int ms = profile->active_map_count - 1; ms >= 0; ms--) {
        int map_idx = profile->active_map_stack[ms];
        if (map_idx < 0 || map_idx >= profile->map_count) continue;
        GtActionMap* map = &profile->maps[map_idx];
        if (!map->enabled) continue;
        
        GtAction* a = gtInputFindAction(map, action);
        if (a) return a->value;
    }
    return 0.0f;
}

GtVec2 gtInputGetActionVec2(GtInputSystem* input, int player, const char* action_x, const char* action_y) {
    float x = gtInputGetActionValue(input, player, action_x);
    float y = gtInputGetActionValue(input, player, action_y);
    return gtVec2(x, y);
}

// Rebinding runtime
bool gtInputRebindAction(GtActionMap* map, const char* action, const GtInputBinding* new_binding) {
    if (!map || !action || !new_binding) return false;
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return false;
    
    a->binding_count = 0;
    a->bindings[0] = *new_binding;
    a->binding_count = 1;
    return true;
}

void gtInputClearActionBindings(GtActionMap* map, const char* action) {
    if (!map || !action) return;
    GtAction* a = gtInputFindAction(map, action);
    if (!a) return;
    a->binding_count = 0;
}

// Sérialisation binaire simple
#define GT_INPUT_MAGIC 0x4754494D // 'GTIM'
#define GT_INPUT_VERSION 1

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t version;
    int player_index;
    float stick_deadzone;
    float trigger_threshold;
    float vibration_strength;
    float mouse_sensitivity;
    bool invert_y;
    int map_count;
} GtInputProfileHeader;

typedef struct {
    char name[64];
    int action_count;
    bool enabled;
} GtInputMapHeader;

typedef struct {
    char name[64];
    int binding_count;
} GtInputActionHeader;

typedef struct {
    uint8_t type;
    uint8_t player_index;
    union {
        struct { uint16_t vk; } keyboard;
        struct { uint8_t button; } mouse_button;
        struct { int16_t wheel_delta; } mouse_wheel;
        struct { uint16_t button; } gamepad_button;
        struct { uint8_t axis; float threshold; } gamepad_axis;
    } data;
} GtInputBindingDisk;
#pragma pack(pop)

bool gtInputSaveProfile(const GtPlayerProfile* profile, const char* filepath) {
    if (!profile || !filepath) return false;
    
    FILE* f = fopen(filepath, "wb");
    if (!f) return false;
    
    // Header
    GtInputProfileHeader hdr = {
        .magic = GT_INPUT_MAGIC,
        .version = GT_INPUT_VERSION,
        .player_index = profile->player_index,
        .stick_deadzone = profile->stick_deadzone,
        .trigger_threshold = profile->trigger_threshold,
        .vibration_strength = profile->vibration_strength,
        .mouse_sensitivity = profile->mouse_sensitivity,
        .invert_y = profile->invert_y,
        .map_count = profile->map_count
    };
    fwrite(&hdr, sizeof(hdr), 1, f);
    
    // Maps
        for (int m = 0; m < profile->map_count; m++) {
            const GtActionMap* map = &profile->maps[m];
            GtInputMapHeader mhdr = {0};
            strncpy(mhdr.name, map->name ? map->name : "", 63);
            mhdr.action_count = map->action_count;
            mhdr.enabled = map->enabled;
            fwrite(&mhdr, sizeof(mhdr), 1, f);

            // Actions
            for (int a = 0; a < map->action_count; a++) {
                const GtAction* act = &map->actions[a];
                GtInputActionHeader ahdr = {0};
                strncpy(ahdr.name, act->name ? act->name : "", 63);
                ahdr.binding_count = act->binding_count;
                fwrite(&ahdr, sizeof(ahdr), 1, f);
            
            // Bindings
            for (int b = 0; b < act->binding_count; b++) {
                const GtInputBinding* src = &act->bindings[b];
                GtInputBindingDisk dst = {0};
                dst.type = src->type;
                dst.player_index = src->player_index;
                // Union copy - manual copy for each variant
                switch (src->type) {
                    case GT_INPUT_KEYBOARD:
                        dst.data.keyboard.vk = src->data.keyboard.vk;
                        break;
                    case GT_INPUT_MOUSE_BUTTON:
                        dst.data.mouse_button.button = src->data.mouse_button.button;
                        break;
                    case GT_INPUT_MOUSE_WHEEL:
                        dst.data.mouse_wheel.wheel_delta = src->data.mouse_wheel.wheel_delta;
                        break;
                    case GT_INPUT_GAMEPAD_BUTTON:
                        dst.data.gamepad_button.button = src->data.gamepad_button.button;
                        break;
                    case GT_INPUT_GAMEPAD_AXIS:
                        dst.data.gamepad_axis.axis = src->data.gamepad_axis.axis;
                        dst.data.gamepad_axis.threshold = src->data.gamepad_axis.threshold;
                        break;
                    default:
                        break;
                }
                fwrite(&dst, sizeof(dst), 1, f);
            }
        }
    }
    
    fclose(f);
    return true;
}

bool gtInputLoadProfile(GtPlayerProfile* profile, const char* filepath) {
    if (!profile || !filepath) return false;
    
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;
    
    GtInputProfileHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 || hdr.magic != GT_INPUT_MAGIC || hdr.version != GT_INPUT_VERSION) {
        fclose(f);
        return false;
    }
    
    // Reset profil
    memset(profile, 0, sizeof(*profile));
    profile->player_index = hdr.player_index;
    profile->stick_deadzone = hdr.stick_deadzone;
    profile->trigger_threshold = hdr.trigger_threshold;
    profile->vibration_strength = hdr.vibration_strength;
    profile->mouse_sensitivity = hdr.mouse_sensitivity;
    profile->invert_y = hdr.invert_y;
    
    // Maps
        for (int m = 0; m < hdr.map_count && m < GT_MAX_MAPS_PER_PROFILE; m++) {
            GtInputMapHeader mhdr;
            if (fread(&mhdr, sizeof(mhdr), 1, f) != 1) break;

            GtActionMap* map = &profile->maps[m];
            map->name = _strdup(mhdr.name); // Strdup pour durée de vie indépendante
            map->action_count = mhdr.action_count;
            map->enabled = mhdr.enabled;

            for (int a = 0; a < mhdr.action_count && a < GT_MAX_ACTIONS_PER_MAP; a++) {
                GtInputActionHeader ahdr;
                if (fread(&ahdr, sizeof(ahdr), 1, f) != 1) break;

                GtAction* act = &map->actions[a];
                act->name = _strdup(ahdr.name);
                act->binding_count = ahdr.binding_count;
                act->is_pressed = false;
                act->was_pressed = false;
                act->value = 0.0f;
                act->prev_value = 0.0f;

                for (int b = 0; b < ahdr.binding_count && b < GT_MAX_BINDINGS_PER_ACTION; b++) {
                    GtInputBindingDisk src;
                    if (fread(&src, sizeof(src), 1, f) != 1) break;
                    GtInputBinding* dst = &act->bindings[b];
                    dst->type = src.type;
                    dst->player_index = src.player_index;
                    // Union copy - manual copy for each variant
                    switch (src.type) {
                        case GT_INPUT_KEYBOARD:
                            dst->data.keyboard.vk = src.data.keyboard.vk;
                            break;
                        case GT_INPUT_MOUSE_BUTTON:
                            dst->data.mouse_button.button = src.data.mouse_button.button;
                            break;
                        case GT_INPUT_MOUSE_WHEEL:
                            dst->data.mouse_wheel.wheel_delta = src.data.mouse_wheel.wheel_delta;
                            break;
                        case GT_INPUT_GAMEPAD_BUTTON:
                            dst->data.gamepad_button.button = src.data.gamepad_button.button;
                            break;
                        case GT_INPUT_GAMEPAD_AXIS:
                            dst->data.gamepad_axis.axis = src.data.gamepad_axis.axis;
                            dst->data.gamepad_axis.threshold = src.data.gamepad_axis.threshold;
                            break;
                        default:
                            break;
                    }
                }
            }
            profile->map_count++;
        }
    
    fclose(f);
    return true;
}

// Gamepad bas niveau
bool gtInputIsGamepadConnected(int player_index) {
    if (player_index < 0 || player_index >= GT_MAX_PLAYERS) return false;
    XINPUT_STATE state;
    return gtXInputGetState(player_index, &state) == ERROR_SUCCESS;
}

void gtInputSetVibration(int player_index, float left_motor, float right_motor, float duration) {
    (void)duration; // Non utilisé pour le standalone, géré par gtInputUpdate
    if (player_index < 0 || player_index >= GT_MAX_PLAYERS) return;

    // Clamp
    if (left_motor < 0.0f) left_motor = 0.0f;
    if (left_motor > 1.0f) left_motor = 1.0f;
    if (right_motor < 0.0f) right_motor = 0.0f;
    if (right_motor > 1.0f) right_motor = 1.0f;

    XINPUT_VIBRATION vib;
    vib.wLeftMotorSpeed = (WORD)(left_motor * 65535.0f);
    vib.wRightMotorSpeed = (WORD)(right_motor * 65535.0f);

    gtXInputSetState(player_index, &vib);

    // Note: le timer de vibration est géré dans gtInputUpdate via le système
    // Pour un usage standalone sans GtInputSystem, l'utilisateur doit gérer le timer lui-même.
}

#endif // LIBGT_IMPLEMENTATION
#endif // LIBGT_H
