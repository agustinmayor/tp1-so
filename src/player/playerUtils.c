#include "game_state.h"
#include "game_sync.h"
#include "playerUtils.h"
#include "sharedMem.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define EXPECTED_ARGC 4
#define CHECK_MASTER_SECONDS 1

// (fun heredada de player.c puesta aca para el bonus)

// chequeamos que el master siga vivo para no quedar bloqueados
// la espera la hacemos x tramos y entre tramo y tranmo chequeamos que el master siga vivo (siga siendo padre)
static bool waitForMyTurn(sem_t * playerTurn, pid_t masterPid) {
    while(1) {
        struct timespec deadline;

        clock_gettime(CLOCK_REALTIME, &deadline);

        deadline.tv_sec += CHECK_MASTER_SECONDS;

        if(sem_timedwait(playerTurn, &deadline) == 0){
            return true;
        }

        if(errno == ETIMEDOUT) {
            if(getppid() != masterPid) {
                return false;
            }

            continue;
        }

        if(errno != EINTR) {
            return false;
        }

    }

}

// usamos esto para no tener q copiar codigo entre player y playerBonus
int runPlayerLoop(int argc, char *argv[], chooseMoveFunction chooseMoveFn) {
    if(argc != EXPECTED_ARGC) {
        fprintf(stderr, "Uso: %s <width> <height> <playerIndex>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned short boardWidth = (unsigned short)strtoul(argv[1], NULL, 10);
    unsigned short boardHeight = (unsigned short)strtoul(argv[2], NULL, 10);
    int myIndex = (int)strtol(argv[3], NULL, 10);

    if(boardWidth == 0 || boardHeight == 0 || myIndex < 0 || myIndex >= MAX_PLAYERS) {
        fprintf(stderr, "Argumentos invalidos: %s <width> <height> <playerIndex>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // si el master cierra su extremo del pipe queremos que falle el write y no que nos mate una SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    size_t gameStateSize = sizeof(GameState) + ((size_t)boardWidth * boardHeight * sizeof(signed char));

    GameState * gs = connectSharedMem(SHM_GAME_STATE_NAME, gameStateSize, SHM_READ_ONLY);
    GameSync * sync = connectSharedMem(SHM_GAME_SYNC_NAME, sizeof(GameSync), SHM_READ_WRITE);

    // guardo una copia del estado del juego cuando voy a leerlo
    GameSnapshot snapshot;

    if(!createSnapshot(&snapshot, boardWidth, boardHeight)) {
        perror("malloc: Error al reservar la copia del tablero");
        disconnectSharedMem(sync, sizeof(GameSync));
        disconnectSharedMem(gs, gameStateSize);
        return EXIT_FAILURE;
    }

    pid_t masterPid = getppid();

    while(1) {
        if(!waitForMyTurn(&sync->playerTurn[myIndex], masterPid)) {
            break;
        }

        readerLock(sync);
        takeSnapshot(&snapshot, gs, myIndex);
        readerUnlock(sync);

        if(snapshot.isGameOver || snapshot.amIBlocked) {
            break;
        }

        // esta parte es la que cambia por el bonus
        // chooseMoveFn es segun el tipo de player
        unsigned char move = chooseMoveFn(&snapshot, myIndex, gs);

        if(write(STDOUT_FILENO, &move, sizeof(move)) != (ssize_t)sizeof(move)) {
            break;
        }
    }

    // si se termina el juego o me bloqueo o el master muere 
    disconnectSharedMem(sync, sizeof(GameSync));
    disconnectSharedMem(gs, gameStateSize);

    destroySnapshot(&snapshot);

    return EXIT_SUCCESS;
}

// Funciones para tomar snapshots de el estado del juego
// lo usa player controlado por IA
bool createSnapshot(GameSnapshot * snapshot, unsigned short boardWidth, unsigned short boardHeight) {
    snapshot->boardWidth = boardWidth;
    snapshot->boardHeight = boardHeight;
    snapshot->cantPlayers = 0;
    snapshot->isGameOver = false;
    snapshot->amIBlocked = false;
    snapshot->board = malloc((size_t)boardWidth * boardHeight * sizeof(signed char));

    return snapshot->board != NULL;
}

void destroySnapshot(GameSnapshot * snapshot) {
    free(snapshot->board);
    snapshot->board = NULL;
}

void takeSnapshot(GameSnapshot * snapshot, const GameState * gs, int myIndex) {
    snapshot->cantPlayers = gs->cantPlayers;
    snapshot->isGameOver = gs->isGameOver;
    snapshot->amIBlocked = gs->players[myIndex].isBlocked;

    for(unsigned char i = 0; i < gs->cantPlayers; i++) {
        snapshot->playersX[i] = gs->players[i].playerX;
        snapshot->playersY[i] = gs->players[i].playerY;
    }

    memcpy(snapshot->board, gs->board, (size_t)snapshot->boardWidth * snapshot->boardHeight);
}