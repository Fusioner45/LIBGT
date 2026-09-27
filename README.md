# libGT

## Français

libGT est une petite bibliothèque graphique 2D en C11 pour Windows, conçue comme projet d'apprentissage et de prototypage de jeux. Elle fournit une fenêtre Win32, un framebuffer CPU, le rendu 2D, la gestion des entrées, des timers, le chargement d'images, l'affichage de texte, une abstraction de renderer et un ECS léger.

> Statut : expérimental / en développement. Le backend actuel est GDI. Direct2D est prévu pour plus tard et utilise actuellement un fallback GDI.

### Fonctionnalités

- Création de fenêtre Win32, polling des événements, redimensionnement et fermeture
- Framebuffer CPU au format ARGB : `0xAARRGGBB`
- Primitives 2D avec clipping : pixels, rectangles pleins et contours, lignes et cercles
- Alpha blending et couleurs prédéfinies (`GT_RED`, `GT_GREEN`, etc.)
- Outils `GtVec2` : addition, soustraction, produit scalaire, longueur, normalisation et lerp
- Temps haute résolution via `QueryPerformanceCounter`
- Timers pilotés par le temps du jeu : timeouts et intervalles
- États clavier/souris et détection des pressions/relâchements
- Support des manettes XInput chargé dynamiquement
- Action maps, rebinding, profils joueurs et sauvegarde de profils
- Police bitmap ASCII intégrée de 8×8, mesure et retour à la ligne
- Chargement d'images via l'implémentation incluse de `stb_image`
- ECS léger avec transforms, sprites, vitesses, colliders, santé, durée de vie, tags et systèmes
- Licence MIT

### Prérequis

- Windows
- Un compilateur compatible C11 (MinGW GCC, Clang ou MSVC)
- Les bibliothèques du SDK Windows : `user32` et `gdi32`
- `stb_image.h` doit être disponible à côté de `libGT.h` si le chargement d'images est compilé

La bibliothèque est actuellement limitée à Windows, car ses fenêtres et son rendu utilisent Win32/GDI. L'application n'a pas besoin de lier directement une DLL XInput : libGT recherche dynamiquement une DLL disponible. Une manette reste facultative.

### Structure du dépôt

| Fichier | Rôle |
|---|---|
| `libGT.h` | API publique et implémentation de la bibliothèque |
| `main.c` | Démonstration interactive des fenêtres, dessins, clavier, souris et clipping |
| `smoke_test.c` | Test automatisé de l'API : maths, couleurs, temps, timers, ECS, rendu, input, sérialisation et sécurité NULL |
| `LICENSE` | Licence MIT |

### Démarrage rapide

libGT est une bibliothèque header-only. Définissez `LIBGT_IMPLEMENTATION` dans un seul fichier C :

```c
#define LIBGT_IMPLEMENTATION
#include "libGT.h"

int main(void) {
    GtWindow *window = gtCreateWindow("Exemple libGT", 800, 600);
    if (!window) return 1;

    while (gtEventsWindow(window)) {
        gtBeginFrame();
        float dt = gtGetDeltaTime();
        (void)dt;

        gtClearWindow(window, GT_DARKGRAY);
        gtDrawRect(window, 100, 100, 160, 80, GT_GREEN);
        gtDrawCircle(window, 400, 300, 50, GT_BLUE);
        gtDrawText(window, 20, 20, "Bonjour libGT", GT_WHITE);
        gtUpdateWindow(window);

        if (gtIsKeyDown(window, GT_KEY_ESCAPE)) break;
    }

    gtDestroyWindow(window);
    return 0;
}
```

### Compilation

