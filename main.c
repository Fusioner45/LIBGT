#define LIBGT_IMPLEMENTATION
#include "libGT.h"
#include <stdio.h>

int main(void) {
    printf("Lancement de la fenetre...\n");

    MyWindow* win = createWin("Mon Moteur Graphique 2D", 960, 600);
    if (!win) {
        fprintf(stderr, "Erreur lors de la creation de la fenetre.\n");
        return 1;
    }

    int player_x = 400;
    int player_y = 250;
    int speed = 2;

    // Boucle principale d'affichage
    while (eventsWin(win)) {
        // Quitter avec Échap
        if (isKeyDown(win, GT_KEY_ESCAPE)) break;

        // Déplacement Clavier (ZQSD / Fleches)
        if (isKeyDown(win, GT_KEY_D) || isKeyDown(win, GT_KEY_RIGHT)) player_x += speed;
        if (isKeyDown(win, GT_KEY_A) || isKeyDown(win, GT_KEY_LEFT))  player_x -= speed;
        if (isKeyDown(win, GT_KEY_S) || isKeyDown(win, GT_KEY_DOWN))  player_y += speed;
        if (isKeyDown(win, GT_KEY_W) || isKeyDown(win, GT_KEY_UP))    player_y -= speed;

        // Rendu
        clearWin(win, 0x000F172A);

        // Dessiner le joueur
        drawRect(win, player_x, player_y, 50, 50, GT_RED);

        // Dessiner un petit carré sous le curseur si le clic gauche est maintenu
        if (isMouseButtonDown(win, GT_MOUSE_BUTTON_LEFT)) {
            int mx, my;
            getMousePos(win, &mx, &my);
            drawRect(win, mx - 10, my - 10, 20, 20, GT_BLUE);
        }

        updateWin(win);
    }

    destroyWin(win);
    printf("Fermeture propre.\n");
    return 0;
}