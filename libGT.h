#ifndef LIBGT_H
#define LIBGT_H

// Inclusions standard C
#include <stdbool.h>   // bool, true, false
#include <stdint.h>    // uint8_t, uint32_t, int64_t, etc.
#include <stddef.h>    // size_t, NULL
#include <math.h>      // sqrtf, sinf, cosf, etc.

/* =========================================================================
* LIBGT - Bibliothèque graphique 2D légère pour Windows (GDI + Direct2D)
* =========================================================================
* Philosophie :
*   - API simple, proche du matériel (framebuffer CPU ou render target GPU)
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
* =========================================================================
* BACKENDS DE RENDU (2.2)
* =========================================================================
* Deux backends derrière la même API (gtCreateRenderer(win, type)) :
*
*   GT_RENDERER_GDI  : software, framebuffer CPU (win->buffer) + StretchDIBits.
*                      Toujours disponible, pixel-exact, aucun lien GPU requis.
*   GT_RENDERER_D2D  : Direct2D (ID2D1HwndRenderTarget). Primitives
*                      anti-aliasées, images GPU, texte bitmap 8x8 via atlas
*                      GPU + texte HQ DirectWrite (gtRendererDrawTextHq).
*                      Linker avec : -ld2d1 -ldwrite -lole32
*                      Désactivable à la compilation : -D LIBGT_NO_D2D
*                      (gtCreateRenderer(..., GT_RENDERER_D2D) -> NULL).
*                      Fallback interne : matériel -> logiciel -> NULL ;
*                      vérifier avec gtRendererGetType().
*
* Matrice de compatibilité (écarts documentés entre backends) :
*   - Z-order : identique (ordre d'appel). En D2D, les gtDraw* window-level
*     sont routés vers le GPU ; win->buffer n'est alors PAS mis à jour
*     (le lire en D2D est indéfini — les tests pixel passent par un RT WIC).
*   - Transforms : GDI aplatit les rects tournés en AABB ; D2D applique la
*     vraie rotation. Les cercles D2D respectent le scale de la transform.
*   - gtRendererDrawImageEx : GDI ignore rot/origin ; D2D les implémente
*     (x,y = point d'ancrage défini par origin, pivot de rotation).
*   - gtRendererDrawTextHq : D2D = DirectWrite ; GDI = fallback police 8x8.
*   - Anti-aliasing : activé par défaut en D2D (gtRendererSetAntialias pour
*     le mode pixel-exact ALIASED) ; GDI est toujours sans AA.
*
* Threading : factory D2D SINGLE_THREADED — un renderer par thread.
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
* DIAGNOSTICS D'ERREURS
* =========================================================================
* La lib retourne NULL/valeurs par défaut en cas d'échec (style C). Pour
* savoir POURQUOI, deux mécanismes complémentaires :
*   - gtGetLastError() : le dernier message d'erreur de la lib (chaîne statique)
*   - gtSetErrorCallback() : notification immédiate (log console, crash report...)
* ========================================================================= */
typedef void (*GtErrorCallback)(const char* message, void* user_data);

// Installe un callback appelé à chaque erreur interne (NULL pour retirer)
void gtSetErrorCallback(GtErrorCallback callback, void* user_data);

// Dernier message d'erreur enregistré par la lib ("" si aucun depuis le départ)
const char* gtGetLastError(void);

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
* MATRICE 3x3 (GtMat3) - Transformations 2D homogènes
* =========================================================================
* Matrice 3x3 en ordre colonne-majeur (column-major) compatible GLSL/DirectXMath :
*   [ m00 m10 m20 ]   [ a c tx ]
*   [ m01 m11 m21 ] = [ b d ty ]
*   [ m02 m12 m22 ]   [ 0 0  1 ]
* Représente : translation, rotation, scale, shear en coordonnées homogènes.
* Multiplication : M * v  (v en coordonnées homogènes = vec3(x, y, 1))
* Ordre d'application : M = T * R * S (translation * rotation * scale)
* ========================================================================= */
typedef struct GtMat3 {
    float m[9];  // Colonne-majeur : m[0]=m00, m[1]=m01, m[2]=m02, m[3]=m10, etc.
} GtMat3;

// Matrice identité
static inline GtMat3 gtMat3Identity(void) {
    GtMat3 m = {{ 1,0,0,  0,1,0,  0,0,1 }};
    return m;
}

// Matrice de translation
static inline GtMat3 gtMat3Translate(float tx, float ty) {
    GtMat3 m = {{ 1,0,0,  0,1,0,  tx,ty,1 }};
    return m;
}
static inline GtMat3 gtMat3TranslateV(GtVec2 v) { return gtMat3Translate(v.x, v.y); }

// Matrice de rotation (angle en radians)
static inline GtMat3 gtMat3Rotate(float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    GtMat3 m = {{ c,-s,0,  s,c,0,  0,0,1 }};
    return m;
}

// Matrice de scale (uniforme ou non-uniforme)
static inline GtMat3 gtMat3Scale(float sx, float sy) {
    GtMat3 m = {{ sx,0,0,  0,sy,0,  0,0,1 }};
    return m;
}
static inline GtMat3 gtMat3ScaleV(GtVec2 v) { return gtMat3Scale(v.x, v.y); }
static inline GtMat3 gtMat3ScaleUniform(float s) { return gtMat3Scale(s, s); }

// Multiplication matrice * matrice (a * b) - applique b puis a
static inline GtMat3 gtMat3Mul(GtMat3 a, GtMat3 b) {
    GtMat3 r;
    r.m[0] = a.m[0]*b.m[0] + a.m[3]*b.m[1] + a.m[6]*b.m[2];
    r.m[1] = a.m[1]*b.m[0] + a.m[4]*b.m[1] + a.m[7]*b.m[2];
    r.m[2] = a.m[2]*b.m[0] + a.m[5]*b.m[1] + a.m[8]*b.m[2];
    r.m[3] = a.m[0]*b.m[3] + a.m[3]*b.m[4] + a.m[6]*b.m[5];
    r.m[4] = a.m[1]*b.m[3] + a.m[4]*b.m[4] + a.m[7]*b.m[5];
    r.m[5] = a.m[2]*b.m[3] + a.m[5]*b.m[4] + a.m[8]*b.m[5];
    r.m[6] = a.m[0]*b.m[6] + a.m[3]*b.m[7] + a.m[6]*b.m[8];
    r.m[7] = a.m[1]*b.m[6] + a.m[4]*b.m[7] + a.m[7]*b.m[8];
    r.m[8] = a.m[2]*b.m[6] + a.m[5]*b.m[7] + a.m[8]*b.m[8];
    return r;
}

// Multiplication matrice * vecteur (point, w=1) - transforme une position
static inline GtVec2 gtMat3MulVec2(GtMat3 m, GtVec2 v) {
    float x = m.m[0]*v.x + m.m[3]*v.y + m.m[6];
    float y = m.m[1]*v.x + m.m[4]*v.y + m.m[7];
    float w = m.m[2]*v.x + m.m[5]*v.y + m.m[8];
    return (w != 0.0f) ? gtVec2(x/w, y/w) : gtVec2(x, y);
}

// Multiplication matrice * vecteur (direction, w=0) - transforme un vecteur (sans translation)
static inline GtVec2 gtMat3MulDir(GtMat3 m, GtVec2 v) {
    return gtVec2(m.m[0]*v.x + m.m[3]*v.y, m.m[1]*v.x + m.m[4]*v.y);
}

// Tente l'inversion d'une matrice 2D affine (translation + rotation + scale).
// Retourne false si la matrice est non inversible (|det| ~ 0) SANS toucher
// *out — permet de détecter l'échec au lieu de le subir silencieusement.
static inline bool gtMat3TryInverse(GtMat3 m, GtMat3* out) {
    // Sous-matrice 2x2 linéaire
    float a = m.m[0], b = m.m[3];
    float c = m.m[1], d = m.m[4];
    float tx = m.m[6], ty = m.m[7];
    float det = a*d - b*c;
    if (fabsf(det) < 1e-6f) return false;
    float inv_det = 1.0f / det;
    GtMat3 r;
    r.m[0] =  d * inv_det;
    r.m[3] = -b * inv_det;
    r.m[1] = -c * inv_det;
    r.m[4] =  a * inv_det;
    r.m[6] = (b*ty - d*tx) * inv_det;
    r.m[7] = (c*tx - a*ty) * inv_det;
    r.m[2] = 0; r.m[5] = 0; r.m[8] = 1;
    if (out) *out = r;
    return true;
}

// Inverse d'une matrice 2D affine (translation + rotation + scale, sans shear/projection)
// ATTENTION : si la matrice est non inversible (déterminant ~ 0), retourne
// l'IDENTITÉ silencieusement — utiliser gtMat3TryInverse pour détecter l'échec.
static inline GtMat3 gtMat3Inverse(GtMat3 m) {
    GtMat3 r;
    if (!gtMat3TryInverse(m, &r)) return gtMat3Identity();
    return r;
}

// Transpose de matrice 3x3
static inline GtMat3 gtMat3Transpose(GtMat3 m) {
    GtMat3 r;
    r.m[0] = m.m[0]; r.m[3] = m.m[1]; r.m[6] = m.m[2];
    r.m[1] = m.m[3]; r.m[4] = m.m[4]; r.m[7] = m.m[5];
    r.m[2] = m.m[6]; r.m[5] = m.m[7]; r.m[8] = m.m[8];
    return r;
}

// Extraction composants (pour debug / décomposition)
static inline GtVec2 gtMat3GetTranslation(GtMat3 m) { return gtVec2(m.m[6], m.m[7]); }
static inline float  gtMat3GetRotation(GtMat3 m)    { return atan2f(m.m[1], m.m[0]); }
static inline GtVec2 gtMat3GetScale(GtMat3 m)       { return gtVec2(sqrtf(m.m[0]*m.m[0] + m.m[1]*m.m[1]), sqrtf(m.m[3]*m.m[3] + m.m[4]*m.m[4])); }

/* =========================================================================
* CAMERA 2D (GtCamera)
* =========================================================================
* Caméra avec position, zoom, rotation.
* Produit une matrice view (world->screen) et projection combinée.
* Coordonnées monde : Y vers le haut (standard mathématique)
* Coordonnées écran : Y vers le bas (GDI)
* La caméra gère la conversion automatiquement.
* ========================================================================= */
typedef struct GtCamera {
    GtVec2 position;    // Position monde (centre de la vue)
    float rotation;     // Rotation en radians
    float zoom;         // Zoom (1.0 = 1:1, 2.0 = 2x, 0.5 = 0.5x)
    int viewport_w;     // Largeur viewport (pixels écran)
    int viewport_h;     // Hauteur viewport (pixels écran)
    GtMat3 view_matrix;     // World -> Screen (mis à jour par gtCameraUpdate)
    GtMat3 inv_view_matrix; // Screen -> World (mis à jour par gtCameraUpdate)

    // Cache de mise à jour (lazy) — géré par la lib, ne pas modifier.
    // Permet de détecter les écritures directes dans position/rotation/zoom/
    // viewport SANS appeler gtCameraUpdate : les accesseurs recalent alors
    // les matrices automatiquement (plus de résultats silencieusement faux).
    GtVec2 _cache_pos;
    float  _cache_rot;
    float  _cache_zoom;
    int    _cache_vpw;
    int    _cache_vph;
    bool   _cache_valid;
} GtCamera;

// Crée une caméra centrée sur (0,0) avec zoom 1.0
static inline GtCamera gtCameraCreate(GtVec2 position, float zoom, int viewport_w, int viewport_h) {
    GtCamera cam = { position, 0.0f, zoom, viewport_w, viewport_h, {{0}}, {{0}},
                     {0, 0}, 0.0f, 0.0f, 0, 0, false };
    return cam;
}

// Met à jour les matrices view / inverse_view de la caméra
// À appeler quand position/rotation/zoom/viewport changent
static inline void gtCameraUpdate(GtCamera* cam) {
    if (!cam) return;
    // View = T(-pos) * R(-rot) * S(zoom) * FlipY * T(viewport/2)
    // FlipY convertit monde (Y up) -> écran (Y down)
    GtMat3 flip_y = {{ 1,0,0,  0,-1,0,  0,0,1 }};
    GtMat3 center = gtMat3Translate(cam->viewport_w * 0.5f, cam->viewport_h * 0.5f);
    GtMat3 scale = gtMat3ScaleUniform(cam->zoom);
    GtMat3 rot = gtMat3Rotate(-cam->rotation);
    GtMat3 trans = gtMat3Translate(-cam->position.x, -cam->position.y);
    
    // M = center * flip_y * scale * rot * trans
    cam->view_matrix = gtMat3Mul(center, gtMat3Mul(flip_y, gtMat3Mul(scale, gtMat3Mul(rot, trans))));
    cam->inv_view_matrix = gtMat3Inverse(cam->view_matrix);

    cam->_cache_pos = cam->position;
    cam->_cache_rot = cam->rotation;
    cam->_cache_zoom = cam->zoom;
    cam->_cache_vpw = cam->viewport_w;
    cam->_cache_vph = cam->viewport_h;
    cam->_cache_valid = true;
}

// Recale les matrices si la caméra a été modifiée sans gtCameraUpdate
// (écritures directes dans les champs). No-op si tout est à jour.
static inline void gtCameraEnsureUpdated(const GtCamera* cam_const) {
    GtCamera* cam = (GtCamera*)cam_const;  // ne touche qu'au cache interne
    if (!cam) return;
    if (!cam->_cache_valid ||
        cam->_cache_pos.x != cam->position.x || cam->_cache_pos.y != cam->position.y ||
        cam->_cache_rot != cam->rotation || cam->_cache_zoom != cam->zoom ||
        cam->_cache_vpw != cam->viewport_w || cam->_cache_vph != cam->viewport_h) {
        gtCameraUpdate(cam);
    }
}

// Convertit coordonnées monde -> écran (pixels)
static inline GtVec2 gtCameraWorldToScreen(const GtCamera* cam, GtVec2 world) {
    gtCameraEnsureUpdated(cam);
    return gtMat3MulVec2(cam->view_matrix, world);
}

// Convertit coordonnées écran -> monde
static inline GtVec2 gtCameraScreenToWorld(const GtCamera* cam, GtVec2 screen) {
    gtCameraEnsureUpdated(cam);
    return gtMat3MulVec2(cam->inv_view_matrix, screen);
}

// Récupère la matrice view-projection combinée pour le renderer
static inline GtMat3 gtCameraGetViewProj(const GtCamera* cam) {
    gtCameraEnsureUpdated(cam);
    return cam->view_matrix;
}

/* =========================================================================
* TRANSFORM HELPERS (pour entités / sprites)
* =========================================================================
* Crée une matrice modèle (model matrix) depuis position, rotation, scale, origin
* origin : ancrage relatif (0,0 = coin haut-gauche, 0.5,0.5 = centre, 1,1 = coin bas-droit)
* Utilisé pour dessiner sprites/entités avec transform complète.
* ========================================================================= */
static inline GtMat3 gtTransformModel(GtVec2 pos, float rot, float scale, GtVec2 origin, GtVec2 size) {
    // M = T(pos) * R(rot) * S(scale) * T(-origin * size)
    GtMat3 t_pos = gtMat3Translate(pos.x, pos.y);
    GtMat3 t_rot = gtMat3Rotate(rot);
    GtMat3 t_scale = gtMat3ScaleUniform(scale);
    GtMat3 t_origin = gtMat3Translate(-origin.x * size.x, -origin.y * size.y);
    return gtMat3Mul(t_pos, gtMat3Mul(t_rot, gtMat3Mul(t_scale, t_origin)));
}

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

// Récupère le delta de la molette souris CETTE frame (stable : plusieurs
// lectures par frame retournent la même valeur ; vidangé au prochain appel
// de gtEventsWindow). Positif = scroll vers le haut (loin de l'utilisateur).
int gtGetMouseWheelDelta(GtWindow* window);

// Récupère les caractères tapés au clavier CETTE frame (WM_CHAR, via TranslateMessage).
// Chaîne ANSI terminée par '\0', valide jusqu'au prochain gtEventsWindow ; plusieurs
// appels dans la même frame retournent la même chaîne. Les caractères de contrôle
// sont stockés tels quels : '\b' (8) backspace, '\t' (9) tab, '\r' (13) entrée,
// 27 échappe — c'est à l'appelant de les interpréter (p.ex. effacer le dernier
// caractère sur '\b'). Seuls les ASCII 32-126 sont rendus par la police bitmap.
// Buffer limité à 31 caractères par frame (les excédents sont ignorés).
const char* gtGetTypedText(GtWindow* window);

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
* RENDERER (Backend GDI)
* =========================================================================
* Pourquoi une abstraction Renderer ?
*   - Centralise l'API de dessin autour du backend GDI
*   - Permet de changer de backend sans toucher au code du jeu
*   - Prépare l'architecture pour : anti-aliasing, batching, shaders, 3D
*   - GDI = fallback simple, CPU-only, compatible partout (WinXP+)
*   - GDI = rendu CPU simple et prévisible
*
* Architecture :
*   GtRenderer (opaque) contient :
*     - window : pointeur vers la fenêtre cible
*     - type   : GT_RENDERER_GDI
*     - hdc    : Device Context GDI (pour blitting final)
*     - aucun état backend supplémentaire requis
*
* Flux de rendu typique :
*   GtRenderer* r = gtCreateRenderer(win, GT_RENDERER_GDI);
*   while (!gtShouldClose(win)) {
*       gtBeginFrame();
*       gtEventsWindow(win);
*
*       gtRendererBegin(r);           // Prépare le frame
*       gtRendererClear(r, GT_BLACK); // Efface le framebuffer
*       gtRendererDrawRect(r, ...);   // Dessine via API Renderer
*       gtRendererDrawCircle(r, ...);
*       gtRendererEnd(r);             // Affiche le framebuffer via GDI
*   }
*   gtDestroyRenderer(r);
*
* Note : Les anciennes fonctions gtDraw* sur GtWindow restent disponibles
*        pour compatibilité et prototypage rapide.
* ========================================================================= */

// Type de backend de rendu
// - GT_RENDERER_GDI : software (framebuffer CPU + StretchDIBits), défaut, toujours dispo.
// - GT_RENDERER_D2D : Direct2D (GPU, anti-aliasing, texte HQ DirectWrite).
//   Linker avec -ld2d1 -ldwrite -lole32. Désactivable à la compilation avec
//   LIBGT_NO_D2D (gtCreateRenderer(..., GT_RENDERER_D2D) retournera alors NULL).
typedef enum {
    GT_RENDERER_GDI,  // GDI software (framebuffer CPU + StretchDIBits) - défaut, toujours dispo
    GT_RENDERER_D2D   // Direct2D (GPU) : primitives AA, images, texte bitmap via atlas + texte HQ DWrite
} GtRendererType;

// Structure opaque du renderer (détails dans section IMPLEMENTATION)
typedef struct GtRenderer GtRenderer;

// Crée un renderer pour une fenêtre donnée
// type : GT_RENDERER_GDI ou GT_RENDERER_D2D
// D2D : tente un render target matériel, puis logiciel (D2D1_RENDER_TARGET_TYPE_SOFTWARE),
//       retourne NULL si Direct2D est indisponible (utiliser gtRendererGetType pour vérifier).
// Retourne NULL si échec (fenêtre invalide, OOM)
GtRenderer* gtCreateRenderer(GtWindow* window, GtRendererType type);

// Détruit le renderer et libère ses ressources (ReleaseDC pour GDI, COM pour D2D)
void        gtDestroyRenderer(GtRenderer* renderer);

// Retourne le backend effectivement utilisé (GT_RENDERER_GDI ou GT_RENDERER_D2D)
GtRendererType gtRendererGetType(const GtRenderer* renderer);

// (D2D) Active/désactive l'anti-aliasing des primitives (défaut : activé).
// OFF = D2D1_ANTIALIAS_MODE_ALIASED (rendu pixel-exact, utile pour le pixel-art).
// Sans effet sur le backend GDI.
void gtRendererSetAntialias(GtRenderer* renderer, bool antialias);

// =========================================================================
// API DE DESSIN VIA RENDERER (remplace gtDraw* sur GtWindow)
// =========================================================================

// Début de frame : GDI n'a rien à préparer
// À appeler AVANT tout dessin dans la frame
void gtRendererBegin(GtRenderer* renderer);

// Fin de frame : présente le résultat à l'écran
// GDI = StretchDIBits (blit framebuffer -> window)
// GDI : le framebuffer est présenté à la fin de la frame
void gtRendererEnd(GtRenderer* renderer);

// Efface le framebuffer avec une couleur (délègue à gtClearWindow)
void gtRendererClear(GtRenderer* renderer, GtColor color);

// Primitives de dessin (délèguent aux gtDraw* de GtWindow pour GDI)
// Pour GDI : dessinent directement dans le framebuffer CPU.
void gtRendererDrawPixel(GtRenderer* renderer, int x, int y, GtColor color);
void gtRendererDrawRect(GtRenderer* renderer, int x, int y, int w, int h, GtColor color);
void gtRendererDrawRectLines(GtRenderer* renderer, int x, int y, int w, int h, GtColor color);
void gtRendererDrawLine(GtRenderer* renderer, int x1, int y1, int x2, int y2, GtColor color);
void gtRendererDrawCircle(GtRenderer* renderer, int cx, int cy, int radius, GtColor color);
void gtRendererDrawCircleLines(GtRenderer* renderer, int cx, int cy, int radius, GtColor color);

/* =========================================================================
* TRANSFORMATIONS 2D VIA RENDERER (Camera, World↔Screen, Matrix stack)
* =========================================================================
* Le renderer maintient une matrice de transformation courante (model-view-projection)
* qui est appliquée à toutes les primitives de dessin suivantes.
* Pile de matrices (push/pop) pour transformations imbriquées (ex: UI dans monde).
* ========================================================================= */

// Définit la matrice de transformation courante (remplace l'existante)
// Passe NULL pour réinitialiser à l'identité (pixels écran directs)
void gtRendererSetTransform(GtRenderer* renderer, const GtMat3* transform);

// Récupère la matrice de transformation courante (peut être NULL si identité)
const GtMat3* gtRendererGetTransform(const GtRenderer* renderer);

// Push/pop de la pile de transformations (imbrication)
void gtRendererPushTransform(GtRenderer* renderer);
void gtRendererPopTransform(GtRenderer* renderer);

// Multiplie la matrice courante par une matrice (post-multiplication: current * m)
void gtRendererMultiplyTransform(GtRenderer* renderer, const GtMat3* m);

// Helpers pour transformations courantes (appliquées sur la matrice courante)
void gtRendererTranslate(GtRenderer* renderer, float tx, float ty);
void gtRendererRotate(GtRenderer* renderer, float angle_rad);
void gtRendererScale(GtRenderer* renderer, float sx, float sy);

// Applique une caméra (définit view matrix = camera view-proj)
// Le viewport de la caméra est synchronisé automatiquement au resize de la
// fenêtre — appeler gtCameraUpdate ensuite pour rafraîchir la view matrix.
void gtRendererSetCamera(GtRenderer* renderer, const GtCamera* camera);

// Récupère la caméra active (si définie via gtRendererSetCamera)
const GtCamera* gtRendererGetCamera(const GtRenderer* renderer);

