#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "game_sync.h"
#include "game_state.h"
#include "master.h"
#include "signals.h"

int main(int argc, char *argv[]) {

    MasterArgs args;
    parseArgs(argc, argv, &args);

    size_t gameStateSize = sizeof(GameState) + (args.width * args.height * sizeof(signed char));
    size_t gameSyncSize = sizeof(GameSync);

    // --- Creamos la memoria compartida de game_state.h ---

    int shmFdGameState;
    GameState * gs = createSharedMem(SHM_GAME_STATE_NAME, gameStateSize, &shmFdGameState);
    
    // Inicializamos el estado del juego en cero
    memset(gs, 0, gameStateSize);

    // guardamos las dim del tablero, la cantidad de jugadores t las flags
    gs->boardWidth = args.width;
    gs->boardHeight = args.height;
    gs->cantPlayers = args.cantPlayers;
    gs->isGameOver = false;
    gs->isGamePaused = false;


    // --- Creamos la memoria compartida de game_sync.h ---

    int shmFdGameSync;
    GameSync * sync = createSharedMem(SHM_GAME_SYNC_NAME, gameSyncSize, &shmFdGameSync);
    memset(sync, 0, gameSyncSize);

    // Inicializamos tablero, ubicamos jugadores, inicializamos semaforos y señales
    initBoard(gs, args.seed);
    locatePlayers(gs);
    semInit(sync, args.cantPlayers);
    initSignalHandlers();


    // -----
    // spawn de los jugadores 
    // -----
    
    pid_t playersPids[MAX_PLAYERS];
    int playersFds[MAX_PLAYERS]; // extremos de lectura del pipe para cada player

    for(int i=0; i < args.cantPlayers; i++) {
        playersPids[i] = spawnPlayer(args.playerPaths[i], args.width, args.height, i, &playersFds[i]);
        gs->players[i].pid = playersPids[i];

        strncpy(gs->players[i].playerName, args.playerPaths[i], 15);
    }

    // -----
    // spawn de la vista (si se especifico un path)
    // -----

    pid_t viewPid = -1;

    // si no se adjunto path de vista usando -v, no se hace spawn de vista
    if(args.hasView) {
        viewPid = spawnView(args.viewPath, args.width, args.height);
    }

    // falta la implementacion de iniciar, terminar el juego y limpiar recursos
    // ...

    return 0;
}

static void parseArgs(int argc, char *argv[], MasterArgs *args) {
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
