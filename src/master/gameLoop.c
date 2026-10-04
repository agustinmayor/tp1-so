
#include "gameLoop.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include <sys/select.h>
#include <sys/wait.h>

#include "board.h"
#include "replay.h"
#include "signals.h"

#define MILLISECONDS_PER_SECOND 1000
#define NANOSECONDS_PER_MILLISECOND 1000000L

static long millisecondsSince(const struct timespec * start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ((now.tv_sec - start->tv_sec) * MILLISECONDS_PER_SECOND)
         + ((now.tv_nsec - start->tv_nsec) / NANOSECONDS_PER_MILLISECOND);
}

static void sleepMilliseconds(unsigned int milliseconds) {
    struct timespec remaining = {
        .tv_sec = milliseconds / MILLISECONDS_PER_SECOND,
        .tv_nsec = (long)(milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
    };
    nanosleep(&remaining, NULL);
}

// Los handlers solo corren con las señales desbloqueadas. Como el master las mantiene
// bloqueadas mientras trabaja, y el pselect no llega a dormirse cuando los jugadores
// siempre tienen un movimiento listo, abrimos una ventana en cada vuelta para que las
// señales que quedaron pendientes se entreguen ahora.
static void deliverPendingSignals(const sigset_t * runningMask, const sigset_t * handledSignals) {
    sigprocmask(SIG_SETMASK, runningMask, NULL);
    sigprocmask(SIG_BLOCK, handledSignals, NULL);
}

// le mando a la vista para que actualice el board
void notifyView(GameSync * sync, const MasterArgs * args) {
    if(!args->hasView) {
        return;
    }
    sem_post(&sync->viewSignal);
    sem_wait(&sync->viewUpdated);
}

// marco a los players encerrados como que estan bloqueados
//      hasFreeNeighbour esta en board.c y me devuelve si tiene alguna celda libre o no
void markEnclosedPlayersAsBlocked(GameState * gs) {
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

bool applyMove(GameState * gs, int playerIndex, unsigned char move) {
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

// Espera comandos por stdin con el pselect y la mascara de ejecucion: el master duerme hasta
// que llegue una señal o haya una linea para leer.
//   * afterGameOver = false (pausa): termina con SIGUSR1 o SIGTERM, el stdin cerrado se ignora.
//   * afterGameOver = true (fin del juego): termina con "over", SIGTERM o si stdin se cierra
//     (nadie podria escribir "over"). SIGUSR1 no tiene sentido y se descarta
static void processCommands(GameState * gs, GameSync * sync, const MasterArgs * args, const sigset_t * runningMask, ReplayLog * replay, bool afterGameOver) {
    bool stdinOpen = true;
    bool over = false;

    while(sigtermReceived == 0 && !over) {
        if(afterGameOver) {
            sigusr1Received = 0;
        }
        else if(sigusr1Received != 0) {
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        if(stdinOpen) {
            FD_SET(STDIN_FILENO, &readSet);
        }

        int ready = pselect(stdinOpen ? STDIN_FILENO + 1 : 0, stdinOpen ? &readSet : NULL, NULL, NULL, NULL, runningMask);

        if(ready == -1 && errno != EINTR) { // stdin invalido
            stdinOpen = false;
        }
        else if(ready > 0 && FD_ISSET(STDIN_FILENO, &readSet)) {
            char line[128];
            ssize_t readBytes = read(STDIN_FILENO, line, sizeof(line) - 1);

            if(readBytes <= 0) { // EOF: si no se desactiva, el pselect despertaria sin parar
                stdinOpen = false;
            }
            else {
                line[readBytes] = '\0';
                over = replayHandleCommand(line, replay, gs, sync, args, runningMask, afterGameOver);
            }
        }

        if(afterGameOver && !stdinOpen) {
            break;
        }
    }
}

// Cuando termina la partida, antes de matar a los procesos y mostrar el resultado, se deja
// que el usuario vea el replay. Se sigue con el flujo de gameOver al escribir "over".
static void offerReplayBeforeGameOver(GameState * gs, GameSync * sync, const MasterArgs * args, const sigset_t * runningMask, ReplayLog * replay) {
    // sin vista o sin log no hay nada para reproducir
    if(!args->hasView || replay->initialState == NULL) {
        return;
    }

    // la vista sale de su bucle al dibujar un cuadro con isGameOver, y todavia la necesitamos.
    // gameOver lo vuelve a poner en true. Se muestra como PAUSADO mientras dura la espera.
    writerLock(sync);
    gs->isGameOver = false;
    gs->isGamePaused = true;
    writerUnlock(sync);
    notifyView(sync, args);

    fprintf(stderr, "\nEl juego termino (%zu jugadas). Comandos: replay [desde [hasta]] | over (ver el resultado y cerrar)\n", replay->count);

    processCommands(gs, sync, args, runningMask, replay, true);

    setPaused(gs, sync, false);
}

// Mientras el juego esta pausado el master no atiende movimientos: duerme en un pselect
// sin descriptores hasta que llegue la proxima señal. El tiempo pausado no se le descuenta
// al timeout, por eso al reanudar se corre hacia adelante la marca del ultimo movimiento.
static void waitWhilePaused(GameState * gs, GameSync * sync, const MasterArgs * args, const sigset_t * runningMask, struct timespec * lastValidMove, ReplayLog * replay) {
    struct timespec pauseStart;
    clock_gettime(CLOCK_MONOTONIC, &pauseStart);

    setPaused(gs, sync, true);
    notifyView(sync, args);
    // informo que se puede usar replay
    fprintf(stderr, "Juego pausado. Comando disponible: replay [desde [hasta]]\n");

    processCommands(gs, sync, args, runningMask, replay, false);
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

    writerLock(sync); // block x si voy a escribir el game_state
    // si algun player se queda encerrado lo marco como "bloqueado"
    markEnclosedPlayersAsBlocked(gs);
    writerUnlock(sync);

    // registro del estado inicial y de las jugadas para el replay (sin vista no hay a quien mostrarlo)
    ReplayLog replay;
    if(!args->hasView || !replayInit(&replay, gs, sizeof(GameState) + (size_t)gs->boardWidth * gs->boardHeight)) {
        memset(&replay, 0, sizeof(replay));
    }

    notifyView(sync, args);

    // arranco reloj para el timeout
    struct timespec lastValidMove;
    clock_gettime(CLOCK_MONOTONIC, &lastValidMove);

    bool gameFinished = false;

    while(1) {

        // por cada bucle entrego señales pendientes si hay
        deliverPendingSignals(&runningMask, &handledSignals);

        if(gameFinished || sigtermReceived) {
            break; // fin juego
        }

        if(sigusr1Received) { // hay que pausar juego
            sigusr1Received = 0;
            waitWhilePaused(gs, sync, args, &runningMask, &lastValidMove, &replay);
            continue;
        }

        // corrige tiempo de timeout si es que hubo una pausa 
        long remainingMilliseconds = (long)args->timeout * MILLISECONDS_PER_SECOND - millisecondsSince(&lastValidMove);
        if(remainingMilliseconds <= 0) { // timeout
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

        if(maxFd < 0) { // no hubo ningun player que pueda jugar (todos bloqueados)
            break;
        }

        struct timespec waitTime = {
            .tv_sec = remainingMilliseconds / MILLISECONDS_PER_SECOND,
            .tv_nsec = (remainingMilliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND
        };

        // con la runningMask dejo que entren señales
        // espero a que algun player pida movimiento o timeout
        int readyPlayers = pselect(maxFd + 1, &readSet, NULL, NULL, &waitTime, &runningMask);

        if(readyPlayers == -1) { // se recibio una señal -> vuelvo al principio
            if(errno == EINTR) {
                continue;
            }
            // si no fue por una señal / hubo otro error
            perror("pselect: Error esperando los movimientos de los jugadores");
            break;
        }

        if(readyPlayers == 0) {
            break;
        }

        // Arrancamos por el siguiente al ultimo que jugo para que ninguno pueda
        // acaparar los turnos solo por tener el pipe listo antes que el resto.
        // (RR)
        int roundStart = *lastPlayerPlayed;

        for(int offset = 0; offset < args->cantPlayers && !gameFinished; offset++) {
            int playerIndex = (roundStart + 1 + offset) % args->cantPlayers;

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
                
                // guardo jugada en replay
                replayRecord(&replay, playerIndex, REPLAY_DISCONNECT);
                
                gameFinished = everyPlayerIsBlocked(gs);
                gs->isGameOver = gameFinished;
                writerUnlock(sync);
                continue;
            }

            *lastPlayerPlayed = playerIndex;

            writerLock(sync);
            bool moveWasValid = applyMove(gs, playerIndex, move);
            
            // guardo jugada en replay
            replayRecord(&replay, playerIndex, move);

            markEnclosedPlayersAsBlocked(gs);
            gameFinished = everyPlayerIsBlocked(gs);
            gs->isGameOver = gameFinished;
            writerUnlock(sync);

            if(moveWasValid) {
                clock_gettime(CLOCK_MONOTONIC, &lastValidMove);
            }

            sem_post(&sync->playerTurn[playerIndex]);

            // el ultimo cuadro lo manda gameOver junto con el aviso de fin de juego,
            // asi la vista no imprime dos veces el mismo tablero.
            if(!gameFinished) {
                notifyView(sync, args);
                if(args->hasView) {
                    sleepMilliseconds(args->delay);
                }
            }
        }
    }

    // libero recursos del replay 
    // si el juego termino solo (no por SIGTERM) se ofrece el replay antes del gameOver
    if(!sigtermReceived) {
        offerReplayBeforeGameOver(gs, sync, args, &runningMask, &replay);
    }

    replayDestroy(&replay);

    // desbloqueo señales de nuevo
    sigprocmask(SIG_SETMASK, &runningMask, NULL);
}

// Desempate del enunciado: mas puntos, menos movimientos validos, menos invalidos.
// Devuelve > 0 si gana a, < 0 si gana b, 0 si empatan en todos los criterios.
static int comparePlayers(const Player * a, const Player * b) {
    if(a->playerScore != b->playerScore) {
        return (a->playerScore > b->playerScore) ? 1 : -1;
    }
    if(a->validMoves != b->validMoves) {
        return (a->validMoves < b->validMoves) ? 1 : -1;
    }
    if(a->invalidMoves != b->invalidMoves) {
        return (a->invalidMoves < b->invalidMoves) ? 1 : -1;
    }
    return 0;
}

// Calcula el ganador y por cuanto gano, y lo escribe a continuacion del tablero
// dentro de /game_state para que la vista lo muestre. Se llama con el writerLock tomado.
static void writeResult(GameState * gs) {
    int best = 0;
    for(int i = 1; i < gs->cantPlayers; i++) {
        if(comparePlayers(&gs->players[i], &gs->players[best]) > 0) {
            best = i;
        }
    }

    int second = -1;
    bool tied = false;
    for(int i = 0; i < gs->cantPlayers; i++) {
        if(i == best) {
            continue;
        }
        if(comparePlayers(&gs->players[best], &gs->players[i]) == 0) {
            tied = true;
        }
        if(second == -1 || comparePlayers(&gs->players[i], &gs->players[second]) > 0) {
            second = i;
        }
    }

    GameResult result;
    memset(&result, 0, sizeof(result));
    result.winnerId = (unsigned char)best;
    result.winnerScore = gs->players[best].playerScore;
    result.margin = (second == -1) ? 0 : result.winnerScore - gs->players[second].playerScore;
    result.hasWinner = !tied;
    result.ready = true;

    // memcpy y no un puntero directo: el tablero puede tener tamaño impar y el struct quedaria desalineado
    size_t boardSize = (size_t)gs->boardWidth * gs->boardHeight;
    memcpy((unsigned char *)gs + sizeof(GameState) + boardSize, &result, sizeof(result));
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

    // el resultado queda escrito antes del ultimo aviso a la vista,
    // asi ya esta disponible cuando la vista sale de su bucle
    writerLock(sync);
    writeResult(gs);
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