// Conversion coordonnées monde <-> écran via la caméra/transform courante
GtVec2 gtRendererWorldToScreen(const GtRenderer* renderer, GtVec2 world);
GtVec2 gtRendererScreenToWorld(const GtRenderer* renderer, GtVec2 screen);

/* =========================================================================
* SCISSOR / CLIPPING RECT (2.4)
* =========================================================================
* Rectangle de découpage (scissor test) : limite le dessin à une zone rectangulaire.
* Pile de rectangles pour imbrication (UI scrollable, viewports, split-screen).
* Coordonnées en espace ÉCRAN (pixels, origine haut-gauche).
* ========================================================================= */

// Définit le rectangle de clipping courant (NULL = pas de clipping / tout l'écran)
void gtRendererSetScissorRect(GtRenderer* renderer, int x, int y, int w, int h);

// Push/pop du rectangle de clipping (imbrication)
void gtRendererPushScissorRect(GtRenderer* renderer, int x, int y, int w, int h);
void gtRendererPopScissorRect(GtRenderer* renderer);

// Récupère le rectangle de clipping actif
void gtRendererGetScissorRect(const GtRenderer* renderer, int* out_x, int* out_y, int* out_w, int* out_h);

// Dessine un caractère unique (police bitmap 8x8). ASCII 32-126 rendu ;
// hors plage : ignoré. Route vers l'atlas GPU si un renderer D2D est attaché.
void gtDrawChar(GtWindow* window, int x, int y, char c, uint32_t color);

// Dessine une chaîne formatée style printf (buffer interne 512 octets, tronqué
// au-delà). Même rendu que gtDrawText (police 8x8), routing D2D identique.
void gtDrawTextFmt(GtWindow* window, int x, int y, uint32_t color, const char* fmt, ...);

// Dessine du texte via renderer
void gtRendererDrawText(GtRenderer* renderer, int x, int y, const char* text, GtColor color);
void gtRendererDrawTextEx(GtRenderer* renderer, int x, int y, const char* text, GtColor color,
                          float scale, int spacing, int wrap_width);

// Texte formaté style printf via renderer (transforme le point d'origine comme
// gtRendererDrawText, buffer interne 512 octets).
void gtRendererDrawTextFmt(GtRenderer* renderer, int x, int y, GtColor color, const char* fmt, ...);

// Texte haute qualité (DirectWrite) - D2D uniquement
// font_px : taille de police en pixels. La police est configurable via
// gtRendererSetHqFontName (défaut : "Segoe UI"). Sur le backend GDI, fallback
// propre : rendu via la police bitmap 8x8 à l'échelle font_px/8.
void gtRendererDrawTextHq(GtRenderer* renderer, float x, float y, const char* text,
                          float font_px, GtColor color);
// (D2D) Change la police du texte HQ (copiée en interne). Prend effet au prochain
// gtRendererDrawTextHq. Sans effet sur le backend GDI.
void gtRendererSetHqFontName(GtRenderer* renderer, const char* font_name);

// Dessin d'image via renderer
// image : pointeur GtImage* retourné par gtLoadImage
// x, y : position coin haut-gauche
// tint : couleur multiplicative (GT_WHITE = pas de teinte)
void gtRendererDrawImage(GtRenderer* renderer, GtImage* image, int x, int y, GtColor tint);
void gtRendererDrawImageEx(GtRenderer* renderer, GtImage* image, int x, int y, int w, int h,
                           float rot, GtVec2 origin, bool flip_x, bool flip_y, GtColor tint);

// Dessin d'image avec transformation complète (matrice modèle)
void gtRendererDrawImageTransformed(GtRenderer* renderer, GtImage* image, const GtMat3* model, GtColor tint);

// Dessin d'un SOUS-RECTANGLE de l'image (spritesheets / animation).
// Coordonnées UV normalisées [0..1] : (0,0) = coin haut-gauche de l'image,
// (1,1) = coin bas-droit. Frame i d'une grille de N colonnes horizontales :
//   u0 = i/N ; u1 = (i+1)/N ; v0 = 0 ; v1 = 1
// (w,h) = taille à l'écran du sous-rect (nearest-neighbor). Les flips
// miroirent le sous-rect. Pour la rotation, gtRendererDrawImageTransformed.
void gtRendererDrawImageUV(GtRenderer* renderer, GtImage* image,
                           int x, int y, int w, int h,
                           float u0, float v0, float u1, float v1,
                           bool flip_x, bool flip_y, GtColor tint);

/* =========================================================================
* BATCH RENDERING (2.1) - Vertex Buffer + Single Draw Call
* =========================================================================
* Permet de soumettre des milliers de sprites/primitives en un seul appel.
* Buffer de vertex dynamique (CPU) -> flush unique par frame.
* Supporte : rects, images, cercles, lignes avec transform/scissor/couleur par vertex.
* ========================================================================= */

// Types de primitives pour le batch
typedef enum {
    GT_BATCH_RECT,        // Rectangle plein
    GT_BATCH_RECT_LINES,  // Rectangle contour
    GT_BATCH_LINE,        // Ligne
    GT_BATCH_CIRCLE,      // Cercle plein
    GT_BATCH_CIRCLE_LINES,// Cercle contour
    GT_BATCH_IMAGE,       // Image (sprite)
    GT_BATCH_TEXT         // Texte (bitmap font)
} GtBatchPrimitiveType;

// Vertex pour le batch rendering (32 bytes aligné)
typedef struct GtBatchVertex {
    float x, y;           // Position écran (après transform)
    float u, v;           // UV (0-1 pour images, inutilisé pour primitives)
    uint32_t color;       // Couleur ARGB
    float radius;         // Rayon pour cercles, échelle pour texte, 0 sinon
    int image_id;         // Index dans le tableau d'images du batch (-1 = pas d'image)
    uint8_t prim_type;    // GtBatchPrimitiveType
    uint8_t glyph;        // Code ASCII (32-126) pour GT_BATCH_TEXT, 0 sinon
    uint8_t pad[2];       // Padding pour alignement 32 bytes
} GtBatchVertex;

// Contrat de layout : les deux backends (et un futur backend D3D11) s'appuient
// sur cette taille — toute évolution du struct doit passer par ici.
_Static_assert(sizeof(GtBatchVertex) == 32, "GtBatchVertex doit faire 32 octets");

// Configuration du batch renderer
typedef struct GtBatchConfig {
    int max_vertices;     // Capacité max vertices (défaut: 65536)
    int max_images;       // Max images différentes par batch (défaut: 256)
    bool auto_flush;      // Flush auto quand buffer plein
} GtBatchConfig;

// État du batch renderer (opaque)
typedef struct GtBatchRenderer GtBatchRenderer;

// Crée un batch renderer attaché à un renderer
GtBatchRenderer* gtBatchCreate(GtRenderer* renderer, const GtBatchConfig* config);

// Détruit le batch renderer
void gtBatchDestroy(GtBatchRenderer* batch);

// Début d'un batch (appeler une fois par frame, avant tout ajout)
void gtBatchBegin(GtBatchRenderer* batch);

// Fin du batch + flush vers le renderer (appeler une fois par frame, après tout ajout)
void gtBatchEnd(GtBatchRenderer* batch);

// Flush manuel (vide le buffer vers le renderer sans finir le batch)
void gtBatchFlush(GtBatchRenderer* batch);

// Ajoute un rectangle au batch
void gtBatchAddRect(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color);

// Ajoute un rectangle contour au batch
void gtBatchAddRectLines(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color);

// Ajoute une ligne au batch
void gtBatchAddLine(GtBatchRenderer* batch, float x1, float y1, float x2, float y2, GtColor color);

// Ajoute un cercle plein au batch
void gtBatchAddCircle(GtBatchRenderer* batch, float cx, float cy, float radius, GtColor color);

// Ajoute un cercle contour au batch
void gtBatchAddCircleLines(GtBatchRenderer* batch, float cx, float cy, float radius, GtColor color);

// Ajoute une image (sprite) au batch
// image : GtImage* chargé, uv_rect : portion de l'image à utiliser (0,0,1,1 = image complète)
// origin : ancrage (0,0=coin, 0.5,0.5=centre), rot : rotation radians, scale : échelle
void gtBatchAddImage(GtBatchRenderer* batch, GtImage* image, float x, float y, 
                     float w, float h, GtColor tint,
                     float u0, float v0, float u1, float v1,
                     GtVec2 origin, float rot, float scale);

// Ajoute du texte au batch (bitmap font 8x8)
void gtBatchAddText(GtBatchRenderer* batch, const char* text, float x, float y, 
                    GtColor color, float scale, int spacing);

// Définit la transform courante pour les primitives suivantes dans le batch
void gtBatchSetTransform(GtBatchRenderer* batch, const GtMat3* transform);

// Push/pop transform dans le batch
void gtBatchPushTransform(GtBatchRenderer* batch);
void gtBatchPopTransform(GtBatchRenderer* batch);

// Transform stack helpers (translation, rotation, scale)
void gtBatchTranslate(GtBatchRenderer* batch, float tx, float ty);
void gtBatchRotate(GtBatchRenderer* batch, float angle_rad);
void gtBatchScale(GtBatchRenderer* batch, float sx, float sy);

// Définit le scissor rect pour le batch
void gtBatchSetScissorRect(GtBatchRenderer* batch, int x, int y, int w, int h);

// Récupère les stats du batch (vertices utilisés, draw calls, etc.)
typedef struct {
    int vertices_used;
    int max_vertices;
    int flush_count;
    int draw_calls;
} GtBatchStats;
void gtBatchGetStats(const GtBatchRenderer* batch, GtBatchStats* out_stats);

/* =========================================================================
* INPUT MAPPING (Actions → Bindings, Gamepad, Profils Joueur, Rebinding)
* ========================================================================= */
/* Architecture :
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
// Le nom est détenu par la bibliothèque (copie interne), voir la convention
// d'ownership documentée après GtPlayerProfile.
#define GT_MAX_BINDINGS_PER_ACTION 8
typedef struct {
    const char* name;                    // "move_left", "jump", etc. (copie possédée)
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
    const char* name;                    // "gameplay", "menu", "debug" (copie possédée)
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

// Convention d'ownership des noms : la bibliothèque COPIE toujours les noms
// (gtInputCreateMap, gtInputAddAction, gtInputLoadProfile). Les chaînes
// passées par l'appelant peuvent donc être temporaires. En contrepartie,
// un profil contenant des maps doit être libéré via gtInputDestroyProfile
// (ou gtInputDestroy pour les profils possédés par le système).

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
// Libère les noms dupliqués d'un profil possédé par l'appelant (après gtInputLoadProfile)
void             gtInputDestroyProfile(GtPlayerProfile* profile);

// Action Maps
GtActionMap*     gtInputCreateMap(const char* name);   // le nom est copié en interne
void             gtInputDestroyMap(GtActionMap* map);
void             gtInputAddAction(GtActionMap* map, const char* action_name); // idem : copié
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
#include <stdarg.h>       // va_list (gtDrawTextFmt, gtRendererDrawTextFmt)

// Backend Direct2D (optionnel : LIBGT_NO_D2D pour construire sans).
// initguid.h AVANT d2d1.h/dwrite.h : émet les définitions des IID dans ce TU
// (indispensable : libuuid.a ne contient aucun IID DirectWrite).
// Linker avec : -ld2d1 -ldwrite -lole32
#ifndef LIBGT_NO_D2D
#include <initguid.h>
#ifndef COBJMACROS
#define COBJMACROS
#define LIBGT_TMP_COBJMACROS
#endif
#include <d2d1.h>
#include <dwrite.h>
#ifdef LIBGT_TMP_COBJMACROS
#undef COBJMACROS
#undef LIBGT_TMP_COBJMACROS
#endif
#endif // LIBGT_NO_D2D

// Le nom de classe est partagé par toutes les fenêtres du processus.
// On accepte ERROR_CLASS_ALREADY_EXISTS lors des créations suivantes.

/* -------------------------------------------------------------------------
* DIAGNOSTICS D'ERREURS (last-error + callback optionnel)
* ------------------------------------------------------------------------- */
static char g_gtLastError[256] = {0};
static GtErrorCallback g_gtErrorCallback = NULL;
static void* g_gtErrorUserData = NULL;

void gtSetErrorCallback(GtErrorCallback callback, void* user_data) {
    g_gtErrorCallback = callback;
    g_gtErrorUserData = user_data;
}

const char* gtGetLastError(void) {
    return g_gtLastError;
}

// Enregistre le dernier message d'erreur et notifie le callback éventuel
static void gtReportError(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    strcpy(g_gtLastError, buffer);  // borné par vsnprintf
    if (g_gtErrorCallback) g_gtErrorCallback(buffer, g_gtErrorUserData);
}

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

#ifndef LIBGT_NO_D2D
    // Cache backend Direct2D (bitmap prémultipliée uploadée, tag owner = RT)
    void* d2d_bitmap;
    void* d2d_owner;
#endif
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

// Glyphes accentues (CP-1252) : accent compresse sur les lignes 0-1, lettre
// sur les lignes 2-7 (cedille sous la lettre pour c). Style coherent avec
// gt_font8x8. Index globaux 95..126 (cf. gtGlyphIndexForCode).
static const uint8_t gt_font8x8_ext[32 * 8] = {
    // 95 A grave
    0x08, 0x10, 0x18, 0x66, 0x66, 0x7E, 0x66, 0x66,
    // 96 A circonflexe
    0x10, 0x38, 0x18, 0x66, 0x66, 0x7E, 0x66, 0x66,
    // 97 A trema
    0x24, 0x00, 0x18, 0x66, 0x66, 0x7E, 0x66, 0x66,
    // 98 C cedille
    0x3C, 0x66, 0x60, 0x60, 0x60, 0x3C, 0x10, 0x20,
    // 99 E aigu
    0x20, 0x10, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E,
    // 100 E grave
    0x08, 0x10, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E,
    // 101 E circonflexe
    0x10, 0x38, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E,
    // 102 E trema
    0x24, 0x00, 0x7E, 0x60, 0x7C, 0x60, 0x60, 0x7E,
    // 103 I circonflexe
    0x10, 0x38, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C,
    // 104 I trema
    0x24, 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C,
    // 105 O circonflexe
    0x10, 0x38, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C,
    // 106 O trema
    0x24, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C,
    // 107 U grave
    0x08, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C,
    // 108 U circonflexe
    0x10, 0x38, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C,
    // 109 U trema
    0x24, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C,
    // 110 a grave
    0x08, 0x10, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E,
    // 111 a circonflexe
    0x10, 0x38, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E,
    // 112 a trema
    0x24, 0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E,
    // 113 c cedille
    0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C, 0x10, 0x20,
    // 114 e aigu
    0x20, 0x10, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C,
    // 115 e grave
    0x08, 0x10, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C,
    // 116 e circonflexe
    0x10, 0x38, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C,
    // 117 e trema
    0x24, 0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C,
    // 118 i circonflexe
    0x10, 0x38, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C,
    // 119 i trema
    0x00, 0x24, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C,
    // 120 o circonflexe
    0x10, 0x38, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C,
    // 121 o trema
    0x24, 0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C,
    // 122 u grave
    0x08, 0x10, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E,
    // 123 u circonflexe
    0x10, 0x38, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E,
    // 124 u trema
    0x24, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E,
    // 125 guillemet ouvrant
    0x00, 0x00, 0x24, 0x12, 0x24, 0x00, 0x00, 0x00,
    // 126 guillemet fermant
    0x00, 0x00, 0x24, 0x48, 0x24, 0x00, 0x00, 0x00,
};

// Nombre total de glyphes rendables (ASCII + accents)
#define GT_FONT_GLYPH_COUNT 127

// Index de glyphe pour un code CP-1252 : 0-94 = ASCII, 95-126 = accents, -1 = ignore
static int gtGlyphIndexForCode(int code) {
    if (code >= 32 && code <= 126) return code - 32;
    switch (code) {
        case 0xC0: return 95;  case 0xC2: return 96;  case 0xC4: return 97;
        case 0xC7: return 98;  case 0xC9: return 99;  case 0xC8: return 100;
        case 0xCA: return 101; case 0xCB: return 102; case 0xCE: return 103;
        case 0xCF: return 104; case 0xD4: return 105; case 0xD6: return 106;
        case 0xD9: return 107; case 0xDB: return 108; case 0xDC: return 109;
        case 0xE0: return 110; case 0xE2: return 111; case 0xE4: return 112;
        case 0xE7: return 113; case 0xE9: return 114; case 0xE8: return 115;
        case 0xEA: return 116; case 0xEB: return 117; case 0xEE: return 118;
        case 0xEF: return 119; case 0xF4: return 120; case 0xF6: return 121;
        case 0xF9: return 122; case 0xFB: return 123; case 0xFC: return 124;
        case 0xAB: return 125; case 0xBB: return 126;
        default: return -1;
    }
}

// Donnees 8 octets du glyphe d'index donne (NULL si hors plage)
static const uint8_t* gtFontGlyphData(int index) {
    if (index < 0 || index >= GT_FONT_GLYPH_COUNT) return NULL;
    if (index < 95) return &gt_font8x8[index * 8];
    return &gt_font8x8_ext[(index - 95) * 8];
}

// Decode le prochain caractere logique d'une chaine : gere l'UTF-8 2 octets
// (U+00A0..U+00FF, accents latins) en le convertissant en code CP-1252
// equivalent, et laisse passer les octets hauts directs (WM_CHAR CP-1252).
// Retourne le code 0-255, ou -1 en fin de chaine. Avance *p.
static int gtTextNextGlyph(const char** p) {
    const unsigned char* u = (const unsigned char*)*p;
    if (!u[0]) return -1;
    if (u[0] == 0xC3 && u[1] >= 0x80 && u[1] <= 0xBF) {  // U+00C0..U+00FF
        *p += 2;
        return 0xC0 + (u[1] & 0x3F);
    }
    if (u[0] == 0xC2 && u[1] >= 0x80 && u[1] <= 0xBF) {  // U+00A0..U+00BF
        *p += 2;
        return 0x80 + (u[1] & 0x3F);
    }
    (*p)++;
    return u[0];
}

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

    // Renderer attaché (back-pointer posé par gtCreateRenderer). En mode D2D,
    // les gtDraw* window-level sont routés vers le GPU et win->buffer devient
    // obsolète (non mis à jour) — le z-order reste l'ordre d'appel.
    struct GtRenderer* renderer;

    bool keys[256];              // État clavier : true = enfoncée (index = code VK)
    uint8_t key_pressed_count[256];  // Appuis FRAIS cette frame (hors auto-repeat) —
    uint8_t key_released_count[256]; // vidés au début de chaque gtEventsWindow
    bool mouse_buttons[3];       // État souris : [0]=gauche, [1]=droit, [2]=milieu
    uint8_t mouse_pressed_count[3];  // Clics frais cette frame (détecte down+up
    uint8_t mouse_released_count[3]; // rapide entre deux frames, contrairement à un booléen)
    int  mouse_x;                // Position X souris relative zone cliente
    int  mouse_y;                // Position Y souris relative zone cliente
    int  mouse_wheel_delta;      // Delta molette accumulé cette frame (reset après lecture)
    bool mouse_tracking;         // Suivi WM_MOUSELEAVE actif (TrackMouseEvent)

    char text_input[32];         // Caractères tapés (WM_CHAR) cette frame — vidée au
    int  text_input_len;         // début de chaque gtEventsWindow (cf. gtGetTypedText)
};

