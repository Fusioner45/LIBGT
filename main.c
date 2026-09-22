#define MY_GFX_IMPLEMENTATION
#include "my_gfx.h"
#include <stdio.h>

int main(void) {
    printf("Lancement de la fenetre...\n");

    MyWindow* win = my_window_create("Mon Moteur Graphique 2D", 800, 600);
    if (!win) {
        fprintf(stderr, "Erreur lors de la creation de la fenetre.\n");
        return 1;
    }

    // Boucle principale de l'application
    while (my_window_poll_events(win)) {
        // Ici, on mettra le rendu plus tard
    }

    my_window_destroy(win);
    printf("Fermeture propre.\n");
    return 0;
}