#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "game_sync.h"
#include "game_state.h"
#include "master.h"

int main(int argc, char *argv[]) {

    MasterArgs args;
    parseArgs(argc, argv, &args);

    // falta inicializar las dos memorias compartidas y los semaforos

    // falta el manejo de las señales de fin y de pausa

    // falta la logica del master para manejar el juego 

    // falta la implementacion de terminar el juego y limpiar recursos
    // ...

    return 0;
}

void parseArgs(int argc, char *argv[], MasterArgs *args) {
    args->width = DEFAULT_WIDTH;
    args->height = DEFAULT_HEIGHT;
    args->delay = DEFAULT_DELAY;
    args->timeout = DEFAULT_TIMEOUT;
    args->seed = (unsigned int)time(NULL);
    args->hasView = false;
    args->cantPlayers = 0;

    int opt;

    while((opt = getopt(argc, argv, "w:h:d:t:s:v:p:i")) != -1) {
        switch(opt) {
            case 'w': args->width = (unsigned short)atoi(optarg); break;
            case 'h': args->height = (unsigned short)atoi(optarg); break;
            case 'd': args->delay = (unsigned int)atoi(optarg); break;
            case 't': args->timeout = (unsigned int)atoi(optarg); break;
            case 's': args->seed = (unsigned int)atoi(optarg); break;
            case 'v':
                strncpy(args->viewPath, optarg, sizeof(args->viewPath) - 1);
                args->hasView = true;
                break;
            case 'p':
                args->playerPaths[args->cantPlayers++] = optarg;
                while (optind < argc && argv[optind][0] != '-') {
                    args->playerPaths[args->cantPlayers++] = argv[optind];
                    optind++;
                }
                break;
            default:
                fprintf(stderr, "El formato debe ser: %s [-w width][-h height][-d delay][-t timeout][-s seed][-v vista] -p jugador1 [jugador2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(args->width < DEFAULT_WIDTH) {
        args->width = DEFAULT_WIDTH;
    }
    if(args->height < DEFAULT_HEIGHT) {
        args->height = DEFAULT_HEIGHT;
    }
    if(args->cantPlayers <= 0 || args->cantPlayers > MAX_PLAYERS) {
        fprintf(stderr, "Cantidad de jugadores invalida. Debe ser entre 1 y 9\n");
        exit(EXIT_FAILURE);
    }
}