/* -------------------------------------------------------------------------
* BACKEND DIRECT2D - DÉLÉGATIONS ANTICIPÉES
* -------------------------------------------------------------------------
* Le cœur D2D est implémenté dans la section RENDERER (plus bas, après
* struct GtRenderer). Ces prototypes permettent de router les fonctions
* window-level (définies avant) vers le GPU quand un renderer D2D est
* attaché à la fenêtre. Toutes retournent/faillent proprement si le
* backend D2D est désactivé (LIBGT_NO_D2D) ou absent.
* ------------------------------------------------------------------------- */
#ifndef LIBGT_NO_D2D
static bool gtD2DWinClear(GtWindow* win, uint32_t color);
static bool gtD2DWinDrawPixel(GtWindow* win, int x, int y, uint32_t color);
static bool gtD2DWinDrawRect(GtWindow* win, int x, int y, int w, int h, uint32_t color);
static bool gtD2DWinDrawRectLines(GtWindow* win, int x, int y, int w, int h, uint32_t color);
static bool gtD2DWinDrawLine(GtWindow* win, int x1, int y1, int x2, int y2, uint32_t color);
static bool gtD2DWinDrawCircle(GtWindow* win, int cx, int cy, int radius, uint32_t color);
static bool gtD2DWinDrawCircleLines(GtWindow* win, int cx, int cy, int radius, uint32_t color);
static void gtD2DWindowSize(GtWindow* win, int new_width, int new_height);
static bool gtWindowD2DPresent(GtWindow* win);  // EndDraw si frame D2D en cours
#endif
static void gtRendererSyncCameraViewport(GtWindow* win, int new_width, int new_height);

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
        if (win && wParam < 256) {
            win->keys[wParam] = true;
            // Bit 30 de lParam = auto-repeat : ne compte que les appuis FRAIS
            if (!(lParam & (1u << 30)) && win->key_pressed_count[wParam] < 255)
                win->key_pressed_count[wParam]++;
        }
        break;

    case WM_SYSKEYDOWN:   // Touche système (Alt+...) enfoncée
        if (win && wParam < 256) {
            win->keys[wParam] = true;
            if (!(lParam & (1u << 30)) && win->key_pressed_count[wParam] < 255)
                win->key_pressed_count[wParam]++;
        }
        // Laisser DefWindowProc gérer les raccourcis système (notamment Alt+F4).
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    case WM_KEYUP:        // Touche relâchée
        if (win && wParam < 256) {
            win->keys[wParam] = false;
            if (win->key_released_count[wParam] < 255)
                win->key_released_count[wParam]++;
        }
        break;

    case WM_SYSKEYUP:     // Touche système (Alt+...) relâchée
        if (win && wParam < 256) {
            win->keys[wParam] = false;
            if (win->key_released_count[wParam] < 255)
                win->key_released_count[wParam]++;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);

    case WM_CHAR:         // Caractère tapé (généré par TranslateMessage depuis WM_KEYDOWN)
        // Inclut les caractères de contrôle : '\b' backspace, '\t' tab, '\r' entrée, 27 échappe.
        // La fenêtre est ANSI (CreateWindowA) : wParam est un caractère de la page code ANSI.
        if (win && win->text_input_len < (int)sizeof(win->text_input) - 1) {
            win->text_input[win->text_input_len] = (char)wParam;
            win->text_input[++win->text_input_len] = '\0';
        }
        break;

    // ===== SOURIS - BOUTONS =====
    case WM_LBUTTONDOWN:  // Clic gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = true;
            if (win->mouse_pressed_count[GT_MOUSE_BUTTON_LEFT] < 255)
                win->mouse_pressed_count[GT_MOUSE_BUTTON_LEFT]++;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_LBUTTONUP:    // Relâche gauche
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_LEFT] = false;
            if (win->mouse_released_count[GT_MOUSE_BUTTON_LEFT] < 255)
                win->mouse_released_count[GT_MOUSE_BUTTON_LEFT]++;
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
            if (win->mouse_pressed_count[GT_MOUSE_BUTTON_RIGHT] < 255)
                win->mouse_pressed_count[GT_MOUSE_BUTTON_RIGHT]++;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_RBUTTONUP:
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_RIGHT] = false;
            if (win->mouse_released_count[GT_MOUSE_BUTTON_RIGHT] < 255)
                win->mouse_released_count[GT_MOUSE_BUTTON_RIGHT]++;
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
            if (win->mouse_pressed_count[GT_MOUSE_BUTTON_MIDDLE] < 255)
                win->mouse_pressed_count[GT_MOUSE_BUTTON_MIDDLE]++;
            if (GetCapture() != hwnd) SetCapture(hwnd);
        }
        break;

    case WM_MBUTTONUP:
        if (win) {
            win->mouse_buttons[GT_MOUSE_BUTTON_MIDDLE] = false;
            if (win->mouse_released_count[GT_MOUSE_BUTTON_MIDDLE] < 255)
                win->mouse_released_count[GT_MOUSE_BUTTON_MIDDLE]++;
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

#ifndef LIBGT_NO_D2D
                    // Backend D2D : redimensionne (ou recrée) le render target
                    gtD2DWindowSize(win, new_width, new_height);
#endif
                    // Caméra attachée : viewport synchronisé (world<->screen
                    // reste correct après resize, sans action de l'appelant)
                    gtRendererSyncCameraViewport(win, new_width, new_height);
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
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons));
                win->mouse_tracking = false;
            }
            break;

        case WM_KILLFOCUS:    // Fenêtre perd le focus clavier
            if (win) {
                memset(win->keys, 0, sizeof(win->keys));            // Relâche toutes touches
                memset(win->mouse_buttons, 0, sizeof(win->mouse_buttons)); // Relâche souris
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
    if (!title || width <= 0 || height <= 0) {
        gtReportError("gtCreateWindow : parametres invalides (titre=%p, %dx%d)",
                      (void*)title, width, height);
        return NULL;
    }
    // Protection contre overflow size_t (width * height * 4 bytes)
    if ((size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) {
        gtReportError("gtCreateWindow : dimensions trop grandes (%dx%d)", width, height);
        return NULL;
    }

    // Alloue la structure fenêtre (zéro-initialisée via calloc)
    GtWindow* win = (GtWindow*)calloc(1, sizeof(GtWindow));
    if (!win) {
        gtReportError("gtCreateWindow : allocation GtWindow OOM");
        return NULL;
    }

    win->width = width;
    win->height = height;
    win->should_close = false;
    win->hInstance = GetModuleHandle(NULL);  // Instance du processus courant
    if (!win->hInstance) {
        gtReportError("gtCreateWindow : GetModuleHandle a echoue");
        free(win);
        return NULL;
    }

    // Enregistre la classe fenêtre Win32 si nécessaire.
    if (!gtRegisterWindowClass(win->hInstance)) {
        gtReportError("gtCreateWindow : enregistrement de la classe fenetre echoue");
        free(win);
        return NULL;
    }
    // Calcule la taille fenêtre complète (avec bordures/titre) depuis zone cliente
    RECT rect = {0, 0, width, height};
    if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
        gtReportError("gtCreateWindow : AdjustWindowRect a echoue");
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
        gtReportError("gtCreateWindow : CreateWindowExA a echoue (erreur %lu)",
                      (unsigned long)GetLastError());
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
        gtReportError("gtCreateWindow : allocation framebuffer OOM (%dx%d)", width, height);
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

    // Vide les événements de la frame précédente AVANT le pump : tout ce qui
    // arrive pendant (WM_KEYDOWN/UP, clics, WM_CHAR, molette) appartient à
    // CETTE frame. Compteurs = aucun appui rapide n'est perdu.
    memset(window->key_pressed_count, 0, sizeof(window->key_pressed_count));
    memset(window->key_released_count, 0, sizeof(window->key_released_count));
    memset(window->mouse_pressed_count, 0, sizeof(window->mouse_pressed_count));
    memset(window->mouse_released_count, 0, sizeof(window->mouse_released_count));
    window->mouse_wheel_delta = 0;

    // Vide la saisie de texte de la frame précédente : les WM_CHAR arrivant
    // pendant le pump ci-dessous s'accumulent pour CETTE frame.
    window->text_input[0] = '\0';
    window->text_input_len = 0;

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
// Mode D2D : termine la frame GPU (EndDraw) au lieu de blitter.
void gtUpdateWindow(GtWindow* window) {
    if (!window || !window->hwnd || !window->buffer) return;
#ifndef LIBGT_NO_D2D
    if (gtWindowD2DPresent(window)) return;  // renderer D2D attaché : présent GPU
#endif
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
// Mode D2D : Clear GPU (win->buffer n'est pas mis à jour)
void gtClearWindow(GtWindow* window, uint32_t color) {
    if (!window) return;
#ifndef LIBGT_NO_D2D
    if (gtD2DWinClear(window, color)) return;
#endif
    if (!window->buffer) return;
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
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawPixel(window, x, y, color)) return;
#endif
    drawPixelClipped64(window, (int64_t)x, (int64_t)y, color);
}

// Dessine un rectangle plein (rempli)
// x,y = coin haut-gauche, w,h = largeur/hauteur en pixels
// Clipping : intersection avec [0,width-1] x [0,height-1]
void gtDrawRect(GtWindow* window, int x, int y, int w, int h, uint32_t color) {
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawRect(window, x, y, w, h, color)) return;
#endif
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
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawRectLines(window, x, y, w, h, color)) return;
#endif
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
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawLine(window, x1, y1, x2, y2, color)) return;
#endif
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
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawCircleLines(window, cx, cy, radius, color)) return;
#endif
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
#ifndef LIBGT_NO_D2D
    if (gtD2DWinDrawCircle(window, cx, cy, radius, color)) return;
#endif
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
    return window->key_pressed_count[keycode] > 0;  // appui frais cette frame
}

// Vérifie si une touche vient d'être relâchée CETTE frame (edge detection)
// Retourne true seulement sur la frame où la touche passe de enfoncée -> relâchée
bool gtWasKeyReleased(GtWindow* window, int keycode) {
    if (!window || keycode < 0 || keycode >= 256) return false;
    return window->key_released_count[keycode] > 0; // relâchement frais cette frame
}

// Vérifie si un bouton souris vient d'être pressé CETTE frame
bool gtWasMouseButtonPressed(GtWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return window->mouse_pressed_count[button] > 0;  // clic frais cette frame
}

// Vérifie si un bouton souris vient d'être relâché CETTE frame
bool gtWasMouseButtonReleased(GtWindow* window, int button) {
    if (!window || button < 0 || button >= 3) return false;
    return window->mouse_released_count[button] > 0; // relâchement frais cette frame
}

// Récupère le delta de la molette souris depuis la dernière frame
// Positif = scroll vers le haut (loin de l'utilisateur), Négatif = scroll vers le bas
// Remet à zéro après lecture (consommer l'événement)
int gtGetMouseWheelDelta(GtWindow* window) {
    if (!window) return 0;
    // Lecture STABLE : vidangé au début de chaque gtEventsWindow, pas ici —
    // plusieurs systèmes peuvent lire la même valeur dans la même frame.
    return window->mouse_wheel_delta;
}

// Caractères tapés cette frame : vidés au début de chaque gtEventsWindow,
// remplis pendant le pump de messages, stables jusqu'au prochain appel.
const char* gtGetTypedText(GtWindow* window) {
    if (!window) return "";
    return window->text_input;
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
* IMPLÉMENTATION : RENDERER ABSTRACTION (Backends GDI + Direct2D)
* -------------------------------------------------------------------------
* Structure interne du renderer (opaque pour l'utilisateur).
* GDI : HDC n'est PAS stocké — obtenu via GetDC() à chaque frame dans
*       gtRendererEnd pour éviter invalidation après WM_SIZE, DPI, veille.
* D2D : un ID2D1HwndRenderTarget est créé au premier besoin et redimensionné
*       sur WM_SIZE ; en cas de perte device (D2DERR_RECREATE_TARGET) il est
*       jeté et recréé paresseusement, caches invalidés par tag owner.
* ------------------------------------------------------------------------- */
#define GT_RENDERER_MAX_TRANSFORM_STACK 32
#define GT_RENDERER_MAX_SCISSOR_STACK 32

typedef struct GtRendererScissorState {
    int x, y, w, h;
    bool active;
} GtRendererScissorState;

#ifndef LIBGT_NO_D2D
// Entrée de cache : copie d'image teintée (prémultipliée) pour DrawBitmap.
#define GT_D2D_MAX_TINTED 16
typedef struct GtD2DTintedEntry {
    GtImage* img;
    uint32_t tint;
    ID2D1Bitmap* bmp;
} GtD2DTintedEntry;

// Cache de brosses à indexation directe : les couleurs alternent vite
// (particules, batch) et CreateSolidColorBrush par appel coûte cher.
#define GT_D2D_BRUSH_CACHE 256
typedef struct GtD2DBrushEntry {
    uint32_t color;
    ID2D1SolidColorBrush* brush;
} GtD2DBrushEntry;

// État de "peinture" D2D : ressources dérivées d'un ID2D1RenderTarget.
// Séparé du renderer pour être testable hors-écran (render target WIC).
typedef struct GtD2DPaint {
    GtD2DBrushEntry brushes[GT_D2D_BRUSH_CACHE];  // brosses cachées (hash couleur)
    ID2D1Bitmap* atlas;            // atlas police 8x8 teintée (1 couleur à la fois)
    uint32_t atlas_color;
    GtD2DTintedEntry tinted[GT_D2D_MAX_TINTED];  // copies d'images teintées (éviction circulaire)
    int tinted_next;
} GtD2DPaint;

#define GT_D2D_MAX_FONT_FORMATS 8
typedef struct GtD2DTextFormat {
    IDWriteTextFormat* format;
    float px;
} GtD2DTextFormat;
#endif // LIBGT_NO_D2D

struct GtRenderer {
    GtWindow* window;          // Fenêtre cible
    GtRendererType type;       // Type de backend (GDI)

    // Transform stack (model-view-projection matrix)
    GtMat3 transform_stack[GT_RENDERER_MAX_TRANSFORM_STACK];
    int transform_stack_depth; // 0 = identité (pas de transform)

    // Scissor/Clipping rect stack (en coordonnées écran)
    GtRendererScissorState scissor_stack[GT_RENDERER_MAX_SCISSOR_STACK];
    int scissor_stack_depth;

    // Caméra active (optionnelle)
    GtCamera* camera;
    bool camera_seeded;   // true tant que slot 0 = copie caméra non écrasée par SetTransform

#ifndef LIBGT_NO_D2D
    // ---- Backend Direct2D ----
    ID2D1Factory* d2d_factory;             // Partagé, créé une fois
    ID2D1HwndRenderTarget* d2d_rt;         // NULL = à créer / device perdu
    bool d2d_software;                     // RT logiciel (fallback sans GPU)
    bool d2d_drawing;                      // Entre BeginDraw et EndDraw
    HRESULT d2d_last_hr;                   // Dernier HRESULT d'EndDraw
    int d2d_clip_depth;                    // PushAxisAlignedClip non encore popés
    bool d2d_antialias;                    // Défaut : true (AA activé)
    GtD2DPaint d2d;                        // Ressources dérivées du RT
    // Texte HQ (DirectWrite)
    IDWriteFactory* dwrite_factory;
    IDWriteTextFormat* dwrite_formats[GT_D2D_MAX_FONT_FORMATS];
    float dwrite_format_px[GT_D2D_MAX_FONT_FORMATS];
    int dwrite_format_count;
    int dwrite_evict_next;   // slot a evictor quand le cache est plein
    wchar_t dwrite_font[64];               // Nom de police (défaut "Segoe UI")
#endif
};

// ---------------------------------------------------------------------------
// BACKEND DIRECT2D - CŒUR
// ---------------------------------------------------------------------------
#ifndef LIBGT_NO_D2D

// Conversion couleur : GtColor ARGB droit -> D2D1_COLOR_F.
// NB : les couleurs de BROSSE sont en alpha droit (D2D prémultiplie
// lui-même au blend), contrairement aux données de BITMAP qui doivent
// être prémultipliées à l'upload (cf. gtD2DGetImageBitmap).
static inline D2D1_COLOR_F gtD2DColor(GtColor c) {
    D2D1_COLOR_F out;
    out.r = (float)((c >> 16) & 0xFFu) / 255.0f;
    out.g = (float)((c >> 8) & 0xFFu) / 255.0f;
    out.b = (float)(c & 0xFFu) / 255.0f;
    out.a = (float)((c >> 24) & 0xFFu) / 255.0f;
    return out;
}

// Conversion GtMat3 (affine, colonne-majeur : x' = m0*x + m3*y + m6) vers
// D2D1_MATRIX_3X2_F (row-vector : x' = x*_11 + y*_21 + dx). Mapping exact :
// _11=m0, _12=m1, _21=m3, _22=m4, dx=m6, dy=m7. NULL = identité.
static inline D2D1_MATRIX_3X2_F gtD2DMatrix(const GtMat3* m) {
    D2D1_MATRIX_3X2_F out;
    if (m) {
        out._11 = m->m[0]; out._12 = m->m[1];
        out._21 = m->m[3]; out._22 = m->m[4];
        out.dx  = m->m[6]; out.dy  = m->m[7];
    } else {
        out._11 = 1.0f; out._12 = 0.0f;
        out._21 = 0.0f; out._22 = 1.0f;
        out.dx  = 0.0f; out.dy  = 0.0f;
    }
    return out;
}

// Libère les ressources dérivées d'un RT (brosses, atlas, copies teintées)
static void gtD2DInvalidatePaint(GtD2DPaint* paint) {
    for (int i = 0; i < GT_D2D_BRUSH_CACHE; i++) {
        if (paint->brushes[i].brush) {
            ID2D1SolidColorBrush_Release(paint->brushes[i].brush);
            paint->brushes[i].brush = NULL;
        }
        paint->brushes[i].color = 0;
    }
    if (paint->atlas) { ID2D1Bitmap_Release(paint->atlas); paint->atlas = NULL; }
    paint->atlas_color = 0;
    for (int i = 0; i < GT_D2D_MAX_TINTED; i++) {
        if (paint->tinted[i].bmp) { ID2D1Bitmap_Release(paint->tinted[i].bmp); paint->tinted[i].bmp = NULL; }
        paint->tinted[i].img = NULL;
        paint->tinted[i].tint = 0;
    }
    paint->tinted_next = 0;
}

// Crée (ou recrée) le render target HWND. Retourne NULL si impossible.
static ID2D1RenderTarget* gtD2DEnsureRT(GtRenderer* r) {
    if (r->d2d_rt) return (ID2D1RenderTarget*)r->d2d_rt;
    if (!r->d2d_factory || !r->window || !r->window->hwnd) return NULL;

    int w = r->window->width > 0 ? r->window->width : 1;
    int h = r->window->height > 0 ? r->window->height : 1;

    D2D1_PIXEL_FORMAT pf;
    pf.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    pf.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

    D2D1_RENDER_TARGET_PROPERTIES rtp;
    rtp.type = r->d2d_software ? D2D1_RENDER_TARGET_TYPE_SOFTWARE : D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rtp.pixelFormat = pf;
    // 96 DPI explicite : 1 unite = 1 pixel. A 0 (DPI bureau), l'espace serait
    // en DIPs et serait etire sur les affichages a mise a l'echelle > 100%.
    rtp.dpiX = 96.0f;
    rtp.dpiY = 96.0f;
    rtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    D2D1_HWND_RENDER_TARGET_PROPERTIES hrtp;
    hrtp.hwnd = r->window->hwnd;
    hrtp.pixelSize.width = (UINT32)w;
    hrtp.pixelSize.height = (UINT32)h;
    // IMMEDIATELY = pas d'attente vsync au EndDraw : la cadence appartient
    // à l'application (comme le blit GDI, non synchronisé)
    hrtp.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;

    ID2D1HwndRenderTarget* rt = NULL;
    HRESULT hr = ID2D1Factory_CreateHwndRenderTarget(r->d2d_factory, &rtp, &hrtp, &rt);
    if (FAILED(hr)) return NULL;

    r->d2d_rt = rt;
    if (r->d2d_antialias) {
        ID2D1RenderTarget_SetAntialiasMode((ID2D1RenderTarget*)rt, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    } else {
        ID2D1RenderTarget_SetAntialiasMode((ID2D1RenderTarget*)rt, D2D1_ANTIALIAS_MODE_ALIASED);
    }
    return (ID2D1RenderTarget*)rt;
}

// BeginDraw paresseux : démarre la frame au premier dessin si pas commencée
static bool gtD2DBeginDraw(GtRenderer* r) {
    ID2D1RenderTarget* rt = gtD2DEnsureRT(r);
    if (!rt) return false;
    if (!r->d2d_drawing) {
        ID2D1RenderTarget_BeginDraw(rt);
        r->d2d_drawing = true;
    }
    return true;
}

// Termine la frame (idempotent). Vérifie le device : en cas de perte
// (D2DERR_RECREATE_TARGET), jette le RT — recréé au prochain dessin,
// caches invalidés par tag owner.
static void gtD2DEndFrame(GtRenderer* r) {
    if (!r || !r->d2d_rt || !r->d2d_drawing) return;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;

    // Équilibre les clips restants (EndDraw exige Push/Pop appariés)
    while (r->d2d_clip_depth > 0) {
        ID2D1RenderTarget_PopAxisAlignedClip(rt);
        r->d2d_clip_depth--;
    }

    D2D1_TAG t1 = 0, t2 = 0;
    HRESULT hr = ID2D1RenderTarget_EndDraw(rt, &t1, &t2);
    r->d2d_drawing = false;
    r->d2d_last_hr = hr;

    if (hr == (HRESULT)D2DERR_RECREATE_TARGET) {
        // Device perdu : jette RT et ressources dérivées
        gtD2DInvalidatePaint(&r->d2d);
        ID2D1HwndRenderTarget_Release(r->d2d_rt);
        r->d2d_rt = NULL;
    }
}

// Brosse cachée (indexation directe par hash couleur) : évite de recréer
// un COM object à chaque draw quand les couleurs alternent.
static ID2D1SolidColorBrush* gtD2DGetBrush(ID2D1RenderTarget* rt, GtD2DPaint* paint, GtColor color) {
    // >> 24 : le modulo seul ne dependait que des 8 bits bas de la couleur
    // (le canal bleu) - deux couleurs partageant leur octet bas s'evictaient.
    uint32_t slot = ((color * 2654435761u) >> 24) % GT_D2D_BRUSH_CACHE;
    GtD2DBrushEntry* e = &paint->brushes[slot];
    if (e->brush && e->color == (uint32_t)color) return e->brush;
    if (e->brush) ID2D1SolidColorBrush_Release(e->brush);
    e->brush = NULL;
    D2D1_COLOR_F c = gtD2DColor(color);
    HRESULT hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &c, NULL, &e->brush);
    if (FAILED(hr)) { e->brush = NULL; return NULL; }
    e->color = (uint32_t)color;
    return e->brush;
}

// ---------------------------------------------------------------------------
// COUCHE PRIMITIVES D2D (fonctionne sur n'importe quel ID2D1RenderTarget :
// HwndRT en production, WIC RT pour les tests hors-écran)
// ---------------------------------------------------------------------------

static void gtD2DFillRect(ID2D1RenderTarget* rt, GtD2DPaint* paint,
                          float x, float y, float w, float h, GtColor color) {
    ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, paint, color);
    if (!b) return;
    D2D1_RECT_F rect;
    rect.left = x; rect.top = y; rect.right = x + w; rect.bottom = y + h;
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush*)b);
}

static void gtD2DFrameRect(ID2D1RenderTarget* rt, GtD2DPaint* paint,
                           float x, float y, float w, float h, GtColor color, float stroke) {
    ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, paint, color);
    if (!b) return;
    D2D1_RECT_F rect;
    rect.left = x; rect.top = y; rect.right = x + w; rect.bottom = y + h;
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush*)b, stroke, NULL);
}

static void gtD2DDrawLineSeg(ID2D1RenderTarget* rt, GtD2DPaint* paint,
                             float x1, float y1, float x2, float y2, GtColor color, float stroke) {
    ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, paint, color);
    if (!b) return;
    D2D1_POINT_2F p0, p1;
    p0.x = x1; p0.y = y1;
    p1.x = x2; p1.y = y2;
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush*)b, stroke, NULL);
}

static void gtD2DFillEllipse(ID2D1RenderTarget* rt, GtD2DPaint* paint,
                             float cx, float cy, float radius, GtColor color) {
    ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, paint, color);
    if (!b) return;
    D2D1_ELLIPSE e;
    e.point.x = cx; e.point.y = cy;
    e.radiusX = radius; e.radiusY = radius;
    ID2D1RenderTarget_FillEllipse(rt, &e, (ID2D1Brush*)b);
}

static void gtD2DStrokeEllipse(ID2D1RenderTarget* rt, GtD2DPaint* paint,
                               float cx, float cy, float radius, GtColor color, float stroke) {
    ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, paint, color);
    if (!b) return;
    D2D1_ELLIPSE e;
    e.point.x = cx; e.point.y = cy;
    e.radiusX = radius; e.radiusY = radius;
    ID2D1RenderTarget_DrawEllipse(rt, &e, (ID2D1Brush*)b, stroke, NULL);
}

