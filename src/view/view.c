#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "game_state.h"
#include "game_sync.h"

#define STATE_SHM "/game_state"
#define SYNC_SHM  "/game_sync"

/* OBS El campo que se llama validMoves guarda en realidad los INVALIDOS y vic */
#define MOVES_VALID(p)   ((p)->invalidMoves)
#define MOVES_INVALID(p) ((p)->validMoves)

static void *mapShm(const char *name, int openFlags, int prot, size_t size) {
    int fd = shm_open(name, openFlags, 0);
    if (fd == -1) {
        perror(name);
        exit(EXIT_FAILURE);
    }
    void *addr = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    close(fd);                       /* el mapeo sobrevive al close */
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return addr;
}

/* id de  player  cabeza esta en y x  -1 si no hay ninguna */
static int headOwner(const GameState *st, unsigned x, unsigned y) {
    for (unsigned i = 0; i < st->cantPlayers; i++) {
        if (st->players[i].playerX == x && st->players[i].playerY == y) {
            return (int)i;
        }
    }
    return -1;
}

static void renderFrame(const GameState *st) {
    unsigned w = st->boardWidth; unsigned h = st->boardHeight;

    printf("\033[2J\033[H");
    printf("%ux%u   %u jugadores   %s\n\n",
           w, h, (unsigned)st->cantPlayers,
           st->isGameOver ? "TERMINADO" : st->isGamePaused ? "PAUSADO" : "EN JUEGO");

    for (unsigned y = 0; y < h; y++) {
        for (unsigned x = 0; x < w; x++) {
            signed char v = st->board[(size_t)y * w + x];
            int head = headOwner(st, x, y);
            if (head >= 0) {
                printf(" @%d", head);        /* cabeza del jugador */
            } else if (v <= 0) {
                printf(" #%d", -(int)v);     /* capturada: dueno = -v, y 0 es el jugador 0 */
            } else {
                printf("  %d", (int)v);      /* libre: recompensa */
            }
        }
        putchar('\n');
    }
    printf("\n  ID  NOMBRE            PUNTOS  VALIDOS  INVALIDOS   POSICION   ESTADO\n");
    for (unsigned i = 0; i < st->cantPlayers; i++) {
        const Player *p = &st->players[i];
        printf("  %2u  %-16.16s  %6u  %7u  %9u   (%3u,%3u)   %s\n",
               i, p->playerName, p->playerScore,
               MOVES_VALID(p), MOVES_INVALID(p),
               (unsigned)p->playerX, (unsigned)p->playerY,
               p->isBlocked ? "BLOQUEADO" : "activo");
    }
    fflush(stdout);
}

int main(int argc, char *argv[]) {
 

    size_t width  = strtoul(argv[1], NULL, 10); size_t height = strtoul(argv[2], NULL, 10);


    size_t stateSize = sizeof(GameState) + width * height;
    GameState *state = mapShm(STATE_SHM, O_RDONLY, PROT_READ, stateSize);

    GameSync *sync = mapShm(SYNC_SHM, O_RDWR, PROT_READ | PROT_WRITE, sizeof(GameSync));

    while (true) {
        /*  Master dice que hay cambios  */
        if (sem_wait(&sync->viewSignal) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("sem_wait");
            break;
        }

        renderFrame(state);

        /* empezar a desarmar todo, y  valor del frame recien impreso */
        bool over = state->isGameOver;

        /* le dice a  master que termino de imprimir */
        if (sem_post(&sync->viewUpdated) == -1) {
            perror("sem_post");
            break;
        }

        if (over) {
            break;
        }
    }

    munmap(state, stateSize);
    munmap(sync, sizeof(GameSync));
    return EXIT_SUCCESS;
}
