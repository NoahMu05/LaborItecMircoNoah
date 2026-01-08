#include "game.h"
#include <stdio.h>

// ersetzt später die vollständige Spiel‑Logik.
int play_game_from_profile(const char *csvfile){
    if(!csvfile){
        fprintf(stderr, "play_game_from_profile: kein csvfile angegeben\n");
        return -1;
    }
    printf("Spiel gestartet (Stub). Profil: %s\n", csvfile);
    printf("Implementiere game.c später für das vollständige Spiel.\n");
    return 0;
}