// Upload (ou récupère du cache) une GtImage en ID2D1Bitmap prémultipliée.
// Cache taggé owner : recréé si le RT a changé (device perdu).
static ID2D1Bitmap* gtD2DGetImageBitmap(ID2D1RenderTarget* rt, GtImage* img) {
    if (!rt || !img || !img->pixels || img->width <= 0 || img->height <= 0) return NULL;
    if (img->d2d_bitmap && img->d2d_owner == (void*)rt) return (ID2D1Bitmap*)img->d2d_bitmap;
    if (img->d2d_bitmap) { ID2D1Bitmap_Release((ID2D1Bitmap*)img->d2d_bitmap); img->d2d_bitmap = NULL; }

    // Prémultiplie (copie) : pixels sont en alpha droit, D2D veut du premultiplié
    size_t n = (size_t)img->width * (size_t)img->height;
    uint32_t* premul = (uint32_t*)malloc(n * sizeof(uint32_t));
    if (!premul) return NULL;
    for (size_t i = 0; i < n; i++) {
        uint32_t p = img->pixels[i];
        uint32_t a = (p >> 24) & 0xFFu;
        if (a == 255u) {
            premul[i] = p;
        } else if (a == 0u) {
            premul[i] = 0;
        } else {
            uint32_t r = (((p >> 16) & 0xFFu) * a) / 255u;
            uint32_t g = (((p >> 8) & 0xFFu) * a) / 255u;
            uint32_t b = ((p & 0xFFu) * a) / 255u;
            premul[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    D2D1_BITMAP_PROPERTIES props;
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.dpiX = 0.0f;
    props.dpiY = 0.0f;

    D2D1_SIZE_U size;
    size.width = (UINT32)img->width;
    size.height = (UINT32)img->height;

    ID2D1Bitmap* bmp = NULL;
    HRESULT hr = ID2D1RenderTarget_CreateBitmap(rt, size, premul,
                                                (UINT32)img->width * 4u, &props, &bmp);
    free(premul);
    if (FAILED(hr)) return NULL;

    img->d2d_bitmap = (void*)bmp;
    img->d2d_owner = (void*)rt;
    return bmp;
}

// Copie d'image teintée (cache LRU circulaire) : DrawBitmap ne sait pas teinter
static ID2D1Bitmap* gtD2DGetTintedBitmap(ID2D1RenderTarget* rt, GtD2DPaint* paint, GtImage* img, GtColor tint) {
    if (!rt || !img || !paint) return NULL;
    if (tint == GT_WHITE || (tint & 0x00FFFFFFu) == 0x00FFFFFFu) {
        return gtD2DGetImageBitmap(rt, img);  // pas de teinte : bitmap brute
    }

    for (int i = 0; i < GT_D2D_MAX_TINTED; i++) {
        if (paint->tinted[i].bmp && paint->tinted[i].img == img && paint->tinted[i].tint == (uint32_t)tint) {
            if (img->d2d_owner != (void*)rt) break;  // bitmap d'un ancien RT : recréer
            return paint->tinted[i].bmp;
        }
    }

    int slot = paint->tinted_next % GT_D2D_MAX_TINTED;
    paint->tinted_next++;
    if (paint->tinted[slot].bmp) ID2D1Bitmap_Release(paint->tinted[slot].bmp);
    paint->tinted[slot].bmp = NULL;

    // Copie teintée calculée depuis les pixels sources (alpha droit) :
    // teinte multiplicative + prémultiplication en une passe
    int w = img->width, h = img->height;
    size_t n = (size_t)w * (size_t)h;
    uint32_t* data = (uint32_t*)malloc(n * sizeof(uint32_t));
    if (!data) return NULL;

    uint32_t tr = (tint >> 16) & 0xFFu, tg = (tint >> 8) & 0xFFu, tb = tint & 0xFFu;
    for (size_t i = 0; i < n; i++) {
        uint32_t p = img->pixels[i];
        uint32_t a = (p >> 24) & 0xFFu;
        if (a == 0u) { data[i] = 0; continue; }
        uint32_t r_ = (((p >> 16) & 0xFFu) * tr) / 255u;
        uint32_t g_ = (((p >> 8) & 0xFFu) * tg) / 255u;
        uint32_t b_ = ((p & 0xFFu) * tb) / 255u;
        if (a != 255u) {  // prémultiplie
            r_ = (r_ * a) / 255u;
            g_ = (g_ * a) / 255u;
            b_ = (b_ * a) / 255u;
        }
        data[i] = (a << 24) | (r_ << 16) | (g_ << 8) | b_;
    }

    D2D1_BITMAP_PROPERTIES props;
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.dpiX = 0.0f;
    props.dpiY = 0.0f;

    D2D1_SIZE_U size;
    size.width = (UINT32)w;
    size.height = (UINT32)h;

    ID2D1Bitmap* bmp = NULL;
    HRESULT hr = ID2D1RenderTarget_CreateBitmap(rt, size, data, (UINT32)w * 4u, &props, &bmp);
    free(data);
    if (FAILED(hr)) return NULL;

    paint->tinted[slot].img = img;
    paint->tinted[slot].tint = (uint32_t)tint;
    paint->tinted[slot].bmp = bmp;
    return bmp;
}

// Atlas de la police 8x8 : 95 glyphes de 8x8 px côte à côte (760x8),
// pixels = couleur du texte (prémultipliée) là où le bit est à 1.
// Cache mono-couleur : reconstruit au changement de couleur (~24 Ko d'upload).
static ID2D1Bitmap* gtD2DGetFontAtlas(ID2D1RenderTarget* rt, GtD2DPaint* paint, GtColor color) {
    if (!rt || !paint) return NULL;
    if (paint->atlas && paint->atlas_color == (uint32_t)color) return paint->atlas;

    const int AW = 95 * 8, AH = 8;
    uint32_t* data = (uint32_t*)malloc((size_t)AW * AH * sizeof(uint32_t));
    if (!data) return NULL;

    D2D1_COLOR_F c = gtD2DColor(color);  // alpha droit
    // Les données de bitmap doivent être prémultipliées
    uint32_t cr = (uint32_t)(c.r * c.a * 255.0f + 0.5f);
    uint32_t cg = (uint32_t)(c.g * c.a * 255.0f + 0.5f);
    uint32_t cb = (uint32_t)(c.b * c.a * 255.0f + 0.5f);
    uint32_t ca = (uint32_t)(c.a * 255.0f + 0.5f);
    uint32_t lit = (ca << 24) | (cr << 16) | (cg << 8) | cb;

    for (int ch = 0; ch < 95; ch++) {
        const uint8_t* glyph = &gt_font8x8[ch * 8];
        for (int row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                data[(size_t)row * AW + (size_t)ch * 8 + col] = (bits & (0x80 >> col)) ? lit : 0;
            }
        }
    }

    D2D1_BITMAP_PROPERTIES props;
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.dpiX = 0.0f;
    props.dpiY = 0.0f;

    D2D1_SIZE_U size;
    size.width = (UINT32)AW;
    size.height = (UINT32)AH;

    ID2D1Bitmap* bmp = NULL;
    HRESULT hr = ID2D1RenderTarget_CreateBitmap(rt, size, data, (UINT32)AW * 4u, &props, &bmp);
    free(data);
    if (FAILED(hr)) return NULL;

    if (paint->atlas) ID2D1Bitmap_Release(paint->atlas);
    paint->atlas = bmp;
    paint->atlas_color = (uint32_t)color;
    return bmp;
}

// Dessine un caractère de la police 8x8 via l'atlas (équivalent gtDrawChar)
static void gtD2DDrawGlyph(ID2D1RenderTarget* rt, GtD2DPaint* paint, ID2D1Bitmap* atlas,
                           float x, float y, char ch, float scale) {
    (void)paint;
    if (ch < 32 || ch > 126) return;
    D2D1_RECT_F src, dst;
    src.left = (float)((ch - 32) * 8); src.top = 0.0f;
    src.right = src.left + 8.0f;       src.bottom = 8.0f;
    dst.left = x;                      dst.top = y;
    dst.right = x + 8.0f * scale;      dst.bottom = y + 8.0f * scale;
    ID2D1RenderTarget_DrawBitmap(rt, atlas, &dst, 1.0f,
                                 D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src);
}

// Chaîne complète via atlas — réplique la boucle de gtDrawTextEx (wrap inclus)
static void gtD2DDrawTextRun(ID2D1RenderTarget* rt, GtD2DPaint* paint, ID2D1Bitmap* atlas,
                             const char* text, float x, float y, float scale,
                             int spacing, int wrap_width) {
    if (scale <= 0.0f) scale = 1.0f;
    float char_w = 8.0f * scale;
    float line_h = 8.0f * scale + (float)spacing;
    float start_x = x;
    float cur_x = x;
    float cur_y = y;

    for (const char* p = text; *p; p++) {
        char c = *p;

        if (c == '\n') {
            cur_x = start_x;
            cur_y += line_h;
            continue;
        }

        // Tabulation : avance de 4 cellules (comme le chemin GDI)
        if (c == '\t') {
            cur_x += 4.0f * char_w;
            continue;
        }

        if (wrap_width > 0 && c == ' ') {
            const char* next = p + 1;
            float word_w = 0.0f;
            while (*next && *next != ' ' && *next != '\n') {
                word_w += char_w + (float)spacing;
                next++;
            }
            if (cur_x + word_w > start_x + (float)wrap_width) {
                cur_x = start_x;
                cur_y += line_h;
            }
        }

        gtD2DDrawGlyph(rt, paint, atlas, cur_x, cur_y, c, scale);
        cur_x += char_w + (float)spacing;
    }
}

#endif // LIBGT_NO_D2D

// Crée un renderer pour une fenêtre
// type : GT_RENDERER_GDI ou GT_RENDERER_D2D
GtRenderer* gtCreateRenderer(GtWindow* window, GtRendererType type) {
    if (!window) {
        gtReportError("gtCreateRenderer : fenetre NULL");
        return NULL;
    }

    GtRenderer* renderer = (GtRenderer*)calloc(1, sizeof(GtRenderer));
    if (!renderer) {
        gtReportError("gtCreateRenderer : allocation OOM");
        return NULL;
    }

    renderer->window = window;
    renderer->type = GT_RENDERER_GDI;

#ifndef LIBGT_NO_D2D
    if (type == GT_RENDERER_D2D) {
        renderer->d2d_antialias = true;
        renderer->dwrite_font[0] = L'S';
        renderer->dwrite_font[1] = L'e';
        renderer->dwrite_font[2] = L'g';
        renderer->dwrite_font[3] = L'o';
        renderer->dwrite_font[4] = L'e';
        renderer->dwrite_font[5] = L' ';
        renderer->dwrite_font[6] = L'U';
        renderer->dwrite_font[7] = L'I';
        renderer->dwrite_font[8] = L'\0';
        renderer->d2d_last_hr = S_OK;

        D2D1_FACTORY_OPTIONS opts;
        opts.debugLevel = D2D1_DEBUG_LEVEL_NONE;
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                       &IID_ID2D1Factory, &opts, (void**)&renderer->d2d_factory);
        if (SUCCEEDED(hr)) {
            renderer->type = GT_RENDERER_D2D;
            window->renderer = renderer;
            // Le RT est créé paresseusement au premier dessin (gtD2DEnsureRT) :
            // la fenêtre peut ne pas être encore affichée à cet instant.
        }
        // Échec (DLL absente, OOM) : reste sur GDI ? Non — retour NULL pour
        // que l'appelant sache que le backend demandé est indisponible.
        if (renderer->type != GT_RENDERER_D2D) {
            gtReportError("gtCreateRenderer : Direct2D indisponible (hr=0x%08lX)",
                          (unsigned long)hr);
            free(renderer);
            return NULL;
        }
        return renderer;
    }
#else
    (void)type;  // LIBGT_NO_D2D : seul GDI existe
#endif

    window->renderer = renderer;
    return renderer;
}

// Détruit le renderer et libère ses ressources
void gtDestroyRenderer(GtRenderer* renderer) {
    if (!renderer) return;
#ifndef LIBGT_NO_D2D
    if (renderer->window && renderer->window->renderer == renderer) {
        renderer->window->renderer = NULL;
    }
    if (renderer->d2d_rt) {
        if (renderer->d2d_drawing) {
            // Frame en cours : clôture au mieux avant destruction
            while (renderer->d2d_clip_depth > 0) {
                ID2D1RenderTarget_PopAxisAlignedClip((ID2D1RenderTarget*)renderer->d2d_rt);
                renderer->d2d_clip_depth--;
            }
            ID2D1RenderTarget_EndDraw((ID2D1RenderTarget*)renderer->d2d_rt, NULL, NULL);
        }
        ID2D1HwndRenderTarget_Release(renderer->d2d_rt);
    }
    gtD2DInvalidatePaint(&renderer->d2d);
    for (int i = 0; i < renderer->dwrite_format_count; i++) {
        if (renderer->dwrite_formats[i]) IDWriteTextFormat_Release(renderer->dwrite_formats[i]);
    }
    if (renderer->dwrite_factory) IDWriteFactory_Release(renderer->dwrite_factory);
    if (renderer->d2d_factory) ID2D1Factory_Release(renderer->d2d_factory);
#endif
    free(renderer);
}

GtRendererType gtRendererGetType(const GtRenderer* renderer) {
    if (!renderer) return GT_RENDERER_GDI;
    return renderer->type;
}

void gtRendererSetAntialias(GtRenderer* renderer, bool antialias) {
    if (!renderer) return;
#ifndef LIBGT_NO_D2D
    renderer->d2d_antialias = antialias;
    if (renderer->d2d_rt) {
        ID2D1RenderTarget_SetAntialiasMode((ID2D1RenderTarget*)renderer->d2d_rt,
                                           antialias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                                     : D2D1_ANTIALIAS_MODE_ALIASED);
    }
#else
    (void)antialias;
#endif
}

// Début de frame : prépare le backend pour le dessin
// GDI : rien à faire (dessine direct dans framebuffer RAM)
// D2D : BeginDraw immédiat (idempotent — les dessins window-level peuvent
//       aussi démarrer la frame paresseusement sans Begin explicite)
void gtRendererBegin(GtRenderer* renderer) {
    if (!renderer) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        gtD2DBeginDraw(renderer);
        return;
    }
#endif
}

// Fin de frame : présente le résultat à l'écran
// GDI : appelle gtUpdateWindow (GetDC/StretchDIBits/ReleaseDC à chaque frame)
// D2D : EndDraw (présent via DWM) — idempotent (double End sans effet)
void gtRendererEnd(GtRenderer* renderer) {
    if (!renderer || !renderer->window) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        gtD2DEndFrame(renderer);
        return;
    }
#endif
    gtUpdateWindow(renderer->window);
}

// ---------------------------------------------------------------------------
// DISPATCHERS WINDOW-LEVEL D2D
// (prototypes anticipés après struct GtWindow)
// Retournent true si le dessin a été routé vers le GPU, false sinon (GDI).
// Z-order préservé : tout passe par le même render target dans l'ordre
// d'appel, la frame étant ouverte paresseusement au premier dessin.
// ---------------------------------------------------------------------------
#ifndef LIBGT_NO_D2D

static GtRenderer* gtWindowGetD2D(GtWindow* win) {
    if (!win || !win->renderer || win->renderer->type != GT_RENDERER_D2D) return NULL;
    return win->renderer;
}

// Synchro du clip D2D avec le sommet de la pile de scissor renderer.
// Le clip est en espace ÉCRAN : transform identité le temps du Push.
static void gtD2DSyncClip(GtRenderer* r) {
    if (!r->d2d_rt) return;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    if (r->d2d_clip_depth > 0) {
        ID2D1RenderTarget_PopAxisAlignedClip(rt);
        r->d2d_clip_depth--;
    }
    const GtRendererScissorState* s = (r->scissor_stack_depth > 0)
        ? &r->scissor_stack[r->scissor_stack_depth - 1] : NULL;
    if (s && s->active) {
        D2D1_RECT_F rect;
        rect.left = (float)s->x;
        rect.top = (float)s->y;
        rect.right = rect.left + (float)s->w;
        rect.bottom = rect.top + (float)s->h;
        D2D1_MATRIX_3X2_F saved;
        ID2D1RenderTarget_GetTransform(rt, &saved);
        D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
        ID2D1RenderTarget_SetTransform(rt, &id);
        ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
        ID2D1RenderTarget_SetTransform(rt, &saved);
        r->d2d_clip_depth++;
    }
}

static bool gtD2DWinClear(GtWindow* win, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_COLOR_F c = gtD2DColor(color);
    c.a = 1.0f;  // clear opaque : un alpha < 1 rend la fenêtre semi-transparente
                 // via DWM (compositing imprévisible) — on l'interdit ici.
    ID2D1RenderTarget_Clear(rt, &c);
    return true;
}

static bool gtD2DWinDrawPixel(GtWindow* win, int x, int y, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    gtD2DFillRect(rt, &r->d2d, (float)x, (float)y, 1.0f, 1.0f, color);
    return true;
}

static bool gtD2DWinDrawRect(GtWindow* win, int x, int y, int w, int h, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (w <= 0 || h <= 0) return true;  // consommé, rien à dessiner (comme GDI)
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    gtD2DFillRect(rt, &r->d2d, (float)x, (float)y, (float)w, (float)h, color);
    return true;
}

static bool gtD2DWinDrawRectLines(GtWindow* win, int x, int y, int w, int h, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (w <= 0 || h <= 0) return true;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    // Inset 0.5 : le contour couvre les pixels x..x+w-1 comme le GDI
    gtD2DFrameRect(rt, &r->d2d, (float)x + 0.5f, (float)y + 0.5f,
                   (float)w - 1.0f, (float)h - 1.0f, color, 1.0f);
    return true;
}

static bool gtD2DWinDrawLine(GtWindow* win, int x1, int y1, int x2, int y2, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    gtD2DDrawLineSeg(rt, &r->d2d, (float)x1, (float)y1, (float)x2, (float)y2, color, 1.0f);
    return true;
}

static bool gtD2DWinDrawCircle(GtWindow* win, int cx, int cy, int radius, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (radius < 0) return true;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    if (radius == 0) {
        gtD2DFillRect(rt, &r->d2d, (float)cx, (float)cy, 1.0f, 1.0f, color);
    } else {
        // +0.5 : même diamètre apparent que le disque GDI (2*r+1 px)
        gtD2DFillEllipse(rt, &r->d2d, (float)cx, (float)cy, (float)radius + 0.5f, color);
    }
    return true;
}

static bool gtD2DWinDrawCircleLines(GtWindow* win, int cx, int cy, int radius, uint32_t color) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    if (radius < 0) return true;
    if (!gtD2DBeginDraw(r)) return false;
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);
    if (radius == 0) {
        gtD2DFillRect(rt, &r->d2d, (float)cx, (float)cy, 1.0f, 1.0f, color);
    } else {
        gtD2DStrokeEllipse(rt, &r->d2d, (float)cx, (float)cy, (float)radius + 0.5f, color, 1.0f);
    }
    return true;
}

static void gtD2DWindowSize(GtWindow* win, int new_width, int new_height) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return;
    if (r->d2d_drawing) gtD2DEndFrame(r);  // Resize exige un EndDraw préalable
    if (!r->d2d_rt) return;                // sera créé à la bonne taille au prochain dessin
    D2D1_SIZE_U size;
    size.width = (UINT32)(new_width > 0 ? new_width : 1);
    size.height = (UINT32)(new_height > 0 ? new_height : 1);
    HRESULT hr = ID2D1HwndRenderTarget_Resize(r->d2d_rt, &size);
    if (FAILED(hr)) {
        gtD2DInvalidatePaint(&r->d2d);
        ID2D1HwndRenderTarget_Release(r->d2d_rt);
        r->d2d_rt = NULL;  // recréation paresseuse au prochain dessin
    }
}

static bool gtWindowD2DPresent(GtWindow* win) {
    GtRenderer* r = gtWindowGetD2D(win);
    if (!r) return false;
    gtD2DEndFrame(r);
    return true;
}

// Texte HQ (DirectWrite) ----------------------------------------------------

static IDWriteFactory* gtD2DEnsureDWrite(GtRenderer* r) {
    if (r->dwrite_factory) return r->dwrite_factory;
    IDWriteFactory* f = NULL;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory,
                                     (IUnknown**)&f);
    if (FAILED(hr)) return NULL;
    r->dwrite_factory = f;
    return f;
}

static IDWriteTextFormat* gtD2DGetTextFormat(GtRenderer* r, float font_px) {
    for (int i = 0; i < r->dwrite_format_count; i++) {
        if (r->dwrite_format_px[i] == font_px) return r->dwrite_formats[i];
    }
    if (r->dwrite_format_count >= GT_D2D_MAX_FONT_FORMATS) {
        // Cache plein : eviction round-robin (remplace le slot le plus ancien
        // par le nouveau format - l'ancien comportement reutilisait betement
        // le slot 0 et rendait la taille de police du premier slot).
        int slot = r->dwrite_evict_next % GT_D2D_MAX_FONT_FORMATS;
        r->dwrite_evict_next++;
        IDWriteFactory* f = gtD2DEnsureDWrite(r);
        if (!f) return NULL;
        IDWriteTextFormat* fmt = NULL;
        HRESULT hr = IDWriteFactory_CreateTextFormat(f, r->dwrite_font, NULL,
                                                     DWRITE_FONT_WEIGHT_NORMAL,
                                                     DWRITE_FONT_STYLE_NORMAL,
                                                     DWRITE_FONT_STRETCH_NORMAL,
                                                     font_px, L"en-US", &fmt);
        if (FAILED(hr)) return NULL;
        if (r->dwrite_formats[slot]) IDWriteTextFormat_Release(r->dwrite_formats[slot]);
        r->dwrite_formats[slot] = fmt;
        r->dwrite_format_px[slot] = font_px;
        return fmt;
    }
    int idx = r->dwrite_format_count;
        IDWriteFactory* f = gtD2DEnsureDWrite(r);
        if (!f) return NULL;
        IDWriteTextFormat* fmt = NULL;
        HRESULT hr = IDWriteFactory_CreateTextFormat(f, r->dwrite_font, NULL,
                                                     DWRITE_FONT_WEIGHT_NORMAL,
                                                     DWRITE_FONT_STYLE_NORMAL,
                                                     DWRITE_FONT_STRETCH_NORMAL,
                                                     font_px, L"en-US", &fmt);
        if (FAILED(hr)) return NULL;
        r->dwrite_formats[idx] = fmt;
        r->dwrite_format_px[idx] = font_px;
        r->dwrite_format_count++;
    return r->dwrite_formats[idx];
}

void gtRendererSetHqFontName(GtRenderer* renderer, const char* font_name) {
    if (!renderer || !font_name || !*font_name) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type != GT_RENDERER_D2D) return;
    int wn = MultiByteToWideChar(CP_UTF8, 0, font_name, -1, renderer->dwrite_font, 63);
    renderer->dwrite_font[(wn > 0) ? wn - 1 : 0] = 0;
    // Invalide les formats cachés (changement de police)
    for (int i = 0; i < renderer->dwrite_format_count; i++) {
        if (renderer->dwrite_formats[i]) IDWriteTextFormat_Release(renderer->dwrite_formats[i]);
        renderer->dwrite_formats[i] = NULL;
    }
    renderer->dwrite_format_count = 0;
#else
    (void)renderer; (void)font_name;
#endif
}

void gtRendererDrawTextHq(GtRenderer* renderer, float x, float y, const char* text,
                          float font_px, GtColor color) {
    if (!renderer || !renderer->window || !text || !*text) return;
    if (font_px <= 0.0f) font_px = 16.0f;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        IDWriteTextFormat* fmt = gtD2DGetTextFormat(renderer, font_px);
        if (!fmt) return;
        ID2D1SolidColorBrush* b = gtD2DGetBrush(rt, &renderer->d2d, color);
        if (!b) return;
        wchar_t wtext[512];
        int wn = MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 511);
        if (wn <= 0) return;  // wtext[wn-1] = L'\0' déjà posé (-1 inclut le null)
        // Texte HQ en espace écran (transform identité) — documenté.
        D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
        ID2D1RenderTarget_SetTransform(rt, &id);
        D2D1_RECT_F layout;
        layout.left = x;
        layout.top = y;
        layout.right = (float)renderer->window->width;
        layout.bottom = y + font_px * 1.7f;
        ID2D1RenderTarget_DrawText(rt, wtext, (UINT32)(wn - 1), fmt, &layout,
                                   (ID2D1Brush*)b,
                                   D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        return;
    }
#endif
    // Backend GDI : fallback police bitmap 8x8 à l'échelle font_px/8
    gtDrawTextEx(renderer->window, (int)x, (int)y, text, color, font_px / 8.0f, 0, 0);
}

#endif // LIBGT_NO_D2D

// WM_SIZE : synchronise le viewport de la caméra attachée au renderer de la
// fenêtre (valable pour les deux backends). La caméra reste possédée par
// l'appelant — la lib ne touche qu'aux champs viewport ; gtCameraUpdate
// recalcule la view matrix au prochain appel.
static void gtRendererSyncCameraViewport(GtWindow* win, int new_width, int new_height) {
    if (win && win->renderer && win->renderer->camera) {
        win->renderer->camera->viewport_w = new_width;
        win->renderer->camera->viewport_h = new_height;
    }
}

