
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

#define MILLISECONDS_PER_SECOND 1000
#define NANOSECONDS_PER_MILLISECOND 1000000L

static long millisecondsSince(const struct timespec * start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start->tv_sec) * MILLISECONDS_PER_SECOND
         + (now.tv_nsec - start->tv_nsec) / NANOSECONDS_PER_MILLISECOND;
}

static void sleepMilliseconds(unsigned int milliseconds) {
    struct timespec remaining = {
        .tv_sec = milliseconds / MILLISECONDS_PER_SECOND,
        .tv_nsec = (long)(milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
    };
    nanosleep(&remaining, NULL);
}

static void notifyView(GameSync * sync, const MasterArgs * args) {
    if(!args->hasView) {
        return;
    }
    sem_post(&sync->viewSignal);
    sem_wait(&sync->viewUpdated);
}

static void markEnclosedPlayersAsBlocked(GameState * gs) {
    for(int i = 0; i < gs->cantPlayers; i++) {
        Player * player = &gs->players[i];
        if(!player->isBlocked &&
           !hasFreeNeighbour(gs->board, gs->boardWidth, gs->boardHeight, player->playerX, player->playerY)) {
            player->isBlocked = true;
        }
    }
}

static bool everyPlayerIsBlocked(const GameState * gs) {
    for(int i = 0; i < gs->cantPlayers; i++) {
        if(!gs->players[i].isBlocked) {
            return false;
        }
    }
    return true;
}

static bool applyMove(GameState * gs, int playerIndex, unsigned char move) {
    Player * player = &gs->players[playerIndex];

    if(move >= DIRECTION_COUNT) {
        player->invalidMoves++;
        return false;
    }

    int targetX = (int)player->playerX + DIRECTION_DX[move];
    int targetY = (int)player->playerY + DIRECTION_DY[move];

    if(!isFreeCell(gs->board, gs->boardWidth, gs->boardHeight, targetX, targetY)) {
        player->invalidMoves++;
        return false;
    }

    player->playerScore += (unsigned int)cellAt(gs->board, gs->boardWidth, targetX, targetY);
    player->validMoves++;
    player->playerX = (unsigned short)targetX;
    player->playerY = (unsigned short)targetY;
    gs->board[(size_t)targetY * gs->boardWidth + (size_t)targetX] = (signed char)(-playerIndex);

    return true;
}

static void setPaused(GameState * gs, GameSync * sync, bool paused) {
    writerLock(sync);
    gs->isGamePaused = paused;
    writerUnlock(sync);
}

// Mientras el juego esta pausado el master no atiende movimientos: duerme en un pselect
// sin descriptores hasta que llegue la proxima señal. El tiempo pausado no se le descuenta
// al timeout, por eso al reanudar se corre hacia adelante la marca del ultimo movimiento.
static void waitWhilePaused(GameState * gs, GameSync * sync, const MasterArgs * args,
                            const sigset_t * runningMask, struct timespec * lastValidMove) {
    struct timespec pauseStart;
    clock_gettime(CLOCK_MONOTONIC, &pauseStart);

    setPaused(gs, sync, true);
    notifyView(sync, args);

    while(sigusr1Received == 0 && sigtermReceived == 0) {
        pselect(0, NULL, NULL, NULL, NULL, runningMask);
    }
    sigusr1Received = 0;

    setPaused(gs, sync, false);
    notifyView(sync, args);

    long pausedMilliseconds = millisecondsSince(&pauseStart);
    lastValidMove->tv_sec += pausedMilliseconds / MILLISECONDS_PER_SECOND;
    lastValidMove->tv_nsec += (pausedMilliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
    if(lastValidMove->tv_nsec >= NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND) {
        lastValidMove->tv_nsec -= NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
        lastValidMove->tv_sec++;
    }
}

void runGame(GameState * gs, GameSync * sync, const MasterArgs * args, int playersFds[], int * lastPlayerPlayed) {

    // Las señales quedan bloqueadas durante todo el loop y solo se habilitan dentro del pselect.
    // Asi no existe la ventana entre "reviso la bandera" y "me duermo" en la que se perderia una señal.
    sigset_t handledSignals, runningMask;
    sigemptyset(&handledSignals);
    sigaddset(&handledSignals, SIGTERM);
    sigaddset(&handledSignals, SIGUSR1);
    sigprocmask(SIG_BLOCK, &handledSignals, &runningMask);

    writerLock(sync);
    markEnclosedPlayersAsBlocked(gs);
    writerUnlock(sync);

    notifyView(sync, args);

    struct timespec lastValidMove;
    clock_gettime(CLOCK_MONOTONIC, &lastValidMove);

    bool gameFinished = false;

    while(!gameFinished && !sigtermReceived) {

        if(sigusr1Received) {
            sigusr1Received = 0;
            waitWhilePaused(gs, sync, args, &runningMask, &lastValidMove);
            continue;
        }

        long remainingMilliseconds = (long)args->timeout * MILLISECONDS_PER_SECOND - millisecondsSince(&lastValidMove);
        if(remainingMilliseconds <= 0) {
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        int maxFd = -1;

        for(int i = 0; i < args->cantPlayers; i++) {
            if(playersFds[i] < 0 || gs->players[i].isBlocked) {
                continue;
            }
            FD_SET(playersFds[i], &readSet);
            if(playersFds[i] > maxFd) {
                maxFd = playersFds[i];
            }
        }

        if(maxFd < 0) {
            break;
        }

        struct timespec waitTime = {
            .tv_sec = remainingMilliseconds / MILLISECONDS_PER_SECOND,
            .tv_nsec = (remainingMilliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
        };

        int readyPlayers = pselect(maxFd + 1, &readSet, NULL, NULL, &waitTime, &runningMask);

        if(readyPlayers == -1) {
            if(errno == EINTR) {
                continue;
            }
            perror("pselect: Error esperando los movimientos de los jugadores");
            break;
        }

        if(readyPlayers == 0) {
            break;
        }

        // Arrancamos por el siguiente al ultimo que jugo para que ninguno pueda
        // acaparar los turnos solo por tener el pipe listo antes que el resto.
        for(int offset = 0; offset < args->cantPlayers && !gameFinished; offset++) {
            int playerIndex = (*lastPlayerPlayed + 1 + offset) % args->cantPlayers;

            if(playersFds[playerIndex] < 0 || !FD_ISSET(playersFds[playerIndex], &readSet)) {
                continue;
            }

            unsigned char move;
            ssize_t readBytes = read(playersFds[playerIndex], &move, sizeof(move));

            if(readBytes != (ssize_t)sizeof(move)) {
                close(playersFds[playerIndex]);
                playersFds[playerIndex] = -1;

                writerLock(sync);
                gs->players[playerIndex].isBlocked = true;
                gameFinished = everyPlayerIsBlocked(gs);
                gs->isGameOver = gameFinished;
                writerUnlock(sync);
                continue;
            }

            *lastPlayerPlayed = playerIndex;

            writerLock(sync);
            bool moveWasValid = applyMove(gs, playerIndex, move);
            markEnclosedPlayersAsBlocked(gs);
            gameFinished = everyPlayerIsBlocked(gs);
            gs->isGameOver = gameFinished;
            writerUnlock(sync);

            if(moveWasValid) {
                clock_gettime(CLOCK_MONOTONIC, &lastValidMove);
            }

            sem_post(&sync->playerTurn[playerIndex]);

            // El ultimo cuadro lo manda gameOver junto con el aviso de fin de juego,
            // asi la vista no imprime dos veces el mismo tablero.
            if(!gameFinished) {
                notifyView(sync, args);
                if(args->hasView) {
                    sleepMilliseconds(args->delay);
                }
            }
        }
    }

    sigprocmask(SIG_SETMASK, &runningMask, NULL);
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
