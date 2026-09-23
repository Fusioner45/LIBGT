#define LIBGT_IMPLEMENTATION
#include "libGT.h"
#include <stdio.h>

int main(void) {
    printf("--- Lancement du banc de test complet libGT ---\n");

    GtWindow* win = gtCreateWindow("Banc de Test libGT - Validation Robustesse", 960, 600);
    if (!win) {
        fprintf(stderr, "Erreur d'initialisation de la fenetre.\n");
        return 1;
    }

    // Position du joueur pour tester WASD + flèches
    int player_x = 480;
    int player_y = 300;
    int speed = 3;

    while (gtEventsWindow(win)) {
        // =====================================================================
        // 1. FERMETURE DE FENÊTRE & ENTRÉES CLAVIER (WASD + Flèches)
        // =====================================================================
        if (gtIsKeyDown(win, GT_KEY_ESCAPE)) break;

        if (gtIsKeyDown(win, GT_KEY_D) || gtIsKeyDown(win, GT_KEY_RIGHT)) player_x += speed;
        if (gtIsKeyDown(win, GT_KEY_A) || gtIsKeyDown(win, GT_KEY_LEFT))  player_x -= speed;
        if (gtIsKeyDown(win, GT_KEY_S) || gtIsKeyDown(win, GT_KEY_DOWN))  player_y += speed;
        if (gtIsKeyDown(win, GT_KEY_W) || gtIsKeyDown(win, GT_KEY_UP))    player_y -= speed;

        // Effacement de l'écran
        gtClearWindow(win, GT_DARKGRAY);

        // =====================================================================
        // 2. TESTS RECTANGLES
        // =====================================================================
        // Rectangle normal
        gtDrawRect(win, 50, 50, 100, 60, GT_GREEN);
        
        // Rectangle partiellement hors écran (dépasse à gauche)
        gtDrawRect(win, -40, 150, 100, 60, GT_RED);
        
        // Rectangle complètement hors écran (ne doit pas crasher ni dessiner)
        gtDrawRect(win, -500, -500, 200, 200, GT_YELLOW);
        gtDrawRect(win, 2000, 2000, 200, 200, GT_YELLOW);

        // =====================================================================
        // 3. TESTS LIGNES
        // =====================================================================
        // Ligne diagonale classique
        gtDrawLine(win, 200, 50, 350, 200, GT_BLUE);

        // Ligne qui traverse TOUT l'écran (de très loin hors champ à très loin hors champ)
        gtDrawLine(win, -1000, -500, 2000, 1200, GT_YELLOW);

        // Ligne entièrement hors écran
        gtDrawLine(win, -2000, -1000, -500, -200, GT_RED);
        gtDrawLine(win, 3000, 3000, 4000, 4000, GT_RED);

        // =====================================================================
        // 4. TESTS CERCLES
        // =====================================================================
        // Cercle normal (plein et contour)
        gtDrawCircle(win, 700, 100, 40, GT_BLUE);
        gtDrawCircleLines(win, 820, 100, 40, GT_GREEN);

        // Cercle coupé par le bord (centre en (0, 0))
        gtDrawCircle(win, 0, 0, 80, GT_RED);
        gtDrawCircleLines(win, 960, 0, 80, GT_YELLOW);

        // Cercle complètement hors écran
        gtDrawCircle(win, -5000, -5000, 300, GT_WHITE);
        gtDrawCircleLines(win, 5000, 5000, 300, GT_WHITE);

        // Cercle énorme (rayon immense, englobant largement l'écran)
        gtDrawCircleLines(win, 480, 300, 5000, GT_WHITE);

        // =====================================================================
        // 5. COORDONNÉES NÉGATIVES GÉNÉRALES & RENDER JOUEUR
        // =====================================================================
        // Dessin d'un contour avec origine négative
        gtDrawRectLines(win, -20, -20, 120, 120, GT_WHITE);

        // Joueur contrôlé au clavier
        gtDrawRect(win, player_x, player_y, 40, 40, GT_RED);

        // =====================================================================
        // 6. SOURIS (Clics Gauche / Droit / Milieu) & PERTE DE FOCUS
        // =====================================================================
        int mx, my;
        gtGetMousePos(win, &mx, &my);

        if (gtIsMouseButtonDown(win, GT_MOUSE_BUTTON_LEFT)) {
            gtDrawCircle(win, mx, my, 15, GT_YELLOW);
        }
        if (gtIsMouseButtonDown(win, GT_MOUSE_BUTTON_RIGHT)) {
            gtDrawRect(win, mx - 15, my - 15, 30, 30, GT_RED);
        }
        if (gtIsMouseButtonDown(win, GT_MOUSE_BUTTON_MIDDLE)) {
            gtDrawCircleLines(win, mx, my, 25, GT_WHITE);
        }

        // Mise à jour du rendu à l'écran
        gtUpdateWindow(win);
    }

    gtDestroyWindow(win);
    printf("Fermeture propre effectuee.\n");
    return 0;
}