// Stubs quand le backend D2D est désactivé à la compilation
#ifdef LIBGT_NO_D2D
void gtRendererSetHqFontName(GtRenderer* renderer, const char* font_name) {
    (void)renderer; (void)font_name;
}
void gtRendererDrawTextHq(GtRenderer* renderer, float x, float y, const char* text,
                          float font_px, GtColor color) {
    if (!renderer || !renderer->window || !text || !*text) return;
    if (font_px <= 0.0f) font_px = 16.0f;
    gtDrawTextEx(renderer->window, (int)x, (int)y, text, color, font_px / 8.0f, 0, 0);
}
#endif


// Helpers internes au renderer.
static inline const GtMat3* gtRendererGetCurrentTransform(const GtRenderer* renderer);
static inline GtVec2 gtRendererApplyTransform(const GtRenderer* renderer, GtVec2 v);
static inline void gtRendererApplyTransformRect(const GtRenderer* renderer,
                                                float x, float y, float w, float h,
                                                float* out_x, float* out_y,
                                                float* out_w, float* out_h);
static inline bool gtRendererCheckScissor(const GtRenderer* renderer, int x, int y);
static inline bool gtRendererClipRectScissor(const GtRenderer* renderer,
                                             int* io_x, int* io_y,
                                             int* io_w, int* io_h);
static inline bool gtRendererClipLineScissor(const GtRenderer* renderer,
                                             int* io_x1, int* io_y1,
                                             int* io_x2, int* io_y2);

void gtRendererClear(GtRenderer* renderer, GtColor color) {
    if (!renderer || !renderer->window) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        D2D1_COLOR_F c = gtD2DColor(color);
        c.a = 1.0f;  // clear opaque (compositing DWM prévisible), cf. gtD2DWinClear
        ID2D1RenderTarget_Clear(rt, &c);
        return;
    }
#endif
    gtClearWindow(renderer->window, color);
}

void gtRendererDrawPixel(GtRenderer* renderer, int x, int y, GtColor color) {
    if (!renderer || !renderer->window) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        // Pixel local 1x1 sous la transform utilisateur (rotation gérée)
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        gtD2DFillRect(rt, &renderer->d2d, (float)x, (float)y, 1.0f, 1.0f, color);
        return;
    }
#endif

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)x, (float)y));
    int px = (int)p.x;
    int py = (int)p.y;

    if (gtRendererCheckScissor(renderer, px, py)) {
        gtDrawPixel(renderer->window, px, py, color);
    }
}

void gtRendererDrawRect(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window || w <= 0 || h <= 0) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        // Rect local sous la transform : rotation réelle (GDI dessine l'AABB)
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        gtD2DFillRect(rt, &renderer->d2d, (float)x, (float)y, (float)w, (float)h, color);
        return;
    }
#endif

    float tx = (float)x;
    float ty = (float)y;
    float tw = (float)w;
    float th = (float)h;
    gtRendererApplyTransformRect(renderer, tx, ty, tw, th, &tx, &ty, &tw, &th);

    int x1 = (int)tx;
    int y1 = (int)ty;
    int x2 = (int)(tx + tw);
    int y2 = (int)(ty + th);

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > renderer->window->width) x2 = renderer->window->width;
    if (y2 > renderer->window->height) y2 = renderer->window->height;

    int rect_w = x2 - x1;
    int rect_h = y2 - y1;
    if (rect_w <= 0 || rect_h <= 0) return;

    if (!gtRendererClipRectScissor(renderer, &x1, &y1, &rect_w, &rect_h)) return;

    x2 = x1 + rect_w;
    y2 = y1 + rect_h;

    for (int row = y1; row < y2; row++) {
        size_t idx = (size_t)row * (size_t)renderer->window->width + (size_t)x1;
        uint32_t* row_ptr = &renderer->window->buffer[idx];

        if (((color >> 24) & 0xFFu) == 255u) {
            uint32_t opaque = color | 0xFF000000u;
            for (int col = 0; col < rect_w; col++) {
                row_ptr[col] = opaque;
            }
        } else if (((color >> 24) & 0xFFu) != 0u) {
            for (int col = 0; col < rect_w; col++) {
                row_ptr[col] = gtBlendPixel(row_ptr[col], color);
            }
        }
    }
}

void gtRendererDrawRectLines(GtRenderer* renderer, int x, int y, int w, int h, GtColor color) {
    if (!renderer || !renderer->window || w <= 0 || h <= 0) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        // Inset 0.5 : contour 1px couvrant x..x+w-1 comme le GDI
        gtD2DFrameRect(rt, &renderer->d2d, (float)x + 0.5f, (float)y + 0.5f,
                       (float)w - 1.0f, (float)h - 1.0f, color, 1.0f);
        return;
    }
#endif

    float tx = (float)x;
    float ty = (float)y;
    float tw = (float)w;
    float th = (float)h;
    gtRendererApplyTransformRect(renderer, tx, ty, tw, th, &tx, &ty, &tw, &th);

    int x1 = (int)tx;
    int y1 = (int)ty;
    int x2 = (int)(tx + tw) - 1;
    int y2 = (int)(ty + th) - 1;

    if (gtRendererCheckScissor(renderer, x1, y1) ||
        gtRendererCheckScissor(renderer, x2, y1)) {
        drawHLineClipped(renderer->window, y1, x1, x2, color);
    }

    if (y2 != y1 &&
        (gtRendererCheckScissor(renderer, x1, y2) ||
         gtRendererCheckScissor(renderer, x2, y2))) {
        drawHLineClipped(renderer->window, y2, x1, x2, color);
    }

    if (gtRendererCheckScissor(renderer, x1, y1) ||
        gtRendererCheckScissor(renderer, x1, y2)) {
        drawVLineClipped(renderer->window, x1, y1 + 1, y2 - 1, color);
    }

    if (x2 != x1 &&
        (gtRendererCheckScissor(renderer, x2, y1) ||
         gtRendererCheckScissor(renderer, x2, y2))) {
        drawVLineClipped(renderer->window, x2, y1 + 1, y2 - 1, color);
    }
}

void gtRendererDrawLine(GtRenderer* renderer, int x1, int y1, int x2, int y2, GtColor color) {
    if (!renderer || !renderer->window) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        // Points transformés côté CPU, trait 1px écran (sémantique GDI)
        GtVec2 p1 = gtRendererApplyTransform(renderer, gtVec2((float)x1, (float)y1));
        GtVec2 p2 = gtRendererApplyTransform(renderer, gtVec2((float)x2, (float)y2));
        D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
        ID2D1RenderTarget_SetTransform(rt, &id);
        gtD2DDrawLineSeg(rt, &renderer->d2d, p1.x, p1.y, p2.x, p2.y, color, 1.0f);
        return;
    }
#endif

    GtVec2 p1 = gtRendererApplyTransform(renderer, gtVec2((float)x1, (float)y1));
    GtVec2 p2 = gtRendererApplyTransform(renderer, gtVec2((float)x2, (float)y2));

    int ix1 = (int)p1.x;
    int iy1 = (int)p1.y;
    int ix2 = (int)p2.x;
    int iy2 = (int)p2.y;

    if (!gtRendererClipLineScissor(renderer, &ix1, &iy1, &ix2, &iy2)) return;

    if (clipLineSegment(&ix1, &iy1, &ix2, &iy2,
                        renderer->window->width, renderer->window->height)) {
        rasterizeBresenhamUnchecked(renderer->window, ix1, iy1, ix2, iy2, color);
    }
}

void gtRendererDrawCircle(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window || radius < 0) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        // Cercle local sous la transform : rayon respecte le scale (amélioration vs GDI)
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        if (radius == 0) {
            gtD2DFillRect(rt, &renderer->d2d, (float)cx, (float)cy, 1.0f, 1.0f, color);
        } else {
            gtD2DFillEllipse(rt, &renderer->d2d, (float)cx, (float)cy, (float)radius + 0.5f, color);
        }
        return;
    }
#endif

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)cx, (float)cy));
    int tcx = (int)p.x;
    int tcy = (int)p.y;

    if (gtRendererCheckScissor(renderer, tcx, tcy)) {
        gtDrawCircle(renderer->window, tcx, tcy, radius, color);
    }
}

void gtRendererDrawCircleLines(GtRenderer* renderer, int cx, int cy, int radius, GtColor color) {
    if (!renderer || !renderer->window || radius < 0) return;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        if (radius == 0) {
            gtD2DFillRect(rt, &renderer->d2d, (float)cx, (float)cy, 1.0f, 1.0f, color);
        } else {
            gtD2DStrokeEllipse(rt, &renderer->d2d, (float)cx, (float)cy, (float)radius + 0.5f, color, 1.0f);
        }
        return;
    }
#endif

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)cx, (float)cy));
    int tcx = (int)p.x;
    int tcy = (int)p.y;

    if (gtRendererCheckScissor(renderer, tcx, tcy)) {
        gtDrawCircleLines(renderer->window, tcx, tcy, radius, color);
    }
}

/* =========================================================================
 * IMPLÉMENTATION : TRANSFORMATIONS 2D VIA RENDERER
 * ========================================================================= */

// Helper interne : obtient la matrice courante (identité si stack vide)
static inline const GtMat3* gtRendererGetCurrentTransform(const GtRenderer* renderer) {
    if (!renderer || renderer->transform_stack_depth == 0) return NULL;
    // Caméra live : tant que slot 0 est la copie caméra (non écrasée par
    // SetTransform), on retourne la view matrix DIRECTEMENT — déplacer la
    // caméra entre deux dessins suffit, plus besoin de re-SetCamera.
    if (renderer->camera_seeded && renderer->transform_stack_depth == 1 && renderer->camera) {
        gtCameraEnsureUpdated(renderer->camera);
        return &renderer->camera->view_matrix;
    }
    return &renderer->transform_stack[renderer->transform_stack_depth - 1];
}

// Helper interne : applique la transform courante à un point
static inline GtVec2 gtRendererApplyTransform(const GtRenderer* renderer, GtVec2 v) {
    const GtMat3* m = gtRendererGetCurrentTransform(renderer);
    if (!m) return v;
    return gtMat3MulVec2(*m, v);
}

// Helper interne : applique la transform courante à un rectangle et retourne son AABB
static inline void gtRendererApplyTransformRect(const GtRenderer* renderer,
                                                float x, float y, float w, float h,
                                                float* out_x, float* out_y,
                                                float* out_w, float* out_h) {
    const GtMat3* m = gtRendererGetCurrentTransform(renderer);
    if (!m) {
        *out_x = x;
        *out_y = y;
        *out_w = w;
        *out_h = h;
        return;
    }

    GtVec2 corners[4] = {
        gtMat3MulVec2(*m, gtVec2(x, y)),
        gtMat3MulVec2(*m, gtVec2(x + w, y)),
        gtMat3MulVec2(*m, gtVec2(x, y + h)),
        gtMat3MulVec2(*m, gtVec2(x + w, y + h))
    };

    float min_x = corners[0].x;
    float max_x = corners[0].x;
    float min_y = corners[0].y;
    float max_y = corners[0].y;

    for (int i = 1; i < 4; i++) {
        if (corners[i].x < min_x) min_x = corners[i].x;
        if (corners[i].x > max_x) max_x = corners[i].x;
        if (corners[i].y < min_y) min_y = corners[i].y;
        if (corners[i].y > max_y) max_y = corners[i].y;
    }

    *out_x = min_x;
    *out_y = min_y;
    *out_w = max_x - min_x;
    *out_h = max_y - min_y;
}

void gtRendererSetTransform(GtRenderer* renderer, const GtMat3* transform) {
    if (!renderer) return;
    renderer->camera_seeded = false;  // l'utilisateur écrase le slot caméra

    if (renderer->transform_stack_depth == 0) {
        if (!transform) return;
        renderer->transform_stack[0] = *transform;
        renderer->transform_stack_depth = 1;
        return;
    }

    if (transform) {
        renderer->transform_stack[renderer->transform_stack_depth - 1] = *transform;
    } else {
        renderer->transform_stack_depth = 0;
    }
}

const GtMat3* gtRendererGetTransform(const GtRenderer* renderer) {
    return gtRendererGetCurrentTransform(renderer);
}

void gtRendererPushTransform(GtRenderer* renderer) {
    if (!renderer) return;
    if (renderer->transform_stack_depth >= GT_RENDERER_MAX_TRANSFORM_STACK) return;

    if (renderer->transform_stack_depth == 0) {
        renderer->transform_stack[0] = gtMat3Identity();
        renderer->camera_seeded = false;  // identité, plus la caméra
    } else {
        renderer->transform_stack[renderer->transform_stack_depth] =
            renderer->transform_stack[renderer->transform_stack_depth - 1];
    }

    renderer->transform_stack_depth++;
}

void gtRendererPopTransform(GtRenderer* renderer) {
    if (!renderer || renderer->transform_stack_depth == 0) return;
    renderer->transform_stack_depth--;
}

void gtRendererMultiplyTransform(GtRenderer* renderer, const GtMat3* m) {
    if (!renderer || !m) return;

    if (renderer->transform_stack_depth == 0) {
        gtRendererPushTransform(renderer);
    }

    if (renderer->transform_stack_depth == 1) {
        renderer->camera_seeded = false;  // on modifie le slot caméra
    }

    GtMat3* current = &renderer->transform_stack[renderer->transform_stack_depth - 1];
    *current = gtMat3Mul(*current, *m);
}

void gtRendererTranslate(GtRenderer* renderer, float tx, float ty) {
    GtMat3 m = gtMat3Translate(tx, ty);
    gtRendererMultiplyTransform(renderer, &m);
}

void gtRendererRotate(GtRenderer* renderer, float angle_rad) {
    GtMat3 m = gtMat3Rotate(angle_rad);
    gtRendererMultiplyTransform(renderer, &m);
}

void gtRendererScale(GtRenderer* renderer, float sx, float sy) {
    GtMat3 m = gtMat3Scale(sx, sy);
    gtRendererMultiplyTransform(renderer, &m);
}

void gtRendererSetCamera(GtRenderer* renderer, const GtCamera* camera) {
    if (!renderer) return;

    renderer->camera = (GtCamera*)camera;

    if (camera) {
        // Matrices fraîches même si l'appelant a oublié gtCameraUpdate
        gtCameraEnsureUpdated(camera);
        if (renderer->transform_stack_depth == 0) {
            gtRendererPushTransform(renderer);
        }
        renderer->transform_stack[0] = camera->view_matrix;
        renderer->camera_seeded = true;
    } else {
        renderer->transform_stack_depth = 0;
        renderer->camera_seeded = false;
    }
}

const GtCamera* gtRendererGetCamera(const GtRenderer* renderer) {
    if (!renderer) return NULL;
    return renderer->camera;
}

GtVec2 gtRendererWorldToScreen(const GtRenderer* renderer, GtVec2 world) {
    if (!renderer) return world;

    if (renderer->camera) {
        return gtCameraWorldToScreen(renderer->camera, world);
    }

    return gtRendererApplyTransform(renderer, world);
}

GtVec2 gtRendererScreenToWorld(const GtRenderer* renderer, GtVec2 screen) {
    if (!renderer) return screen;

    if (renderer->camera) {
        return gtCameraScreenToWorld(renderer->camera, screen);
    }

    const GtMat3* m = gtRendererGetCurrentTransform(renderer);
    if (!m) return screen;

    return gtMat3MulVec2(gtMat3Inverse(*m), screen);
}

/* =========================================================================
 * IMPLÉMENTATION : SCISSOR / CLIPPING RECT
 * ========================================================================= */

void gtRendererSetScissorRect(GtRenderer* renderer, int x, int y, int w, int h) {
    if (!renderer) return;

    if (renderer->scissor_stack_depth == 0) {
        renderer->scissor_stack[0].x = x;
        renderer->scissor_stack[0].y = y;
        renderer->scissor_stack[0].w = w;
        renderer->scissor_stack[0].h = h;
        renderer->scissor_stack[0].active = (w > 0 && h > 0);
        renderer->scissor_stack_depth = 1;
    } else {
        GtRendererScissorState* scissor =
            &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

        scissor->x = x;
        scissor->y = y;
        scissor->w = w;
        scissor->h = h;
        scissor->active = (w > 0 && h > 0);
    }

#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) gtD2DSyncClip(renderer);
#endif
}

void gtRendererPushScissorRect(GtRenderer* renderer, int x, int y, int w, int h) {
    if (!renderer) return;
    if (renderer->scissor_stack_depth >= GT_RENDERER_MAX_SCISSOR_STACK) return;

    if (renderer->scissor_stack_depth > 0 &&
        renderer->scissor_stack[renderer->scissor_stack_depth - 1].active) {

        const GtRendererScissorState* parent =
            &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

        int nx = (x > parent->x) ? x : parent->x;
        int ny = (y > parent->y) ? y : parent->y;
        int nx2 = (x + w < parent->x + parent->w) ? x + w : parent->x + parent->w;
        int ny2 = (y + h < parent->y + parent->h) ? y + h : parent->y + parent->h;

        GtRendererScissorState* child =
            &renderer->scissor_stack[renderer->scissor_stack_depth];

        if (nx2 <= nx || ny2 <= ny) {
            child->x = 0;
            child->y = 0;
            child->w = 0;
            child->h = 0;
            child->active = false;
        } else {
            child->x = nx;
            child->y = ny;
            child->w = nx2 - nx;
            child->h = ny2 - ny;
            child->active = true;
        }
    } else {
        GtRendererScissorState* child =
            &renderer->scissor_stack[renderer->scissor_stack_depth];

        child->x = x;
        child->y = y;
        child->w = w;
        child->h = h;
        child->active = (w > 0 && h > 0);
    }

    renderer->scissor_stack_depth++;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) gtD2DSyncClip(renderer);
#endif
}

void gtRendererPopScissorRect(GtRenderer* renderer) {
    if (!renderer || renderer->scissor_stack_depth == 0) return;
    renderer->scissor_stack_depth--;
#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) gtD2DSyncClip(renderer);
#endif
}

void gtRendererGetScissorRect(const GtRenderer* renderer,
                              int* out_x, int* out_y,
                              int* out_w, int* out_h) {
    if (!renderer || renderer->scissor_stack_depth == 0 ||
        !renderer->scissor_stack[renderer->scissor_stack_depth - 1].active) {

        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        if (out_w) *out_w = renderer ? renderer->window->width : 0;
        if (out_h) *out_h = renderer ? renderer->window->height : 0;
        return;
    }

    const GtRendererScissorState* scissor =
        &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

    if (out_x) *out_x = scissor->x;
    if (out_y) *out_y = scissor->y;
    if (out_w) *out_w = scissor->w;
    if (out_h) *out_h = scissor->h;
}

static inline bool gtRendererCheckScissor(const GtRenderer* renderer, int x, int y) {
    if (!renderer || renderer->scissor_stack_depth == 0) return true;

    const GtRendererScissorState* scissor =
        &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

    if (!scissor->active) return true;

    return x >= scissor->x &&
           x < scissor->x + scissor->w &&
           y >= scissor->y &&
           y < scissor->y + scissor->h;
}

static inline bool gtRendererClipRectScissor(const GtRenderer* renderer,
                                             int* io_x, int* io_y,
                                             int* io_w, int* io_h) {
    if (!renderer || renderer->scissor_stack_depth == 0) return true;

    const GtRendererScissorState* scissor =
        &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

    if (!scissor->active) return true;

    int x1 = *io_x;
    int y1 = *io_y;
    int x2 = x1 + *io_w;
    int y2 = y1 + *io_h;

    if (x2 <= scissor->x ||
        x1 >= scissor->x + scissor->w ||
        y2 <= scissor->y ||
        y1 >= scissor->y + scissor->h) {
        return false;
    }

    int clipped_x1 = (x1 > scissor->x) ? x1 : scissor->x;
    int clipped_y1 = (y1 > scissor->y) ? y1 : scissor->y;
    int clipped_x2 = (x2 < scissor->x + scissor->w) ? x2 : scissor->x + scissor->w;
    int clipped_y2 = (y2 < scissor->y + scissor->h) ? y2 : scissor->y + scissor->h;

    *io_x = clipped_x1;
    *io_y = clipped_y1;
    *io_w = clipped_x2 - clipped_x1;
    *io_h = clipped_y2 - clipped_y1;

    return *io_w > 0 && *io_h > 0;
}

static inline bool gtRendererClipLineScissor(const GtRenderer* renderer,
                                             int* io_x1, int* io_y1,
                                             int* io_x2, int* io_y2) {
    if (!renderer || renderer->scissor_stack_depth == 0) return true;

    const GtRendererScissorState* scissor =
        &renderer->scissor_stack[renderer->scissor_stack_depth - 1];

    if (!scissor->active) return true;

    int x1 = *io_x1 - scissor->x;
    int y1 = *io_y1 - scissor->y;
    int x2 = *io_x2 - scissor->x;
    int y2 = *io_y2 - scissor->y;

    if (!clipLineSegment(&x1, &y1, &x2, &y2, scissor->w, scissor->h)) {
        return false;
    }

    *io_x1 = x1 + scissor->x;
    *io_y1 = y1 + scissor->y;
    *io_x2 = x2 + scissor->x;
    *io_y2 = y2 + scissor->y;
    return true;
}

void gtRendererDrawText(GtRenderer* renderer, int x, int y,
                        const char* text, GtColor color) {
    if (!renderer || !renderer->window || !text) return;

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)x, (float)y));
    gtDrawText(renderer->window, (int)p.x, (int)p.y, text, color);
}

void gtRendererDrawTextEx(GtRenderer* renderer, int x, int y,
                          const char* text, GtColor color,
                          float scale, int spacing, int wrap_width) {
    if (!renderer || !renderer->window || !text) return;

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)x, (float)y));
    gtDrawTextEx(renderer->window, (int)p.x, (int)p.y, text, color,
                 scale, spacing, wrap_width);
}

// Texte formaté style printf via renderer : transforme le point d'origine
// (comme gtRendererDrawText) puis délègue. Buffer interne 512 octets.
void gtRendererDrawTextFmt(GtRenderer* renderer, int x, int y, GtColor color, const char* fmt, ...) {
    if (!renderer || !renderer->window || !fmt) return;

    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    GtVec2 p = gtRendererApplyTransform(renderer, gtVec2((float)x, (float)y));
    gtDrawTextEx(renderer->window, (int)p.x, (int)p.y, buffer, color, 1.0f, 0, 0);
}

