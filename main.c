#define LIBGT_IMPLEMENTATION
#include "libGT.h"
#include <stdio.h>

int main(void) {
    printf("Lancement de la fenetre...\n");

    MyWindow* win = createWin("Mon Moteur Graphique 2D", 900, 700);
    if (!win) {
        fprintf(stderr, "Erreur lors de la creation de la fenetre.\n");
        return 1;
    }

    // Boucle principale de l'application
    while (eventsWin(win)) {
        // Ici, on mettra le rendu plus tard
    }

    destroyWin(win);
    printf("Fermeture propre.\n");
    return 0;
}