Avec MinGW GCC :

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -o main.exe -lgdi32 -luser32
main.exe
```

Pour compiler le smoke test :

```bash
gcc -std=c11 -Wall -Wextra -O2 smoke_test.c -o smoke_test.exe -lgdi32 -luser32
smoke_test.exe
```

Le smoke test ouvre brièvement une fenêtre et crée `test_profile.bin` dans le dossier courant. Il renvoie un code de sortie différent de zéro si un test échoue.

Avec MSVC, liez `user32.lib` et `gdi32.lib` :

```bat
cl /std:c11 /W4 /O2 main.c user32.lib gdi32.lib
```

### Boucle principale

Une frame suit généralement cet ordre :

```c
gtBeginFrame();
gtEventsWindow(window);
gtInputUpdate(input, window, gtGetDeltaTime());
gtTimersUpdate(gtGetDeltaTime());

/* logique du jeu et dessin */

gtUpdateWindow(window);
```

Appelez `gtBeginFrame()` une fois par frame et `gtEventsWindow()` avant de lire les entrées. `gtTimersUpdate()` utilise le delta time du jeu : les timers s'arrêtent naturellement si vous cessez de les mettre à jour.

### Abstraction de rendu

```c
GtRenderer *renderer = gtCreateRenderer(window, GT_RENDERER_GDI);
gtRendererBegin(renderer);
gtRendererClear(renderer, GT_BLACK);
gtRendererDrawRect(renderer, 50, 50, 100, 100, GT_RED);
gtRendererEnd(renderer);
gtDestroyRenderer(renderer);
```

`GT_RENDERER_D2D` est accepté, mais utilise actuellement un fallback GDI.

### Input mapping

Les actions sont séparées des contrôles physiques :

```c
GtInputSystem *input = gtInputCreate();
GtPlayerProfile *profile = gtInputGetProfile(input, 0);
GtActionMap *gameplay = gtInputCreateMap("gameplay");

gtInputAddAction(gameplay, "jump");
gtInputBindKey(gameplay, "jump", GT_KEY_SPACE);
gtInputPushMap(profile, gameplay);

gtInputUpdate(input, window, gtGetDeltaTime());
if (gtInputWasActionPressed(input, 0, "jump")) {
    /* sauter une seule fois */
}

gtInputDestroyMap(gameplay);
gtInputDestroy(input);
```

Les bindings clavier, souris, boutons de manette et axes de manette sont pris en charge. Jusqu'à quatre profils joueurs sont disponibles. Les profils utilisent le format binaire actuel de libGT : considérez-les comme des données de votre application et ne supposez pas une compatibilité avec les futures versions.

### Aperçu de l'ECS

```c
GtECS ecs = {0};
gtECSInit(&ecs, 256);
GtEntity entity = gtECSCreateEntity(&ecs);
gtECSAddTransform(&ecs, entity, gtVec2(100, 100), 0.0f, 1.0f);
gtECSAddVelocity(&ecs, entity, gtVec2(40, 0), 0.0f);