void gtRendererDrawImage(GtRenderer* renderer, GtImage* image,
                         int x, int y, GtColor tint) {
    if (!renderer || !renderer->window || !image) return;

    int img_w = gtImageGetWidth(image);
    int img_h = gtImageGetHeight(image);
    uint32_t* img_pixels = gtImageGetPixels(image);
    if (!img_pixels || img_w <= 0 || img_h <= 0) return;

#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if ((tint >> 24) == 0) return;  // invisible (comme le chemin GDI)
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        ID2D1Bitmap* bmp = gtD2DGetTintedBitmap(rt, &renderer->d2d, image, tint);
        if (!bmp) return;
        // Rect local sous la transform utilisateur (rotation réelle, vs AABB GDI)
        D2D1_MATRIX_3X2_F m = gtD2DMatrix(gtRendererGetCurrentTransform(renderer));
        ID2D1RenderTarget_SetTransform(rt, &m);
        D2D1_RECT_F dst;
        dst.left = (float)x; dst.top = (float)y;
        dst.right = dst.left + (float)img_w; dst.bottom = dst.top + (float)img_h;
        ID2D1RenderTarget_DrawBitmap(rt, bmp, &dst, 1.0f,
                                     D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
        return;
    }
#endif

    float tx = (float)x;
    float ty = (float)y;
    float tw = (float)img_w;
    float th = (float)img_h;
    gtRendererApplyTransformRect(renderer, tx, ty, tw, th, &tx, &ty, &tw, &th);

    GtWindow* win = renderer->window;

    int x1 = (int)tx;
    int y1 = (int)ty;
    int x2 = (int)(tx + tw);
    int y2 = (int)(ty + th);

    int clipped_w = x2 - x1;
    int clipped_h = y2 - y1;
    if (clipped_w <= 0 || clipped_h <= 0) return;

    if (!gtRendererClipRectScissor(renderer,
                                   &x1, &y1, &clipped_w, &clipped_h)) {
        return;
    }

    int screen_x1 = x1;
    int screen_y1 = y1;
    int screen_x2 = x1 + clipped_w;
    int screen_y2 = y1 + clipped_h;

    if (screen_x1 < 0) {
        int delta = -screen_x1;
        screen_x1 = 0;
        clipped_w -= delta;
    }
    if (screen_y1 < 0) {
        int delta = -screen_y1;
        screen_y1 = 0;
        clipped_h -= delta;
    }
    if (screen_x2 > win->width) {
        clipped_w -= screen_x2 - win->width;
        screen_x2 = win->width;
    }
    if (screen_y2 > win->height) {
        clipped_h -= screen_y2 - win->height;
        screen_y2 = win->height;
    }

    if (clipped_w <= 0 || clipped_h <= 0) return;

    int src_x1 = screen_x1 - (int)tx;
    int src_y1 = screen_y1 - (int)ty;

    if (src_x1 < 0) src_x1 = 0;
    if (src_y1 < 0) src_y1 = 0;
    if (src_x1 >= img_w || src_y1 >= img_h) return;

    uint8_t tint_a = (uint8_t)((tint >> 24) & 0xFFu);
    if (tint_a == 0) return;

    for (int row = 0; row < clipped_h; row++) {
        int dst_y = screen_y1 + row;
        int src_y = src_y1 + row;
        if (src_y < 0 || src_y >= img_h) continue;

        uint32_t* dst_row =
            &win->buffer[(size_t)dst_y * (size_t)win->width + (size_t)screen_x1];
        uint32_t* src_row =
            &img_pixels[(size_t)src_y * (size_t)img_w + (size_t)src_x1];

        for (int col = 0; col < clipped_w; col++) {
            uint32_t src = src_row[col];
            uint8_t sa = (uint8_t)((src >> 24) & 0xFFu);
            if (sa == 0) continue;

            if (tint_a != 255) {
                uint32_t sr = ((src >> 16) & 0xFFu) * ((tint >> 16) & 0xFFu) / 255u;
                uint32_t sg = ((src >> 8) & 0xFFu) * ((tint >> 8) & 0xFFu) / 255u;
                uint32_t sb = (src & 0xFFu) * (tint & 0xFFu) / 255u;
                src = ((uint32_t)sa << 24) | (sr << 16) | (sg << 8) | sb;
            }

            dst_row[col] = (sa == 255 && tint_a == 255)
                ? (src | 0xFF000000u)
                : gtBlendPixel(dst_row[col], src);
        }
    }
}

// Blit software d'un sous-rectangle UV (GDI) — défini plus bas, partagé par
// gtRendererDrawImageEx (image complète : UV 0..1) et gtRendererDrawImageUV.
static void gtBlitImageUV(GtRenderer* renderer, GtImage* image,
                          int x, int y, int w, int h,
                          float u0, float v0, float u1, float v1,
                          bool flip_x, bool flip_y, GtColor tint);

void gtRendererDrawImageEx(GtRenderer* renderer, GtImage* image,
                           int x, int y, int w, int h,
                           float rot, GtVec2 origin,
                           bool flip_x, bool flip_y, GtColor tint) {
#ifndef LIBGT_NO_D2D
    if (renderer && renderer->type == GT_RENDERER_D2D) {
        if (!image || w <= 0 || h <= 0 || (tint >> 24) == 0) return;
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        ID2D1Bitmap* bmp = gtD2DGetTintedBitmap(rt, &renderer->d2d, image, tint);
        if (!bmp) return;

        // Rect destination en espace LOCAL : (0,0)-(w,h). La matrice porte
        // TOUTE la translation vers l'ecran (dx,dy) - dessiner le rect en
        // (dx,dy) sous une matrice contenant deja T(dx,dy) doublait la
        // position. L'ancre (x,y) correspond au point local
        // (origin.x*w, origin.y*h) : c'est le pivot de rotation.
        float dx = (float)x - origin.x * (float)w;
        float dy = (float)y - origin.y * (float)h;

        // M = user . T(dx,dy) . T(ancre) . R(rot) . T(-ancre) . T(centre) . S(flip) . T(-centre)
        // (ordre col-major : le flip s'applique d'abord, puis la rotation)
        GtMat3 m = gtMat3Translate(dx, dy);
        if (flip_x || flip_y) {
            float fcx = (float)w * 0.5f;
            float fcy = (float)h * 0.5f;
            m = gtMat3Mul(m, gtMat3Translate(fcx, fcy));
            m = gtMat3Mul(m, gtMat3Scale(flip_x ? -1.0f : 1.0f, flip_y ? -1.0f : 1.0f));
            m = gtMat3Mul(m, gtMat3Translate(-fcx, -fcy));
        }
        if (rot != 0.0f) {
            float ax = origin.x * (float)w;
            float ay = origin.y * (float)h;
            m = gtMat3Mul(m, gtMat3Translate(ax, ay));
            m = gtMat3Mul(m, gtMat3Rotate(rot));
            m = gtMat3Mul(m, gtMat3Translate(-ax, -ay));
        }
        const GtMat3* user = gtRendererGetCurrentTransform(renderer);
        GtMat3 total = user ? gtMat3Mul(*user, m) : m;

        D2D1_MATRIX_3X2_F dm = gtD2DMatrix(&total);
        ID2D1RenderTarget_SetTransform(rt, &dm);
        D2D1_RECT_F dst;
        dst.left = 0.0f; dst.top = 0.0f;
        dst.right = (float)w; dst.bottom = (float)h;
        ID2D1RenderTarget_DrawBitmap(rt, bmp, &dst, 1.0f,
                                     D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
        return;
    }
#endif
    /* GDI renderer actuel : rotation/origine ne sont pas encore rasterisés.
     * flip_x/flip_y sont supportés (miroir du sampling source). */
    (void)rot;
    (void)origin;

    gtBlitImageUV(renderer, image, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f,
                  flip_x, flip_y, tint);
}

// Blit software d'un sous-rectangle UV de l'image (GDI). Utilisé par
// gtRendererDrawImageEx (image complète : UV 0..1) et gtRendererDrawImageUV.
static void gtBlitImageUV(GtRenderer* renderer, GtImage* image,
                          int x, int y, int w, int h,
                          float u0, float v0, float u1, float v1,
                          bool flip_x, bool flip_y, GtColor tint) {
    if (!renderer || !renderer->window || !image || w <= 0 || h <= 0) return;
    if (u1 <= u0 || v1 <= v0) return;

    int img_w = gtImageGetWidth(image);
    int img_h = gtImageGetHeight(image);
    uint32_t* img_pixels = gtImageGetPixels(image);
    if (!img_pixels || img_w <= 0 || img_h <= 0) return;

    // Bornes source en pixels (arrondi : tolère les fractions non exactes
    // type 0.9999999 pour v1=1.0 d'un spritesheet)
    int sx0 = (int)(u0 * (float)img_w + 0.5f);
    int sx1 = (int)(u1 * (float)img_w + 0.5f);
    int sy0 = (int)(v0 * (float)img_h + 0.5f);
    int sy1 = (int)(v1 * (float)img_h + 0.5f);
    if (sx0 < 0) sx0 = 0;
    if (sx0 > img_w) sx0 = img_w;
    if (sx1 < sx0) sx1 = sx0;
    if (sx1 > img_w) sx1 = img_w;
    if (sy0 < 0) sy0 = 0;
    if (sy0 > img_h) sy0 = img_h;
    if (sy1 < sy0) sy1 = sy0;
    if (sy1 > img_h) sy1 = img_h;
    int sub_w = sx1 - sx0;
    int sub_h = sy1 - sy0;
    if (sub_w <= 0 || sub_h <= 0) return;

    float tx = (float)x;
    float ty = (float)y;
    float tw = (float)w;
    float th = (float)h;
    gtRendererApplyTransformRect(renderer, tx, ty, tw, th, &tx, &ty, &tw, &th);

    if (tw <= 0.0f || th <= 0.0f) return;

    GtWindow* win = renderer->window;

    int x1 = (int)tx;
    int y1 = (int)ty;
    int clipped_w = (int)tw;
    int clipped_h = (int)th;

    if (clipped_w <= 0 || clipped_h <= 0) return;

    if (!gtRendererClipRectScissor(renderer, &x1, &y1, &clipped_w, &clipped_h)) {
        return;
    }

    int screen_x1 = x1;
    int screen_y1 = y1;

    if (screen_x1 < 0) {
        int delta = -screen_x1;
        screen_x1 = 0;
        clipped_w -= delta;
    }
    if (screen_y1 < 0) {
        int delta = -screen_y1;
        screen_y1 = 0;
        clipped_h -= delta;
    }
    if (screen_x1 + clipped_w > win->width) {
        clipped_w = win->width - screen_x1;
    }
    if (screen_y1 + clipped_h > win->height) {
        clipped_h = win->height - screen_y1;
    }

    if (clipped_w <= 0 || clipped_h <= 0) return;

    uint8_t tint_a = (uint8_t)((tint >> 24) & 0xFFu);
    if (tint_a == 0) return;

    for (int dy = 0; dy < clipped_h; dy++) {
        int dst_y = screen_y1 + dy;
        float src_frac_y = (float)(dy + (screen_y1 - (int)ty)) / th;
        int src_y = sy0 + (int)(src_frac_y * (float)sub_h);
        if (src_y < sy0) src_y = sy0;
        if (src_y >= sy1) src_y = sy1 - 1;
        if (flip_y) src_y = sy1 - 1 - (src_y - sy0); // miroir vertical dans le sous-rect

        uint32_t* dst_row =
            &win->buffer[(size_t)dst_y * (size_t)win->width];

        for (int dx = 0; dx < clipped_w; dx++) {
            int dst_x = screen_x1 + dx;
            float src_frac_x = (float)(dx + (screen_x1 - (int)tx)) / tw;
            int src_x = sx0 + (int)(src_frac_x * (float)sub_w);
            if (src_x < sx0) src_x = sx0;
            if (src_x >= sx1) src_x = sx1 - 1;
            if (flip_x) src_x = sx1 - 1 - (src_x - sx0); // miroir horizontal dans le sous-rect

            uint32_t src =
                img_pixels[(size_t)src_y * (size_t)img_w + (size_t)src_x];
            uint8_t sa = (uint8_t)((src >> 24) & 0xFFu);
            if (sa == 0) continue;

            if (tint_a != 255) {
                uint32_t sr = ((src >> 16) & 0xFFu) * ((tint >> 16) & 0xFFu) / 255u;
                uint32_t sg = ((src >> 8) & 0xFFu) * ((tint >> 8) & 0xFFu) / 255u;
                uint32_t sb = (src & 0xFFu) * (tint & 0xFFu) / 255u;
                src = ((uint32_t)sa << 24) | (sr << 16) | (sg << 8) | sb;
            }

            dst_row[(size_t)dst_x] = gtBlendPixel(dst_row[(size_t)dst_x], src);
        }
    }
}

// Dessine un sous-rectangle de l'image (spritesheets). Coordonnées UV
// normalisées [0..1] : frame i d'une grille de N colonnes = u0 = i/N,
// u1 = (i+1)/N. Taille écran (w,h) libre (nearest-neighbor). Flips miroirent
// le sous-rect. D2D : sourceRect natif. Pour la rotation, utiliser
// gtRendererDrawImageTransformed.
void gtRendererDrawImageUV(GtRenderer* renderer, GtImage* image,
                           int x, int y, int w, int h,
                           float u0, float v0, float u1, float v1,
                           bool flip_x, bool flip_y, GtColor tint) {
    if (!renderer || !renderer->window || !image || w <= 0 || h <= 0) return;
    if ((tint >> 24) == 0) return;
    if (u1 <= u0 || v1 <= v0) return;

#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        int img_w = gtImageGetWidth(image);
        int img_h = gtImageGetHeight(image);
        if (img_w <= 0 || img_h <= 0) return;
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        ID2D1Bitmap* bmp = gtD2DGetTintedBitmap(rt, &renderer->d2d, image, tint);
        if (!bmp) return;

        // Transform utilisateur + miroir du sous-rect autour de son centre
        // (même composition que gtRendererDrawImageEx, sans rot/origin)
        const GtMat3* user = gtRendererGetCurrentTransform(renderer);
        GtMat3 m = user ? *user : gtMat3Identity();
        if (flip_x || flip_y) {
            float fcx = (float)x + (float)w * 0.5f;
            float fcy = (float)y + (float)h * 0.5f;
            m = gtMat3Mul(m, gtMat3Translate(fcx, fcy));
            m = gtMat3Mul(m, gtMat3Scale(flip_x ? -1.0f : 1.0f, flip_y ? -1.0f : 1.0f));
            m = gtMat3Mul(m, gtMat3Translate(-fcx, -fcy));
        }
        D2D1_MATRIX_3X2_F dm = gtD2DMatrix(&m);
        ID2D1RenderTarget_SetTransform(rt, &dm);

        D2D1_RECT_F dst;
        dst.left = (float)x; dst.top = (float)y;
        dst.right = dst.left + (float)w; dst.bottom = dst.top + (float)h;
        D2D1_RECT_F src;
        src.left = u0 * (float)img_w;          src.top = v0 * (float)img_h;
        src.right = u1 * (float)img_w;         src.bottom = v1 * (float)img_h;
        ID2D1RenderTarget_DrawBitmap(rt, bmp, &dst, 1.0f,
                                     D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src);
        return;
    }
#endif
    gtBlitImageUV(renderer, image, x, y, w, h, u0, v0, u1, v1, flip_x, flip_y, tint);
}

