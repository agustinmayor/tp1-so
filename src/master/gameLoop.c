
#include "gameLoop.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/select.h>
#include <sys/wait.h>

#include "board.h"
#include "signals.h"
#include "utils.h"

void runGame(GameState * gs, GameSync * sync, const MasterArgs * args, int playersFds[], int * lastPlayerPlayed) {
    // Implementación pendiente del loop principal del juego
    (void)gs;
    (void)sync;
    (void)args;
    (void)playersFds;
    (void)lastPlayerPlayed;
}

// Imprime como termino un hijo: valor de retorno del exit, o señal que lo mato
static void printChildOutcome(const char * label, int status) {
    if(WIFEXITED(status)) {
        printf("%s termino satisfactoriamente (exit code: %d)", label, WEXITSTATUS(status));
    }
    else if(WIFSIGNALED(status)) {
        printf("%s termino por la señal %d", label, WTERMSIG(status));
    }
    else {
        printf("%s termino de forma inesperada", label);
    }
}

void gameOver(GameState * gs, GameSync * sync, const MasterArgs * args, pid_t playersPids[], int playersFds[], pid_t viewPid) {

    writerLock(sync);
    gs->isGameOver = true;
    writerUnlock(sync);

    // despierto a los jugadores que estuvieran esperando su turno para que vean que el juego termino
    for(int i = 0; i < args->cantPlayers; i++) {
        sem_post(&sync->playerTurn[i]);
    }

    // cierro los extremos de lectura antes de esperar: si un jugador quedara escribiendo
    // en un pipe lleno nunca terminaria y el master se colgaria en el waitpid
    for(int i = 0; i < args->cantPlayers; i++) {
        if(playersFds[i] >= 0) {
            close(playersFds[i]);
            playersFds[i] = -1;
        }
    }

    int status;

    if(args->hasView && viewPid > 0) {
        sem_post(&sync->viewSignal);
        sem_wait(&sync->viewUpdated);

        waitpid(viewPid, &status, 0);

        printChildOutcome("La vista", status);
        printf("\n");
    }

    for(int i = 0; i < args->cantPlayers; i++) {

        waitpid(playersPids[i], &status, 0);

        char label[64];
        snprintf(label, sizeof(label), "Jugador %s (%d)", gs->players[i].playerName, i);

        printChildOutcome(label, status);
        printf(" con un puntaje de %u / %u movimientos validos / %u movimientos invalidos\n",
            gs->players[i].playerScore, gs->players[i].validMoves, gs->players[i].invalidMoves);
    }
}