gtECSSystemVelocity(&ecs, gtGetDeltaTime());
gtECSDestroy(&ecs);
```

Les masques intégrés incluent `GT_COMP_TRANSFORM`, `GT_COMP_SPRITE`, `GT_COMP_VELOCITY`, `GT_COMP_COLLIDER`, `GT_COMP_HEALTH`, `GT_COMP_LIFETIME`, `GT_COMP_PLAYER` et `GT_COMP_ENEMY`. Les collisions utilisent actuellement une vérification brute par paires (`O(n²)`) et appellent une fonction de rappel ; aucune réponse physique automatique n'est appliquée.

### Points importants

- Incluez `libGT.h` normalement dans les autres fichiers, mais définissez `LIBGT_IMPLEMENTATION` une seule fois.
- `GtWindow` et `GtRenderer` sont opaques : créez-les et détruisez-les avec l'API.
- L'origine est `(0, 0)` en haut à gauche de la zone cliente ; Y augmente vers le bas.
- Les couleurs utilisent `0xAARRGGBB` ; utilisez `gtColorRGB(r, g, b)` et `gtColorRGBA(r, g, b, a)`.
- Le système de sprites ECS dessine actuellement des rectangles colorés placeholder. Le rendu direct d'images est disponible via les fonctions image du renderer.
- `gtRendererDrawImageEx` effectue actuellement un redimensionnement nearest-neighbor ; rotation, origine et miroir sont réservés pour une future implémentation.

### Tests et contributions

Lancez `smoke_test.c` avant de proposer une modification. Pour signaler un bug ou proposer une fonctionnalité, ouvrez une issue et mettez à jour les exemples et commentaires de l'API lorsque le comportement public change.

### Licence

Copyright (c) 2026 Fusioner45.

Distribué sous licence MIT. Voir le fichier `LICENSE`.

---

## English

libGT is a small **2D graphics library for Windows in C11**, designed as a learning and game-prototyping project. It provides a Win32 window, a CPU framebuffer, 2D rendering, input handling, timers, image loading, text rendering, a renderer abstraction layer, and a lightweight ECS.

> Status: experimental / in active development. The current backend is GDI. Direct2D is planned for later and currently falls back to GDI.

### Features

- Win32 window creation, event polling, resizing, and shutdown handling
- CPU framebuffer using the ARGB format: `0xAARRGGBB`
- 2D primitives with clipping: pixels, filled and outlined rectangles, lines, and circles
- Alpha blending and predefined colors (`GT_RED`, `GT_GREEN`, etc.)
- `GtVec2` helpers: add, subtract, dot product, length, normalize, and lerp
- High-resolution timing via `QueryPerformanceCounter`
- Game-timed timers: one-shot timeouts and repeating intervals
- Keyboard and mouse states plus pressed/released edge detection
- XInput gamepad support loaded dynamically at runtime
- Action maps, rebinding, player profiles, and profile serialization
- Built-in 8×8 bitmap font with measurement and wrapping helpers
- Image loading via the bundled `stb_image` implementation
- Lightweight ECS with transforms, sprites, velocities, colliders, health, lifetimes, tags, and systems
- MIT licensed

### Requirements

- Windows
- A C11-compatible compiler (MinGW GCC, Clang, or MSVC)
- The Windows SDK libraries: `user32` and `gdi32`
- `stb_image.h` must be available beside `libGT.h` when image loading is compiled

This library is Windows-only because it uses Win32 and GDI for its windowing and rendering. It does not require linking a direct XInput DLL: libGT searches for an available XInput DLL dynamically. A controller remains optional.

### Repository structure

| File | Purpose |
|---|---|
| `libGT.h` | Public API and implementation |
| `main.c` | Interactive example using windows, drawing, keyboard, mouse, and clipping |
| `smoke_test.c` | Automated API smoke test covering math, colors, timing, timers, ECS, rendering, input, serialization, and NULL safety |
| `LICENSE` | MIT license |

### Quick start

libGT is a header-only library. Define `LIBGT_IMPLEMENTATION` in one C file only:

```c
#define LIBGT_IMPLEMENTATION
#include "libGT.h"

int main(void) {
    GtWindow *window = gtCreateWindow("libGT example", 800, 600);
    if (!window) return 1;

    while (gtEventsWindow(window)) {
        gtBeginFrame();
        float dt = gtGetDeltaTime();
        (void)dt;

        gtClearWindow(window, GT_DARKGRAY);
        gtDrawRect(window, 100, 100, 160, 80, GT_GREEN);
        gtDrawCircle(window, 400, 300, 50, GT_BLUE);
        gtDrawText(window, 20, 20, "Hello from libGT", GT_WHITE);
        gtUpdateWindow(window);

        if (gtIsKeyDown(window, GT_KEY_ESCAPE)) break;
    }

    gtDestroyWindow(window);
    return 0;
}
```

### Build

Using MinGW GCC:

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -o main.exe -lgdi32 -luser32
main.exe
```

For the smoke test:

```bash
gcc -std=c11 -Wall -Wextra -O2 smoke_test.c -o smoke_test.exe -lgdi32 -luser32
smoke_test.exe
```