void gtRendererDrawImageTransformed(GtRenderer* renderer,
                                    GtImage* image,
                                    const GtMat3* model,
                                    GtColor tint) {
    if (!renderer || !renderer->window || !image || !model) return;

    int img_w = gtImageGetWidth(image);
    int img_h = gtImageGetHeight(image);
    if (img_w <= 0 || img_h <= 0) return;

#ifndef LIBGT_NO_D2D
    if (renderer->type == GT_RENDERER_D2D) {
        if ((tint >> 24) == 0) return;
        if (!gtD2DBeginDraw(renderer)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)renderer->d2d_rt;
        ID2D1Bitmap* bmp = gtD2DGetTintedBitmap(rt, &renderer->d2d, image, tint);
        if (!bmp) return;
        // Vrai quad transformé (vs AABB à plat en GDI)
        const GtMat3* user = gtRendererGetCurrentTransform(renderer);
        GtMat3 total = user ? gtMat3Mul(*user, *model) : *model;
        D2D1_MATRIX_3X2_F dm = gtD2DMatrix(&total);
        ID2D1RenderTarget_SetTransform(rt, &dm);
        D2D1_RECT_F dst;
        dst.left = 0.0f; dst.top = 0.0f;
        dst.right = (float)img_w; dst.bottom = (float)img_h;
        ID2D1RenderTarget_DrawBitmap(rt, bmp, &dst, 1.0f,
                                     D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
        return;
    }
#endif

    GtVec2 corners[4] = {
        gtMat3MulVec2(*model, gtVec2(0.0f, 0.0f)),
        gtMat3MulVec2(*model, gtVec2((float)img_w, 0.0f)),
        gtMat3MulVec2(*model, gtVec2(0.0f, (float)img_h)),
        gtMat3MulVec2(*model, gtVec2((float)img_w, (float)img_h))
    };

    float min_x = corners[0].x;
    float max_x = corners[0].x;
    float min_y = corners[0].y;
    float max_y = corners[0].y;

    for (int i = 1; i < 4; i++) {
        if (corners[i].x < min_x) min_x = corners[i].x;
        if (corners[i].x > max_x) max_x = corners[i].x;
        if (corners[i].y < min_y) min_y = corners[i].y;
        if (corners[i].y > max_y) max_y = corners[i].y;
    }

    gtRendererDrawImageEx(renderer, image,
                          (int)min_x, (int)min_y,
                          (int)(max_x - min_x), (int)(max_y - min_y),
                          0.0f, gtVec2(0.0f, 0.0f),
                          false, false, tint);
}

/* =========================================================================
* IMPLÉMENTATION : BATCH RENDERING (2.1)
* =========================================================================
* Architecture : ce "batch renderer" est un command buffer CPU, rejoué au
* flush par le backend actif. GtBatchVertex (champs x/y, u/v, color, radius,
* image_id, prim_type, glyph) est le contrat consommé par les deux backends :
*   - GDI  : rastérisation CPU directe (gtBatchFlush).
*   - D2D  : rejeu via le render target GPU (gtBatchFlushD2D) — ellipses
*     natives, FillRectangle, DrawBitmap, atlas texte.
* Le coût D2D reste ~350ns/primitive (API immédiate) : pour les charges
* type particules au-delà de ~10k primitives/frame, un futur backend
* D3D11 raw-quads (vertex buffer unique) supprimerait ce coût — le buffer
* est déjà en forme pour ça (1 vertex/cercle, quads pré-transformés).
* Reste à évaluer : tri par image_id/couleur pour minimiser les changements
* d'état GPU.
* ========================================================================= */

#define GT_BATCH_MAX_TRANSFORM_STACK 32
#define GT_BATCH_MAX_SCISSOR_STACK 32

typedef struct GtBatchScissorState {
    int x, y, w, h;
    bool active;
} GtBatchScissorState;

struct GtBatchRenderer {
    GtRenderer* renderer;           // Renderer parent
    GtBatchConfig config;           // Configuration
    
    // Vertex buffer
    GtBatchVertex* vertices;        // Buffer de vertices
    int vertex_count;               // Nombre de vertices utilisés
    int vertex_capacity;            // Capacité du buffer
    
    // Images référencées dans ce batch
    GtImage** images;               // Tableau d'images (max_images)
    int image_count;                // Nombre d'images
    
    // Transform stack
    GtMat3 transform_stack[GT_BATCH_MAX_TRANSFORM_STACK];
    int transform_stack_depth;
    
    // Scissor stack
    GtBatchScissorState scissor_stack[GT_BATCH_MAX_SCISSOR_STACK];
    int scissor_stack_depth;
    
    // Stats
    int flush_count;
    int draw_calls;
    
    // État
    bool began;                     // gtBatchBegin appelé
};

static inline const GtMat3* gtBatchGetCurrentTransform(const GtBatchRenderer* batch) {
    if (!batch || batch->transform_stack_depth == 0) return NULL;
    return &batch->transform_stack[batch->transform_stack_depth - 1];
}

static inline GtVec2 gtBatchApplyTransform(const GtBatchRenderer* batch, GtVec2 v) {
    const GtMat3* m = gtBatchGetCurrentTransform(batch);
    if (!m) return v;
    return gtMat3MulVec2(*m, v);
}

static inline void gtBatchApplyTransformRect(const GtBatchRenderer* batch, float x, float y, float w, float h, 
                                             float* out_x, float* out_y, float* out_w, float* out_h) {
    const GtMat3* m = gtBatchGetCurrentTransform(batch);
    if (!m) {
        *out_x = x; *out_y = y; *out_w = w; *out_h = h;
        return;
    }
    GtVec2 corners[4] = {
        gtMat3MulVec2(*m, gtVec2(x, y)),
        gtMat3MulVec2(*m, gtVec2(x + w, y)),
        gtMat3MulVec2(*m, gtVec2(x, y + h)),
        gtMat3MulVec2(*m, gtVec2(x + w, y + h))
    };
    float min_x = corners[0].x, max_x = corners[0].x;
    float min_y = corners[0].y, max_y = corners[0].y;
    for (int i = 1; i < 4; i++) {
        if (corners[i].x < min_x) min_x = corners[i].x;
        if (corners[i].x > max_x) max_x = corners[i].x;
        if (corners[i].y < min_y) min_y = corners[i].y;
        if (corners[i].y > max_y) max_y = corners[i].y;
    }
    *out_x = min_x; *out_y = min_y; *out_w = max_x - min_x; *out_h = max_y - min_y;
}

static inline int gtBatchGetImageId(GtBatchRenderer* batch, GtImage* image) {
    if (!image) return -1;
    for (int i = 0; i < batch->image_count; i++) {
        if (batch->images[i] == image) return i;
    }
    if (batch->image_count < batch->config.max_images) {
        batch->images[batch->image_count++] = image;
        return batch->image_count - 1;
    }
    return -1; // Plus de place pour images
}

static inline bool gtBatchCheckScissor(const GtBatchRenderer* batch, int x, int y) {
    if (!batch || batch->scissor_stack_depth == 0) return true;
    const GtBatchScissorState* s = &batch->scissor_stack[batch->scissor_stack_depth - 1];
    if (!s->active) return true;
    return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

static inline bool gtBatchClipRectScissor(const GtBatchRenderer* batch, int* io_x, int* io_y, int* io_w, int* io_h) {
    if (!batch || batch->scissor_stack_depth == 0) return true;
    const GtBatchScissorState* s = &batch->scissor_stack[batch->scissor_stack_depth - 1];
    if (!s->active) return true;
    
    int x1 = *io_x;
    int y1 = *io_y;
    int x2 = *io_x + *io_w;
    int y2 = *io_y + *io_h;
    
    if (x2 <= s->x || x1 >= s->x + s->w || y2 <= s->y || y1 >= s->y + s->h) return false;
    
    if (x1 < s->x) { *io_w -= s->x - x1; x1 = s->x; }
    if (y1 < s->y) { *io_h -= s->y - y1; y1 = s->y; }
    if (x2 > s->x + s->w) { *io_w = s->x + s->w - x1; }
    if (y2 > s->y + s->h) { *io_h = s->y + s->h - y1; }
    
    *io_x = x1; *io_y = y1;
    return *io_w > 0 && *io_h > 0;
}

static inline void gtBatchEnsureCapacity(GtBatchRenderer* batch, int needed) {
    if (batch->vertex_count + needed > batch->vertex_capacity) {
        if (batch->config.auto_flush) {
            gtBatchFlush(batch);
        } else {
            // Grow buffer
            int new_cap = batch->vertex_capacity * 2;
            if (new_cap < batch->vertex_count + needed) new_cap = batch->vertex_count + needed;
            GtBatchVertex* new_vertices = (GtBatchVertex*)realloc(batch->vertices, (size_t)new_cap * sizeof(GtBatchVertex));
            if (new_vertices) {
                batch->vertices = new_vertices;
                batch->vertex_capacity = new_cap;
            }
        }
    }
}

static void gtBatchAddVertex(GtBatchRenderer* batch, GtBatchVertex v) {
    if (batch->vertex_count < batch->vertex_capacity) {
        batch->vertices[batch->vertex_count++] = v;
    }
}

// Rasterise un rectangle en vertices (2 triangles = 6 vertices)
static void gtBatchEmitRect(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color, int image_id, float u0, float v0, float u1, float v1) {
    GtBatchVertex v[6];
    // Triangle 1: (x,y) (x+w,y) (x,y+h)
    v[0] = (GtBatchVertex){ x, y, u0, v0, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    v[1] = (GtBatchVertex){ x + w, y, u1, v0, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    v[2] = (GtBatchVertex){ x, y + h, u0, v1, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    // Triangle 2: (x+w,y) (x+w,y+h) (x,y+h)
    v[3] = (GtBatchVertex){ x + w, y, u1, v0, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    v[4] = (GtBatchVertex){ x + w, y + h, u1, v1, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    v[5] = (GtBatchVertex){ x, y + h, u0, v1, color, 0, image_id, GT_BATCH_RECT, 0, {0} };
    
    gtBatchEnsureCapacity(batch, 6);
    for (int i = 0; i < 6; i++) gtBatchAddVertex(batch, v[i]);
}

// Émet un cercle : un seul vertex (centre) portant le rayon.
// Le flush GDI n'utilise que centre/rayon/couleur, et un backend GPU
// (Direct2D) dessinerait une ellipse native : pas de pré-tessellation
// en triangle fan, qui de plus rendait ambigüe la frontière entre deux
// cercles consécutifs au flush.
static void gtBatchEmitCircle(GtBatchRenderer* batch, float cx, float cy, float radius, GtColor color) {
    GtBatchVertex v = { cx, cy, 0, 0, color, radius, -1, GT_BATCH_CIRCLE, 0, {0} };
    gtBatchEnsureCapacity(batch, 1);
    gtBatchAddVertex(batch, v);
}

// Émet le quad d'un caractère de texte (bitmap font 8x8) : 6 vertices
// tagués GT_BATCH_TEXT ; le flush échantillonne gt_font8x8 via `glyph`.
static void gtBatchEmitGlyph(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color, char ch, float scale) {
    GtBatchVertex v[6];
    v[0] = (GtBatchVertex){ x, y, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    v[1] = (GtBatchVertex){ x + w, y, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    v[2] = (GtBatchVertex){ x, y + h, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    v[3] = (GtBatchVertex){ x + w, y, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    v[4] = (GtBatchVertex){ x + w, y + h, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    v[5] = (GtBatchVertex){ x, y + h, 0, 0, color, scale, -1, GT_BATCH_TEXT, (uint8_t)ch, {0} };
    
    gtBatchEnsureCapacity(batch, 6);
    for (int i = 0; i < 6; i++) gtBatchAddVertex(batch, v[i]);
}

// Rasterise une ligne en vertices (quad épais = 4 vertices)
static void gtBatchEmitLine(GtBatchRenderer* batch, float x1, float y1, float x2, float y2, GtColor color) {
    // Pour ligne simple, on émet un quad fin (2 triangles)
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return;
    
    // Normalisée perpendiculaire pour épaisseur
    float nx = -dy / len * 0.5f;
    float ny = dx / len * 0.5f;
    
    GtBatchVertex v[4];
    v[0] = (GtBatchVertex){ x1 + nx, y1 + ny, 0, 0, color, 0, -1, GT_BATCH_LINE, 0, {0} };
    v[1] = (GtBatchVertex){ x1 - nx, y1 - ny, 0, 0, color, 0, -1, GT_BATCH_LINE, 0, {0} };
    v[2] = (GtBatchVertex){ x2 + nx, y2 + ny, 0, 0, color, 0, -1, GT_BATCH_LINE, 0, {0} };
    v[3] = (GtBatchVertex){ x2 - nx, y2 - ny, 0, 0, color, 0, -1, GT_BATCH_LINE, 0, {0} };
    
    gtBatchEnsureCapacity(batch, 4);
    for (int i = 0; i < 4; i++) gtBatchAddVertex(batch, v[i]);
}

GtBatchRenderer* gtBatchCreate(GtRenderer* renderer, const GtBatchConfig* config) {
    if (!renderer) return NULL;
    
    GtBatchConfig cfg = { 65536, 256, true };
    if (config) cfg = *config;
    if (cfg.max_vertices <= 0) cfg.max_vertices = 65536;
    if (cfg.max_images <= 0) cfg.max_images = 256;
    
    GtBatchRenderer* batch = (GtBatchRenderer*)calloc(1, sizeof(GtBatchRenderer));
    if (!batch) {
        gtReportError("gtBatchCreate : allocation OOM");
        return NULL;
    }
    
    batch->renderer = renderer;
    batch->config = cfg;
    batch->vertex_capacity = cfg.max_vertices;
    batch->vertices = (GtBatchVertex*)malloc((size_t)cfg.max_vertices * sizeof(GtBatchVertex));
    batch->images = (GtImage**)malloc((size_t)cfg.max_images * sizeof(GtImage*));
    
    if (!batch->vertices || !batch->images) {
        gtReportError("gtBatchCreate : allocation des buffers OOM");
        gtBatchDestroy(batch);
        return NULL;
    }
    
    return batch;
}

void gtBatchDestroy(GtBatchRenderer* batch) {
    if (!batch) return;
    free(batch->vertices);
    free(batch->images);
    free(batch);
}

void gtBatchBegin(GtBatchRenderer* batch) {
    if (!batch || batch->began) return;
    batch->vertex_count = 0;
    batch->image_count = 0;
    batch->transform_stack_depth = 0;
    batch->scissor_stack_depth = 0;
    batch->flush_count = 0;
    batch->draw_calls = 0;
    batch->began = true;
}

void gtBatchEnd(GtBatchRenderer* batch) {
    if (!batch || !batch->began) return;
    gtBatchFlush(batch);
    batch->began = false;
}

#ifndef LIBGT_NO_D2D
// Flush D2D : rejoue le command buffer via le render target GPU.
// Les vertices sont pré-transformés en espace écran (à l'ajout) → transform
// identité. Le scissor batch suit la même sémantique que le flush GDI :
// l'état FINAL de la pile s'applique à toutes les primitives du flush.
static void gtBatchFlushD2D(GtBatchRenderer* batch, GtRenderer* r) {
    if (!gtD2DBeginDraw(r)) {
        batch->vertex_count = 0;
        batch->image_count = 0;
        batch->flush_count++;
        return;
    }
    ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;

    D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
    ID2D1RenderTarget_SetTransform(rt, &id);

    bool clip_pushed = false;
    if (batch->scissor_stack_depth > 0) {
        GtBatchScissorState* s = &batch->scissor_stack[batch->scissor_stack_depth - 1];
        if (s->active) {
            D2D1_RECT_F rect;
            rect.left = (float)s->x;
            rect.top = (float)s->y;
            rect.right = rect.left + (float)s->w;
            rect.bottom = rect.top + (float)s->h;
            ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
            r->d2d_clip_depth++;
            clip_pushed = true;
        }
    }

    for (int i = 0; i < batch->vertex_count; ) {
        GtBatchVertex* v = &batch->vertices[i];
        GtImage* img = (v->image_id >= 0 && v->image_id < batch->image_count)
            ? batch->images[v->image_id] : NULL;

        switch (v->prim_type) {
            case GT_BATCH_RECT: {
                if (i + 5 < batch->vertex_count) {
                    float x = v[0].x;
                    float y = v[0].y;
                    float w = v[1].x - v[0].x;
                    float h = v[2].y - v[0].y;
                    if (w > 0.0f && h > 0.0f) {
                        if (img) {
                            ID2D1Bitmap* bmp = gtD2DGetTintedBitmap(rt, &r->d2d, img, v[0].color);
                            if (bmp) {
                                D2D1_RECT_F dst;
                                dst.left = x; dst.top = y;
                                dst.right = x + w; dst.bottom = y + h;
                                ID2D1RenderTarget_DrawBitmap(rt, bmp, &dst, 1.0f,
                                                             D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
                            }
                        } else {
                            gtD2DFillRect(rt, &r->d2d, x, y, w, h, v[0].color);
                        }
                    }
                }
                i += 6;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_CIRCLE: {
                int radius = (int)v[0].radius;
                if (radius > 0) {
                    gtD2DFillEllipse(rt, &r->d2d, v[0].x, v[0].y, (float)radius + 0.5f, v[0].color);
                }
                i += 1;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_LINE: {
                if (i + 3 < batch->vertex_count) {
                    gtD2DDrawLineSeg(rt, &r->d2d, v[0].x, v[0].y, v[2].x, v[2].y, v[0].color, 1.0f);
                }
                i += 4;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_TEXT: {
                if (i + 5 < batch->vertex_count && v[0].glyph >= 32 && v[0].glyph <= 126) {
                    float x = v[0].x;
                    float y = v[0].y;
                    float w = v[1].x - v[0].x;
                    float h = v[2].y - v[0].y;
                    if (w > 0.0f && h > 0.0f) {
                        ID2D1Bitmap* atlas = gtD2DGetFontAtlas(rt, &r->d2d, v[0].color);
                        if (atlas) {
                            float scale = (v[0].radius > 0.0f) ? v[0].radius : h / 8.0f;
                            gtD2DDrawGlyph(rt, &r->d2d, atlas, x, y, (char)v[0].glyph, scale);
                        }
                    }
                }
                i += 6;
                batch->draw_calls++;
                break;
            }
            default:
                i++;
                break;
        }
    }

    if (clip_pushed) {
        ID2D1RenderTarget_PopAxisAlignedClip(rt);
        r->d2d_clip_depth--;
    }

    batch->vertex_count = 0;
    batch->image_count = 0;
    batch->flush_count++;
}
#endif // LIBGT_NO_D2D

void gtBatchFlush(GtBatchRenderer* batch) {
    if (!batch || !batch->began || batch->vertex_count == 0) return;

    GtRenderer* r = batch->renderer;
    if (!r) return;

#ifndef LIBGT_NO_D2D
    if (r->type == GT_RENDERER_D2D) {
        gtBatchFlushD2D(batch, r);
        return;
    }
#endif

    if (r->type != GT_RENDERER_GDI) return;

    GtWindow* win = r->window;
    if (!win) return;
    
    // Pour GDI : on parcourt les vertices et on dessine directement
    // GDI : le batching parcourt les primitives côté CPU.
    batch->draw_calls = 0;
    
    // Tri par image_id pour minimiser les changements de texture
    // (Pour l'instant, on dessine dans l'ordre - optimisable)
    
    for (int i = 0; i < batch->vertex_count; ) {
        GtBatchVertex* v = &batch->vertices[i];
        GtImage* img = (v->image_id >= 0 && v->image_id < batch->image_count) ? batch->images[v->image_id] : NULL;
        
        switch (v->prim_type) {
            case GT_BATCH_RECT: {
                // 6 vertices = 2 triangles = 1 rect
                if (i + 5 < batch->vertex_count) {
                    float x = v[0].x;
                    float y = v[0].y;
                    float w = v[1].x - v[0].x;
                    float h = v[2].y - v[0].y;
                    
                    // Clip contre fenêtre
                    int ix = (int)x; if (ix < 0) ix = 0;
                    int iy = (int)y; if (iy < 0) iy = 0;
                    int iw = (int)w; if (ix + iw > win->width) iw = win->width - ix;
                    int ih = (int)h; if (iy + ih > win->height) ih = win->height - iy;
                    
                    if (iw > 0 && ih > 0 && gtBatchClipRectScissor(batch, &ix, &iy, &iw, &ih)) {
                        if (img) {
                            // Draw image rect (simplified)
                            gtRendererDrawImageEx(r, img, ix, iy, iw, ih, 0.0f, gtVec2(0, 0), false, false, v[0].color);
                        } else {
                            // Draw filled rect
                            for (int row = iy; row < iy + ih; row++) {
                                size_t idx = (size_t)row * (size_t)win->width + (size_t)ix;
                                uint32_t* row_ptr = &win->buffer[idx];
                                for (int col = 0; col < iw; col++) {
                                    row_ptr[col] = (v[0].color >> 24 == 255) ? (v[0].color | 0xFF000000u) : gtBlendPixel(row_ptr[col], v[0].color);
                                }
                            }
                        }
                    }
                }
                i += 6;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_CIRCLE: {
                // 1 vertex = 1 cercle (centre) ; rayon et couleur portés par le vertex
                int cx = (int)v[0].x;
                int cy = (int)v[0].y;
                int radius = (int)v[0].radius;
                if (radius > 0 && gtBatchCheckScissor(batch, cx - radius, cy - radius)) {
                    gtDrawCircle(win, cx, cy, radius, v[0].color);
                }
                i += 1;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_TEXT: {
                // 6 vertices = quad d'un caractère ; échantillonne la bitmap font 8x8
                if (i + 5 < batch->vertex_count && v[0].glyph >= 32 && v[0].glyph <= 126) {
                    float x = v[0].x;
                    float y = v[0].y;
                    float w = v[1].x - v[0].x;
                    float h = v[2].y - v[0].y;

                    // Clip contre fenêtre
                    int ix = (int)x; if (ix < 0) ix = 0;
                    int iy = (int)y; if (iy < 0) iy = 0;
                    int iw = (int)w; if (ix + iw > win->width) iw = win->width - ix;
                    int ih = (int)h; if (iy + ih > win->height) ih = win->height - iy;

                    if (iw > 0 && ih > 0 && gtBatchClipRectScissor(batch, &ix, &iy, &iw, &ih)) {
                        // Même logique que gtDrawChar : blocs [row*scale,(row+1)*scale) x [col*scale,(col+1)*scale)
                        const uint8_t* glyph_data = &gt_font8x8[(v[0].glyph - 32) * 8];
                        float scale = (v[0].radius > 0.0f) ? v[0].radius : (float)h / 8.0f;
                        uint32_t color = v[0].color;
                        bool opaque = ((color >> 24) == 255);

                        for (int row = 0; row < 8; row++) {
                            uint8_t bits = glyph_data[row];
                            if (!bits) continue;
                            int py_start = (int)(y + row * scale);
                            int py_end = (int)(y + (row + 1) * scale);
                            if (py_start < iy) py_start = iy;
                            if (py_end > iy + ih) py_end = iy + ih;
                            for (int col = 0; col < 8; col++) {
                                if (!(bits & (0x80 >> col))) continue;
                                int px_start = (int)(x + col * scale);
                                int px_end = (int)(x + (col + 1) * scale);
                                if (px_start < ix) px_start = ix;
                                if (px_end > ix + iw) px_end = ix + iw;
                                for (int py = py_start; py < py_end; py++) {
                                    uint32_t* row_ptr = &win->buffer[(size_t)py * (size_t)win->width];
                                    for (int px = px_start; px < px_end; px++) {
                                        row_ptr[px] = opaque ? (color | 0xFF000000u) : gtBlendPixel(row_ptr[px], color);
                                    }
                                }
                            }
                        }
                    }
                }
                i += 6;
                batch->draw_calls++;
                break;
            }
            case GT_BATCH_LINE: {
                if (i + 3 < batch->vertex_count) {
                    int x1 = (int)v[0].x, y1 = (int)v[0].y;
                    int x2 = (int)v[2].x, y2 = (int)v[2].y; // Opposite corner of quad
                    gtDrawLine(win, x1, y1, x2, y2, v[0].color);
                }
                i += 4;
                batch->draw_calls++;
                break;
            }
            default:
                i++;
                break;
        }
    }
    
    batch->vertex_count = 0;
    batch->image_count = 0;
    batch->flush_count++;
}

void gtBatchAddRect(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color) {
    if (!batch || !batch->began || w <= 0 || h <= 0) return;
    float tx = x, ty = y, tw = w, th = h;
    gtBatchApplyTransformRect(batch, tx, ty, tw, th, &tx, &ty, &tw, &th);
    gtBatchEmitRect(batch, tx, ty, tw, th, color, -1, 0, 0, 1, 1);
}

void gtBatchAddRectLines(GtBatchRenderer* batch, float x, float y, float w, float h, GtColor color) {
    if (!batch || !batch->began || w <= 0 || h <= 0) return;
    // Emit as 4 lines for now
    gtBatchAddLine(batch, x, y, x + w, y, color);
    gtBatchAddLine(batch, x + w, y, x + w, y + h, color);
    gtBatchAddLine(batch, x + w, y + h, x, y + h, color);
    gtBatchAddLine(batch, x, y + h, x, y, color);
}

void gtBatchAddLine(GtBatchRenderer* batch, float x1, float y1, float x2, float y2, GtColor color) {
    if (!batch || !batch->began) return;
    GtVec2 p1 = gtBatchApplyTransform(batch, gtVec2(x1, y1));
    GtVec2 p2 = gtBatchApplyTransform(batch, gtVec2(x2, y2));
    gtBatchEmitLine(batch, p1.x, p1.y, p2.x, p2.y, color);
}

void gtBatchAddCircle(GtBatchRenderer* batch, float cx, float cy, float radius, GtColor color) {
    if (!batch || !batch->began || radius <= 0) return;
    GtVec2 p = gtBatchApplyTransform(batch, gtVec2(cx, cy));
    gtBatchEmitCircle(batch, p.x, p.y, radius, color);
}

void gtBatchAddCircleLines(GtBatchRenderer* batch, float cx, float cy, float radius, GtColor color) {
    if (!batch || !batch->began || radius <= 0) return;
    // Pour l'instant, même chose que filled (à améliorer avec line loop)
    gtBatchAddCircle(batch, cx, cy, radius, color);
}

void gtBatchAddImage(GtBatchRenderer* batch, GtImage* image, float x, float y, 
                     float w, float h, GtColor tint,
                     float u0, float v0, float u1, float v1,
                     GtVec2 origin, float rot, float scale) {
    if (!batch || !batch->began || !image || w <= 0 || h <= 0) return;
    
    // Applique origin
    float ox = x - origin.x * w * scale;
    float oy = y - origin.y * h * scale;
    float tw = w * scale;
    float th = h * scale;
    
    // Applique rotation via transform stack (simplifié: rotation autour du centre)
    if (rot != 0.0f) {
        gtBatchPushTransform(batch);
        gtBatchTranslate(batch, ox + tw * 0.5f, oy + th * 0.5f);
        gtBatchRotate(batch, rot);
        gtBatchTranslate(batch, -tw * 0.5f, -th * 0.5f);
    }
    
    int img_id = gtBatchGetImageId(batch, image);
    float tx = ox, ty = oy, ttw = tw, tth = th;
    gtBatchApplyTransformRect(batch, tx, ty, ttw, tth, &tx, &ty, &ttw, &tth);
    
    if (rot != 0.0f) {
        gtBatchPopTransform(batch);
    }
    
    gtBatchEmitRect(batch, tx, ty, ttw, tth, tint, img_id, u0, v0, u1, v1);
}

void gtBatchAddText(GtBatchRenderer* batch, const char* text, float x, float y, 
                    GtColor color, float scale, int spacing) {
    if (!batch || !batch->began || !text) return;
    
    GtVec2 p = gtBatchApplyTransform(batch, gtVec2(x, y));
    int char_w = (int)(8 * scale);
    int cur_x = (int)p.x;
    int cur_y = (int)p.y;
    
    for (const char* c = text; *c; ) {
        int code = gtTextNextGlyph(&c);       // decode UTF-8/CP-1252, avance c
        if (code < 0) break;
        if (code == '\n') {
            cur_x = (int)p.x;
            cur_y += (int)(8 * scale) + spacing;
            continue;
        }
        int gi = gtGlyphIndexForCode(code);
        if (gi < 0) { cur_x += char_w + spacing; continue; }

        // Un quad par caractère ; le flush échantillonne la bitmap font 8x8
        gtBatchEmitGlyph(batch, (float)cur_x, (float)cur_y, (float)char_w, (float)(8 * scale), color, (char)gi, scale);
        cur_x += char_w + spacing;
    }
}

void gtBatchSetTransform(GtBatchRenderer* batch, const GtMat3* transform) {
    if (!batch) return;
    if (batch->transform_stack_depth == 0) {
        if (transform) {
            batch->transform_stack[0] = *transform;
            batch->transform_stack_depth = 1;
        }
    } else {
        if (transform) {
            batch->transform_stack[batch->transform_stack_depth - 1] = *transform;
        } else {
            batch->transform_stack_depth = 0;
        }
    }
}

void gtBatchPushTransform(GtBatchRenderer* batch) {
    if (!batch || batch->transform_stack_depth >= GT_BATCH_MAX_TRANSFORM_STACK) return;
    if (batch->transform_stack_depth == 0) {
        batch->transform_stack[0] = gtMat3Identity();
    } else {
        batch->transform_stack[batch->transform_stack_depth] = batch->transform_stack[batch->transform_stack_depth - 1];
    }
    batch->transform_stack_depth++;
}

void gtBatchPopTransform(GtBatchRenderer* batch) {
    if (!batch || batch->transform_stack_depth == 0) return;
    batch->transform_stack_depth--;
}

void gtBatchTranslate(GtBatchRenderer* batch, float tx, float ty) {
    if (!batch) return;
    GtMat3 m = gtMat3Translate(tx, ty);
    if (batch->transform_stack_depth == 0) gtBatchPushTransform(batch);
    GtMat3* cur = &batch->transform_stack[batch->transform_stack_depth - 1];
    *cur = gtMat3Mul(*cur, m);
}

void gtBatchRotate(GtBatchRenderer* batch, float angle_rad) {
    if (!batch) return;
    GtMat3 m = gtMat3Rotate(angle_rad);
    if (batch->transform_stack_depth == 0) gtBatchPushTransform(batch);
    GtMat3* cur = &batch->transform_stack[batch->transform_stack_depth - 1];
    *cur = gtMat3Mul(*cur, m);
}

void gtBatchScale(GtBatchRenderer* batch, float sx, float sy) {
    if (!batch) return;
    GtMat3 m = gtMat3Scale(sx, sy);
    if (batch->transform_stack_depth == 0) gtBatchPushTransform(batch);
    GtMat3* cur = &batch->transform_stack[batch->transform_stack_depth - 1];
    *cur = gtMat3Mul(*cur, m);
}

void gtBatchSetScissorRect(GtBatchRenderer* batch, int x, int y, int w, int h) {
    if (!batch) return;
    if (batch->scissor_stack_depth == 0) {
        batch->scissor_stack[0].x = x;
        batch->scissor_stack[0].y = y;
        batch->scissor_stack[0].w = w;
        batch->scissor_stack[0].h = h;
        batch->scissor_stack[0].active = (w > 0 && h > 0);
        batch->scissor_stack_depth = 1;
    } else {
        batch->scissor_stack[batch->scissor_stack_depth - 1].x = x;
        batch->scissor_stack[batch->scissor_stack_depth - 1].y = y;
        batch->scissor_stack[batch->scissor_stack_depth - 1].w = w;
        batch->scissor_stack[batch->scissor_stack_depth - 1].h = h;
        batch->scissor_stack[batch->scissor_stack_depth - 1].active = (w > 0 && h > 0);
    }
}

void gtBatchGetStats(const GtBatchRenderer* batch, GtBatchStats* out_stats) {
    if (!batch || !out_stats) return;
    out_stats->vertices_used = batch->vertex_count;
    out_stats->max_vertices = batch->vertex_capacity;
    out_stats->flush_count = batch->flush_count;
    out_stats->draw_calls = batch->draw_calls;
}

/* -------------------------------------------------------------------------
* IMPLÉMENTATION : CHARGEMENT D'IMAGES (stb_image)
* ------------------------------------------------------------------------- */
GtImage* gtLoadImage(const char* filepath, int* out_width, int* out_height) {
    if (!filepath) return NULL;

    FILE* f = fopen(filepath, "rb");
    if (!f) {
        gtReportError("gtLoadImage : fichier introuvable (%s)", filepath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        gtReportError("gtLoadImage : fichier vide (%s)", filepath);
        fclose(f);
        return NULL;
    }

    unsigned char* file_data = (unsigned char*)malloc((size_t)file_size);
    if (!file_data) {
        gtReportError("gtLoadImage : allocation OOM (%s)", filepath);
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(file_data, 1, (size_t)file_size, f);
    fclose(f);

    if (read_bytes != (size_t)file_size) {
        gtReportError("gtLoadImage : lecture incomplete (%s)", filepath);
        free(file_data);
        return NULL;
    }

    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(file_data, (int)file_size, &w, &h, &channels, 4);
    free(file_data);

    if (!pixels) {
        gtReportError("gtLoadImage : format non supporte ou image corrompue (%s)", filepath);
        return NULL;
    }

    // Alloue GtImage
    // calloc : les champs de cache D2D (d2d_bitmap/d2d_owner) doivent etre
    // a zero - gtFreeImage libere d2d_bitmap si non-NULL.
    GtImage* image = (GtImage*)calloc(1, sizeof(GtImage));
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
#ifndef LIBGT_NO_D2D
    if (image->d2d_bitmap) ID2D1Bitmap_Release((ID2D1Bitmap*)image->d2d_bitmap);
#endif
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
// Version interne scalée (utilisée par gtDrawTextEx)
static void gtDrawCharScaled(GtWindow* window, int x, int y, char c, uint32_t color, float scale) {
    int idx = gtGlyphIndexForCode((unsigned char)c);  // ASCII + accents CP-1252
    if (idx < 0) return;
    const uint8_t* glyph = gtFontGlyphData(idx);
    if (!glyph) return;

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

// Dessine un caractère unique (API publique, scale 1). Route vers l'atlas GPU
// si un renderer D2D est attaché à la fenêtre, sinon blit bitmap CPU.
void gtDrawChar(GtWindow* window, int x, int y, char c, uint32_t color) {
    if (!window) return;
#ifndef LIBGT_NO_D2D
    if (window->renderer && window->renderer->type == GT_RENDERER_D2D) {
        GtRenderer* r = window->renderer;
        if (!gtD2DBeginDraw(r)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
        ID2D1Bitmap* atlas = gtD2DGetFontAtlas(rt, &r->d2d, color);
        if (!atlas) return;
        D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
        ID2D1RenderTarget_SetTransform(rt, &id);
        gtD2DDrawGlyph(rt, &r->d2d, atlas, (float)x, (float)y, c, 1.0f);
        return;
    }
#endif
    gtDrawCharScaled(window, x, y, c, color, 1.0f);
}

void gtDrawText(GtWindow* window, int x, int y, const char* text, uint32_t color) {
    gtDrawTextEx(window, x, y, text, color, 1.0f, 0, 0);
}

void gtDrawTextEx(GtWindow* window, int x, int y, const char* text, uint32_t color,
                  float scale, int spacing, int wrap_width) {
    if (!window || !text) return;
#ifndef LIBGT_NO_D2D
    // Mode D2D : texte via l'atlas GPU (z-order = ordre d'appel)
    if (window->renderer && window->renderer->type == GT_RENDERER_D2D) {
        GtRenderer* r = window->renderer;
        if (!gtD2DBeginDraw(r)) return;
        ID2D1RenderTarget* rt = (ID2D1RenderTarget*)r->d2d_rt;
        ID2D1Bitmap* atlas = gtD2DGetFontAtlas(rt, &r->d2d, color);
        if (!atlas) return;
        D2D1_MATRIX_3X2_F id = gtD2DMatrix(NULL);
        ID2D1RenderTarget_SetTransform(rt, &id);
        gtD2DDrawTextRun(rt, &r->d2d, atlas, text, (float)x, (float)y, scale, spacing, wrap_width);
        return;
    }
#endif
    if (!window->buffer) return;
    if (scale <= 0.0f) scale = 1.0f;

    int cur_x = x;
    int cur_y = y;
    int char_w = (int)(8 * scale);
    int line_h = (int)(8 * scale) + spacing;

    for (const char* p = text; *p; ) {
        int code = gtTextNextGlyph(&p);       // decode UTF-8/CP-1252, avance p
        if (code < 0) break;
        char c = (char)code;

        // Gestion saut de ligne explicite
        if (c == '\n') {
            cur_x = x;
            cur_y += line_h;
            continue;
        }

        // Tabulation : avance de 4 cellules de caractère
        if (c == '\t') {
            cur_x += char_w * 4;
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

        gtDrawCharScaled(window, cur_x, cur_y, c, color, scale);
        cur_x += char_w + spacing;
    }
}

// Texte formaté style printf : formate en buffer interne puis délègue à
// gtDrawTextEx (routing D2D identique). Tronqué à 511 caractères.
void gtDrawTextFmt(GtWindow* window, int x, int y, uint32_t color, const char* fmt, ...) {
    if (!window || !fmt) return;

    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    gtDrawTextEx(window, x, y, buffer, color, 1.0f, 0, 0);
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

    for (const char* p = text; *p; ) {
        int code = gtTextNextGlyph(&p);       // decode UTF-8/CP-1252, avance p
        if (code < 0) break;
        char c = (char)code;

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
    // Taille effective, axe par axe : size explicite > dimensions de l'image
    // > 32x32 en dernier recours (sprite sans image, placeholder de rendu).
    float w = s->size.x;
    float h = s->size.y;
    if (w <= 0.0f && s->image) w = (float)gtImageGetWidth(s->image);
    if (h <= 0.0f && s->image) h = (float)gtImageGetHeight(s->image);
    if (w <= 0.0f) w = 32.0f;
    if (h <= 0.0f) h = 32.0f;
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

        float x, y, w, h;
        gtECSGetSpriteRect(ecs, idx, &x, &y, &w, &h);

        if (s->image) {
            // Image réelle : dessinée via le renderer, avec teinte et flips.
            // TODO: la rotation de GtTransform reste ignorée (AABB seulement),
            // comme pour l'API directe gtRendererDrawImageEx.
            gtRendererDrawImageEx(renderer, s->image,
                                  (int)x, (int)y, (int)w, (int)h,
                                  0.0f, gtVec2(0, 0), s->flip_x, s->flip_y,
                                  s->tint);
        } else {
            // Sans image : rectangle teinté + contour comme placeholder
            gtRendererDrawRect(renderer, (int)x, (int)y, (int)w, (int)h, s->tint);
            gtRendererDrawRectLines(renderer, (int)x, (int)y, (int)w, (int)h, GT_WHITE);
        }
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
// Teste une paire d'entites (layers/masks + formes) et invoque on_hit si contact.
// Factorise : utilise par le chemin O(n^2) ET par la grille spatiale. Les
// pointeurs sont re-resolus a chaque appel car le callback peut detruire
// des entites ou retirer des composants en cours de route.
static void gtECSTestPair(GtECS* ecs, GtEntity ent_a, GtEntity ent_b,
                          GtECSCollisionFunc on_hit, void* user_data) {
    if (!gtECSIsAlive(ecs, ent_a) || !gtECSIsAlive(ecs, ent_b)) return;

    GtCollider* col_a = gtECSGetCollider(ecs, ent_a);
    GtCollider* col_b = gtECSGetCollider(ecs, ent_b);
    if (!col_a || !col_b) return;
    if (!gtECSGetTransform(ecs, ent_a) || !gtECSGetTransform(ecs, ent_b)) return;

    // Verifie layers/masks dans les deux sens.
    if ((col_a->mask & col_b->layer) == 0 || (col_b->mask & col_a->layer) == 0) return;

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
        // est donc le mecanisme de reaction, trigger ou collision solide.
        on_hit(ecs, ent_a, ent_b, user_data);
    }
}

// Extension (rayon / demi-extent max) d'un collider, pour la grille
static float gtECSColliderExtent(const GtCollider* col) {
    if (col->type == GT_COLLIDER_CIRCLE) return col->circle.radius;
    float hx = col->box.half_extents.x;
    float hy = col->box.half_extents.y;
    return (hx > hy) ? hx : hy;
}

void gtECSSystemCollisions(GtECS* ecs, GtECSCollisionFunc on_hit, void* user_data) {
    if (!ecs || !on_hit || !ecs->alive || !ecs->masks ||
        !ecs->generations || !ecs->colliders || !ecs->transforms) return;
    if (ecs->alive_count < 2) return;

    // Snapshot des handles, pas seulement des indices : si le callback detruit
    // puis recree une entite pendant la boucle, l'ancienne handle reste invalide.
    GtEntity* entities = (GtEntity*)malloc((size_t)ecs->alive_count * sizeof(*entities));
    if (!entities) return;

    int count = 0;
    for (int idx = 0; idx < ecs->capacity; idx++) {
        if (!ecs->alive[idx]) continue;
        if ((ecs->masks[idx] & (GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) !=
            (GT_COMP_TRANSFORM | GT_COMP_COLLIDER)) continue;
        entities[count++] = GT_ENTITY_MAKE(idx, ecs->generations[idx]);
    }

    // Petit nombre d'entites : O(n^2) direct (la grille coute plus cher)
    if (count <= 32) {
        for (int i = 0; i < count; i++) {
            if (!gtECSIsAlive(ecs, entities[i])) continue;
            if (!gtECSGetCollider(ecs, entities[i])) continue;
            for (int j = i + 1; j < count; j++) {
                gtECSTestPair(ecs, entities[i], entities[j], on_hit, user_data);
            }
        }
        free(entities);
        return;
    }

    // ---- Grille spatiale uniforme (O(n) typique) ----
    // Cellule = 2x le plus grand collider ; grille bornee a 64x64 cellules
    // (au-dela, on double la taille de cellule). Deux passes d'insertion en
    // arena : zero allocation par bucket.
    float cell = 1.0f;
    for (int i = 0; i < count; i++) {
        GtCollider* col = gtECSGetCollider(ecs, entities[i]);
        if (col) {
            float ext = gtECSColliderExtent(col);
            if (ext > cell) cell = ext;
        }
    }
    cell *= 2.0f;

    float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
    bool have_bounds = false;
    for (int i = 0; i < count; i++) {
        GtTransform* t = gtECSGetTransform(ecs, entities[i]);
        if (!t) continue;
        if (!have_bounds) {
            min_x = max_x = t->pos.x;
            min_y = max_y = t->pos.y;
            have_bounds = true;
        } else {
            if (t->pos.x < min_x) min_x = t->pos.x;
            if (t->pos.x > max_x) max_x = t->pos.x;
            if (t->pos.y < min_y) min_y = t->pos.y;
            if (t->pos.y > max_y) max_y = t->pos.y;
        }
    }
    if (!have_bounds) { free(entities); return; }

    int gw = (int)((max_x - min_x) / cell) + 1;
    int gh = (int)((max_y - min_y) / cell) + 1;
    while (gw * gh > 64 * 64) {   // grille trop grande : cellules plus larges
        cell *= 2.0f;
        gw = (int)((max_x - min_x) / cell) + 1;
        gh = (int)((max_y - min_y) / cell) + 1;
    }
    int ncells = gw * gh;

    int* cell_start = (int*)calloc((size_t)ncells + 1, sizeof(int));
    int* cell_items = (int*)malloc((size_t)count * 8 * sizeof(int)); // >= 4 cellules/entite
    int* tested_with = (int*)malloc((size_t)count * sizeof(int));
    if (!cell_start || !cell_items || !tested_with) {
        // Grille impossible : repli sur l'O(n^2)
        free(cell_start); free(cell_items); free(tested_with);
        for (int i = 0; i < count; i++) {
            if (!gtECSIsAlive(ecs, entities[i])) continue;
            for (int j = i + 1; j < count; j++)
                gtECSTestPair(ecs, entities[i], entities[j], on_hit, user_data);
        }
        free(entities);
        return;
    }
    for (int i = 0; i < count; i++) tested_with[i] = -1;

    // Passe 1 : comptage des insertions par cellule
    for (int i = 0; i < count; i++) {
        GtTransform* t = gtECSGetTransform(ecs, entities[i]);
        GtCollider* col = gtECSGetCollider(ecs, entities[i]);
        if (!t || !col) continue;
        float ext = gtECSColliderExtent(col);
        float ox = (col->offset.x >= 0) ? col->offset.x : -col->offset.x;
        float oy = (col->offset.y >= 0) ? col->offset.y : -col->offset.y;
        ext += (ox > oy) ? ox : oy;
        int cx0 = (int)((t->pos.x - ext - min_x) / cell); if (cx0 < 0) cx0 = 0;
        int cx1 = (int)((t->pos.x + ext - min_x) / cell); if (cx1 >= gw) cx1 = gw - 1;
        int cy0 = (int)((t->pos.y - ext - min_y) / cell); if (cy0 < 0) cy0 = 0;
        int cy1 = (int)((t->pos.y + ext - min_y) / cell); if (cy1 >= gh) cy1 = gh - 1;
        for (int cy = cy0; cy <= cy1; cy++)
            for (int cx = cx0; cx <= cx1; cx++)
                cell_start[cy * gw + cx + 1]++;
    }
    for (int c = 0; c < ncells; c++) cell_start[c + 1] += cell_start[c];

    // Passe 2 : remplissage de l'arena
    int* cursor = (int*)malloc((size_t)ncells * sizeof(int));
    if (!cursor) {
        free(cell_start); free(cell_items); free(tested_with);
        for (int i = 0; i < count; i++)
            for (int j = i + 1; j < count; j++)
                gtECSTestPair(ecs, entities[i], entities[j], on_hit, user_data);
        free(entities);
        return;
    }
    for (int c = 0; c < ncells; c++) cursor[c] = cell_start[c];
    for (int i = 0; i < count; i++) {
        GtTransform* t = gtECSGetTransform(ecs, entities[i]);
        GtCollider* col = gtECSGetCollider(ecs, entities[i]);
        if (!t || !col) continue;
        float ext = gtECSColliderExtent(col);
        float ox = (col->offset.x >= 0) ? col->offset.x : -col->offset.x;
        float oy = (col->offset.y >= 0) ? col->offset.y : -col->offset.y;
        ext += (ox > oy) ? ox : oy;
        int cx0 = (int)((t->pos.x - ext - min_x) / cell); if (cx0 < 0) cx0 = 0;
        int cx1 = (int)((t->pos.x + ext - min_x) / cell); if (cx1 >= gw) cx1 = gw - 1;
        int cy0 = (int)((t->pos.y - ext - min_y) / cell); if (cy0 < 0) cy0 = 0;
        int cy1 = (int)((t->pos.y + ext - min_y) / cell); if (cy1 >= gh) cy1 = gh - 1;
        for (int cy = cy0; cy <= cy1; cy++)
            for (int cx = cx0; cx <= cx1; cx++)
                cell_items[cursor[cy * gw + cx]++] = i;
    }
    free(cursor);

    // Paires : chaque entite interroge ses cellules ; j > i garantit un test
    // unique par paire (deux colliders en contact partagent au moins une cellule).
    for (int i = 0; i < count; i++) {
        if (!gtECSIsAlive(ecs, entities[i])) continue;
        GtTransform* t = gtECSGetTransform(ecs, entities[i]);
        GtCollider* col = gtECSGetCollider(ecs, entities[i]);
        if (!t || !col) continue;
        float ext = gtECSColliderExtent(col);
        float ox = (col->offset.x >= 0) ? col->offset.x : -col->offset.x;
        float oy = (col->offset.y >= 0) ? col->offset.y : -col->offset.y;
        ext += (ox > oy) ? ox : oy;
        int cx0 = (int)((t->pos.x - ext - min_x) / cell); if (cx0 < 0) cx0 = 0;
        int cx1 = (int)((t->pos.x + ext - min_x) / cell); if (cx1 >= gw) cx1 = gw - 1;
        int cy0 = (int)((t->pos.y - ext - min_y) / cell); if (cy0 < 0) cy0 = 0;
        int cy1 = (int)((t->pos.y + ext - min_y) / cell); if (cy1 >= gh) cy1 = gh - 1;
        for (int cy = cy0; cy <= cy1; cy++) {
            for (int cx = cx0; cx <= cx1; cx++) {
                int c = cy * gw + cx;
                for (int k = cell_start[c]; k < cell_start[c + 1]; k++) {
                    int j = cell_items[k];
                    if (j <= i) continue;              // paire deja testee (ou soi-meme)
                    if (tested_with[j] == i) continue; // deja vu dans une autre cellule
                    tested_with[j] = i;
                    gtECSTestPair(ecs, entities[i], entities[j], on_hit, user_data);
                }
            }
        }
    }

    free(cell_start);
    free(cell_items);
    free(tested_with);
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
        // Passage par uintptr_t : conversion propre ISO C objet<->fonction
        // (évite le warning -Wpedantic du cast direct).
        g_gtXInput.get_state = (DWORD (WINAPI*)(DWORD, XINPUT_STATE*))(uintptr_t)
            GetProcAddress(g_gtXInput.module, "XInputGetState");
        g_gtXInput.set_state = (DWORD (WINAPI*)(DWORD, XINPUT_VIBRATION*))(uintptr_t)
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

// --- Helpers internes : ownership des noms ---
// Convention : les noms de maps/actions sont TOUJOURS des copies possédées
// par la structure qui les porte (_strdup). Toute copie de map entre
// structures passe par gtInputDeepCopyMap pour éviter le partage de
// pointeurs (et donc les double-free).

// Libère les noms possédés d'une map (tous les slots ; free(NULL) est un no-op)
static void gtInputFreeMapNames(GtActionMap* map) {
    if (!map) return;
    free((void*)map->name);
    map->name = NULL;
    for (int a = 0; a < GT_MAX_ACTIONS_PER_MAP; a++) {
        free((void*)map->actions[a].name);
        map->actions[a].name = NULL;
    }
}

// Copie profonde d'une map : dst libère ses anciens noms, puis re-duplique
// ceux de src. src reste inchangé et conserve la propriété de ses noms.
static void gtInputDeepCopyMap(GtActionMap* dst, const GtActionMap* src) {
    gtInputFreeMapNames(dst);
    *dst = *src;                    // copie bindings/états (noms aliasés un instant)
    dst->name = NULL;               // …réinitialisés immédiatement avant re-duplication
    for (int a = 0; a < GT_MAX_ACTIONS_PER_MAP; a++) dst->actions[a].name = NULL;
    if (src->name) dst->name = _strdup(src->name);

    int count = src->action_count;
    if (count < 0) count = 0;
    if (count > GT_MAX_ACTIONS_PER_MAP) count = GT_MAX_ACTIONS_PER_MAP;
    for (int a = 0; a < count; a++) {
        if (src->actions[a].name) dst->actions[a].name = _strdup(src->actions[a].name);
    }
}

// Libère les noms possédés par toutes les maps d'un profil
static void gtInputFreeProfileNames(GtPlayerProfile* profile) {
    if (!profile) return;
    for (int m = 0; m < profile->map_count && m < GT_MAX_MAPS_PER_PROFILE; m++) {
        gtInputFreeMapNames(&profile->maps[m]);
    }
}

// Détruit le système d'input
void gtInputDestroy(GtInputSystem* input) {
    if (!input) return;
    // Arrête vibrations
    for (int i = 0; i < GT_MAX_PLAYERS; i++) {
        XINPUT_VIBRATION vib = {0, 0};
        gtXInputSetState(i, &vib);
    }
    // Libère les noms dupliqués des profils possédés par le système
    for (int i = 0; i < GT_MAX_PLAYERS; i++) {
        gtInputFreeProfileNames(&input->profiles[i]);
    }
    free(input);
}

// Libère un profil possédé par l'appelant (typiquement après gtInputLoadProfile)
void gtInputDestroyProfile(GtPlayerProfile* profile) {
    if (!profile) return;
    gtInputFreeProfileNames(profile);
    memset(profile, 0, sizeof(*profile));
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
    GtPlayerProfile* dst = &input->profiles[player_index];

    int count = profile->map_count;
    if (count < 0) count = 0;
    if (count > GT_MAX_MAPS_PER_PROFILE) count = GT_MAX_MAPS_PER_PROFILE;

    // Copie profonde : le système duplique les noms des maps, le profil
    // source reste intact et demeure la propriété de l'appelant.
    for (int m = 0; m < GT_MAX_MAPS_PER_PROFILE; m++) {
        gtInputFreeMapNames(&dst->maps[m]);          // anciens noms du système
        memset(&dst->maps[m], 0, sizeof(GtActionMap));
        if (m < count) {
            gtInputDeepCopyMap(&dst->maps[m], &profile->maps[m]);
        }
    }

    // Settings et stacks copiés tels quels
    dst->player_index = player_index;
    dst->stick_deadzone = profile->stick_deadzone;
    dst->trigger_threshold = profile->trigger_threshold;
    dst->vibration_strength = profile->vibration_strength;
    dst->gamepad_connected = profile->gamepad_connected;
    dst->mouse_sensitivity = profile->mouse_sensitivity;
    dst->invert_y = profile->invert_y;
    memcpy(dst->active_map_stack, profile->active_map_stack, sizeof(dst->active_map_stack));
    dst->active_map_count = (profile->active_map_count >= 0) ? profile->active_map_count : 0;
    dst->map_count = count;
}

// Action Maps
GtActionMap* gtInputCreateMap(const char* name) {
    if (!name) return NULL;
    GtActionMap* map = (GtActionMap*)calloc(1, sizeof(GtActionMap));
    if (!map) return NULL;
    map->name = _strdup(name); // Copie possédée : la chaîne passée peut être temporaire
    map->enabled = true;
    return map;
}

void gtInputDestroyMap(GtActionMap* map) {
    if (!map) return;
    gtInputFreeMapNames(map); // Libère les copies des noms de map et d'actions
    free(map);
}

void gtInputAddAction(GtActionMap* map, const char* action_name) {
    if (!map || !action_name) return;
    if (map->action_count >= GT_MAX_ACTIONS_PER_MAP) return;
    if (gtInputFindAction(map, action_name)) return; // Déjà existante

    GtAction* action = &map->actions[map->action_count];
    action->name = _strdup(action_name); // Copie possédée
    if (!action->name) return;
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
    // la copie stockée dans le profil. Copie PROFONDE : le profil duplique
    // les noms, la map source reste la propriété de l'appelant.
    gtInputDeepCopyMap(&profile->maps[map_idx], map);

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
    // Libère les noms de l'ancien contenu avant de vider le stockage
    gtInputFreeProfileNames(profile);
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
    
    // Libère les noms d'un éventuel contenu précédent avant le reset
    gtInputFreeProfileNames(profile);

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