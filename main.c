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

    int rect_x = 100;

    // Boucle principale d'affichage
    while (eventsWin(win)) {
        // 1. Effacer le fond avec un bleu foncé (0x000F172A)
        clearWin(win, 0x000F172A);

        // 2. Dessiner un rectangle rouge/rose (0x00F43F5E) qui se déplace
        drawRect(win, rect_x, 200, 150, 100, 0x00F43F5E);
        
        // Fait défiler le rectangle vers la droite
        rect_x += 2;
        if (rect_x > win->width) rect_x = -150;

        // 3. Envoyer le buffer à l'écran
        updateWin(win);
    }

    destroyWin(win);
    printf("Fermeture propre.\n");
    return 0;
}