The smoke test opens a window briefly and creates `test_profile.bin` in the working directory. It exits with a non-zero status if any check fails.

With MSVC, link `user32.lib` and `gdi32.lib`:

```bat
cl /std:c11 /W4 /O2 main.c user32.lib gdi32.lib
```

### Main loop

A typical frame follows this order:

```c
gtBeginFrame();
gtEventsWindow(window);
gtInputUpdate(input, window, gtGetDeltaTime());
gtTimersUpdate(gtGetDeltaTime());

/* game logic and drawing */

gtUpdateWindow(window);
```

Call `gtBeginFrame()` once per frame, then `gtEventsWindow()` before reading input. `gtTimersUpdate()` uses game delta time, so timers naturally stop updating when you stop driving the game loop.

### Renderer abstraction

```c
GtRenderer *renderer = gtCreateRenderer(window, GT_RENDERER_GDI);
gtRendererBegin(renderer);
gtRendererClear(renderer, GT_BLACK);
gtRendererDrawRect(renderer, 50, 50, 100, 100, GT_RED);
gtRendererEnd(renderer);
gtDestroyRenderer(renderer);
```

`GT_RENDERER_D2D` is accepted but currently falls back to GDI.

### Input mapping

Actions are separated from physical controls:

```c
GtInputSystem *input = gtInputCreate();
GtPlayerProfile *profile = gtInputGetProfile(input, 0);
GtActionMap *gameplay = gtInputCreateMap("gameplay");

gtInputAddAction(gameplay, "jump");
gtInputBindKey(gameplay, "jump", GT_KEY_SPACE);
gtInputPushMap(profile, gameplay);

gtInputUpdate(input, window, gtGetDeltaTime());
if (gtInputWasActionPressed(input, 0, "jump")) {
    /* jump once */
}

gtInputDestroyMap(gameplay);
gtInputDestroy(input);
```

Keyboard, mouse, gamepad buttons, and gamepad axes are supported. Up to four player profiles are available. Profile data uses libGT's current binary format; treat it as application data and do not assume compatibility across future versions.

### ECS overview

```c
GtECS ecs = {0};
gtECSInit(&ecs, 256);
GtEntity entity = gtECSCreateEntity(&ecs);
gtECSAddTransform(&ecs, entity, gtVec2(100, 100), 0.0f, 1.0f);
gtECSAddVelocity(&ecs, entity, gtVec2(40, 0), 0.0f);

gtECSSystemVelocity(&ecs, gtGetDeltaTime());
gtECSDestroy(&ecs);
```

Built-in component masks include `GT_COMP_TRANSFORM`, `GT_COMP_SPRITE`, `GT_COMP_VELOCITY`, `GT_COMP_COLLIDER`, `GT_COMP_HEALTH`, `GT_COMP_LIFETIME`, `GT_COMP_PLAYER`, and `GT_COMP_ENEMY`. Collision checks currently use brute-force pair detection (`O(n²)`) and report hits through a callback; they do not provide automatic physical response.

### Important notes

- Include `libGT.h` in the usual C files, but define `LIBGT_IMPLEMENTATION` only once.
- `GtWindow` and `GtRenderer` are opaque objects; create and destroy them via the API.
- Coordinates use `(0, 0)` at the top-left of the client area; Y increases downward.
- Color values use `0xAARRGGBB`; constructors are `gtColorRGB(r, g, b)` and `gtColorRGBA(r, g, b, a)`.
- The ECS sprite system currently draws color rectangle placeholders. Direct image rendering is available through the renderer image functions.
- `gtRendererDrawImageEx` currently performs nearest-neighbor scaling; rotation, origin, and mirroring are reserved for future implementation.

### Testing and contributions

Run `smoke_test.c` before submitting changes. To report a bug or suggest a feature, open an issue and keep examples and API comments updated whenever public behavior changes.

### License

Copyright (c) 2026 Fusioner45.

Distributed under the MIT License. See `LICENSE` for details.
