#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "args.h"
#include "gameInit.h"
#include "gameLoop.h"
#include "game_state.h"
#include "game_sync.h"
#include "master.h"
#include "sharedMem.h"
#include "signals.h"

// Se guarda solo el nombre del binario para que la tabla de la vista sea legible
static const char * baseName(const char * path) {
    const char * lastSlash = strrchr(path, '/');
    return (lastSlash != NULL) ? lastSlash + 1 : path;
}

int main(int argc, char *argv[]) {

    MasterArgs args;
    parseArgs(argc, argv, &args);

    size_t gameStateSize = sizeof(GameState) + ((size_t)args.width * args.height * sizeof(signed char));
    size_t gameSyncSize = sizeof(GameSync);

    // --- Creamos la memoria compartida de game_state.h ---

    int shmFdGameState;
    GameState * gs = createSharedMem(SHM_GAME_STATE_NAME, gameStateSize, &shmFdGameState);

    // Inicializamos el estado del juego en cero
    memset(gs, 0, gameStateSize);

    // guardamos las dim del tablero, la cantidad de jugadores y las flags
    gs->boardWidth = args.width;
    gs->boardHeight = args.height;
    gs->cantPlayers = (unsigned char)args.cantPlayers;
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

    for(int i = 0; i < args.cantPlayers; i++) {
        strncpy(gs->players[i].playerName, baseName(args.playerPaths[i]), sizeof(gs->players[i].playerName) - 1);

        playersPids[i] = spawnPlayer(args.playerPaths[i], args.width, args.height, i, &playersFds[i]);
        gs->players[i].pid = playersPids[i];
    }

    // -----
    // spawn de la vista (si se especifico un path)
    // -----

    pid_t viewPid = -1;

    // si no se adjunto path de vista usando -v, no se hace spawn de vista
    if(args.hasView) {
        viewPid = spawnView(args.viewPath, args.width, args.height);
    }

    // -----
    // corremos el loop principal del juego
    // -----
    int lastPlayerPlayed = -1;
    runGame(gs, sync, &args, playersFds, &lastPlayerPlayed);

    // -----
    // terminamos el juego y limpiamos sync
    // -----
    gameOver(gs, sync, &args, playersPids, playersFds, viewPid);

    // -----
    // limpiamos la memoria compartida y de los recursos IPC
    // -----

    semDestroy(sync, args.cantPlayers);
    removeSharedMem(SHM_GAME_SYNC_NAME, shmFdGameSync, sync, gameSyncSize);
    removeSharedMem(SHM_GAME_STATE_NAME, shmFdGameState, gs, gameStateSize);

    return 0;
}
