#include "game_state.h"
#include "game_sync.h"
#include "playerAI.h"
#include "sharedMem.h"
#include "utils.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define EXPECTED_ARGC 4

int main(int argc, char * argv[]) {

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

    GameSnapshot snapshot;

    if(!createSnapshot(&snapshot, boardWidth, boardHeight)) {
        perror("malloc: Error al reservar la copia del tablero");
        disconnectSharedMem(sync, sizeof(GameSync));
        disconnectSharedMem(gs, gameStateSize);
        return EXIT_FAILURE;
    }

    FOR_EVER {
        if(sem_wait(&sync->playerTurn[myIndex]) == -1) {
            if(errno == EINTR) {
                continue;
            }
            break;
        }

        readerLock(sync);
        takeSnapshot(&snapshot, gs, myIndex);
        readerUnlock(sync);

        if(snapshot.isGameOver || snapshot.amIBlocked) {
            break;
        }

        unsigned char move = chooseMove(&snapshot, myIndex);

        if(write(STDOUT_FILENO, &move, sizeof(move)) != (ssize_t)sizeof(move)) {
            break;
        }
    }

    destroySnapshot(&snapshot);
    disconnectSharedMem(sync, sizeof(GameSync));
    disconnectSharedMem(gs, gameStateSize);

    return EXIT_SUCCESS;
}
