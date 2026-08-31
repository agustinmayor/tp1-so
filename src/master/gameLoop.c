
#include "gameLoop.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/select.h>
#include <sys/wait.h>

void runGame(GameState * gs, GameSync * sync, const MasterArgs * args, int playersFds[], int * lastPlayerPlayed) {
    // Implementación pendiente del loop principal del juego
}

void gameOver(GameState * gs, GameSync * sync, const MasterArgs * args, pid_t playersPids[], int playersFds[], pid_t viewPid) {

    gs->isGameOver = true;

    // notifico a los jugadores que el juego termino

    for(int i = 0; i < args->cantPlayers; i++) {
        sem_post(&sync->playerTurn[i]); // libero el semaforo del jugador para que pueda leer el estado y ver que el juego termino
    }

    // guardo el return del hijo en el waitpid
    int status;

    if(args->hasView && viewPid > 0) {
        sem_post(&sync->viewSignal);

        waitpid(viewPid, &status, 0);

        printf("La vista termino satisfactoriamente (exit code: %d)\n", WEXITSTATUS(status));
    }


    for(int j = 0; j < args->cantPlayers; j++) {

        waitpid(playersPids[j], &status, 0);

        if(WIFEXITED(status)) { // termino normalmente
            printf("Jugador %s (ID: %d) termino satisfactoriamente (exit code: %d) con un score de %u / %u / %u\n", 
                gs->players[j].playerName, j, WEXITSTATUS(status), gs->players[j].playerScore, gs->players[j].validMoves, gs->players[j].invalidMoves);
        }
        else if(WIFSIGNALED(status)) { // termino con señal
            printf("Jugador %s (ID: %d) termino con señal %d\n",
                gs->players[j].playerName, j, WTERMSIG(status));
        }

        close(playersFds[j]);
    }

    
}