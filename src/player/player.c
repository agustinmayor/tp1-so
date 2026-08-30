#include <game_state.h>
#include <game_sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <playerAI.h>
#include <semaphores.h>
#include <unistd.h>


int main(argc, char *argv[]){
    if(argc != 3) {
        fprintf(stderr, "Uso: %s  <width> <height>\n", argv[0]);
        return 1;
    }
    size_t width, height;
    int checkResult = checkArguments(argv, &width, &height);

    if(checkResult == -1){
        return 1;
    }

    int syncFd = -1;
    int stateFd = -1;

    SyncData *sync = NULL;
    GameState *state = NULL;

    if(syncOpen(&syncFd, &sync) == -1){
        perror("Error en syncOpen.");
        return 1;
    }

    if(stateOpen(&stateFd, &state, width, height) == -1){
        perror("Error en stateOpen.");
        syncClose(syncFd, sync);
        return 1;
    }

    int myIndex = findMyIndex(state);

    if(myIndex == -1){
        fprintf(stderr, "Error: Jugador no encontrado en el estado del juego.\n");
        stateClose(stateFd, state, width, height);
        syncClose(syncFd, sync);
        return 1;
    }

    /* Loop principal del jugador. */
    runLoop(state, sync, myIndex);

    /* Estado posterior a la finalización del juego. */
    syncClose(syncFd, sync);
    stateClose(stateFd, state, width, height);

    return 0;
}
