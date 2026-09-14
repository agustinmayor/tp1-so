#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game_state.h"
#include "game_sync.h"

#define EXPECTED_ARGC 3

#define TITLE     "CHOMPCHAMPS"
#define TITLE_W   11
#define TABLE_HDR "ID  NOMBRE            PUNTOS  VALIDOS  INVALIDOS    POSICION   ESTADO"
#define TABLE_W   72   /* ancho de una fila completa, incluye "BLOQUEADO" */

#define BORDER "\033[38;5;240m"   /* para el marco */

#define RESET  "\033[0m"
#define EOL    "\033[0m\033[K\n"  /* fin de linea(resetea color y borra  resto) */

/* Colores para los jugadores */
static const unsigned char PLAYER_HEAD[MAX_PLAYERS]  = {  33, 208,  41, 137, 226,  51, 196, 129, 245 };
static const unsigned char PLAYER_TRAIL[MAX_PLAYERS] = {  19, 130,  28,  94, 100,  30,  88,  54, 238 };
static const unsigned char PLAYER_FG[MAX_PLAYERS]    = { 231,  16,  16,  16,  16,  16, 231, 231,  16 };

/* Buffer de pantalla */
typedef struct {
    char  *data;
    size_t cap;
    size_t len;
} FrameBuf;

__attribute__((format(printf, 2, 3)))
static void emit(FrameBuf *f, const char *fmt, ...) {
    size_t space = f->cap - f->len;
    if (space <= 1) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(f->data + f->len, space, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return;
    }
    f->len += ((size_t)n < space) ? (size_t)n : space - 1;
}

/* devuelve el ancho del padding para centrar un item de ancho 'item' dentro de uno de ancho 'total' */
static unsigned centerPad(unsigned total, unsigned item) {
    return (total > item) ? (total - item) / 2 : 0;
}

static void pad(FrameBuf *f, unsigned n) {
    if (n > 0) {
        emit(f, "%*s", (int)n, "");
    }
}

/* Consulta al kernel tamano de ventana (para centrarlo) y  devuelve por parametro.  */
static void terminalSize(unsigned *cols, unsigned *rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return;
    }

    const char *envCols = getenv("COLUMNS");
    const char *envRows = getenv("LINES");
    unsigned long c = (envCols != NULL) ? strtoul(envCols, NULL, 10) : 0;
    unsigned long r = (envRows != NULL) ? strtoul(envRows, NULL, 10) : 0;

    *cols = (c > 0 && c <= 1000) ? (unsigned)c : 80;
    *rows = (r > 0 && r <= 1000) ? (unsigned)r : 24;
}

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

/* id del jugador cuya cabeza esta en (x,y), o -1 si no hay ninguna */
static int headOwner(const GameState *st, unsigned x, unsigned y) {
    for (unsigned i = 0; i < st->cantPlayers; i++) {
        if (st->players[i].playerX == x && st->players[i].playerY == y) {
            return (int)i;
        }
    }
    return -1;
}

static void hline(FrameBuf *f, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        emit(f, "\u2500");
    }
}

static void renderFrame(FrameBuf *f, const GameState *st) {
    unsigned w = st->boardWidth;
    unsigned h = st->boardHeight;

    unsigned cols, rows;
    terminalSize(&cols, &rows);

    unsigned boardW = w * 3 + 2;              /* celdas de 3 + los dos bordes */
    unsigned blockW = (boardW > TABLE_W) ? boardW : TABLE_W;
    unsigned left   = centerPad(cols, blockW);
    unsigned boardL = left + (blockW - boardW) / 2;

    /* titulo, estado, blanco, borde, tablero, borde, blanco, encabezado, filas */
    unsigned contentH = 7 + h + st->cantPlayers;
    unsigned top      = centerPad(rows, contentH);

    f->len = 0;
    emit(f, "\033[H");                        /* arriba de todo, sin borrar la pantalla */

    for (unsigned i = 0; i < top; i++) {
        emit(f, "\033[K\n");                  /* las lineas en blanco tambien se limpian */
    }

    pad(f, centerPad(cols, TITLE_W));
    emit(f, "\033[1m" TITLE EOL);

    char status[128];
    int n = snprintf(status, sizeof status, "%ux%u   %u jugadores   %s",
                     w, h, (unsigned)st->cantPlayers,
                     st->isGameOver ? "TERMINADO" : st->isGamePaused ? "PAUSADO" : "EN JUEGO");
    pad(f, centerPad(cols, (n > 0) ? (unsigned)n : 0));
    emit(f, "%s" EOL, status);
    emit(f, "\033[K\n");

    pad(f, boardL);
    emit(f, BORDER "\u250c");
    hline(f, w * 3);
    emit(f, "\u2510" EOL);

    for (unsigned y = 0; y < h; y++) {
        pad(f, boardL);
        emit(f, BORDER "\u2502" RESET);
        for (unsigned x = 0; x < w; x++) {
            signed char v = st->board[(size_t)y * w + x];
            int head = headOwner(st, x, y);

            if (head >= 0) {
                /* donde esta parado ahora: color vivo con su id */
                emit(f, "\033[1;38;5;%u;48;5;%um %u " RESET,
                     (unsigned)PLAYER_FG[head], (unsigned)PLAYER_HEAD[head], (unsigned)head);
            } else if (v <= 0) {
                /* ya capturada: mismo tono pero oscuro. El dueno es -v, y el
                   valor 0 corresponde al jugador 0, por eso es <= y no < */
                emit(f, "\033[48;5;%um   " RESET, (unsigned)PLAYER_TRAIL[-v]);
            } else {
                /* libre: la recompensa (1 a 9), mas brillante cuanto mas vale */
                emit(f, "\033[38;5;%um %d " RESET, 237u + 2u * (unsigned)v, (int)v);
            }
        }
        emit(f, BORDER "\u2502" EOL);
    }

    pad(f, boardL);
    emit(f, BORDER "\u2514");
    hline(f, w * 3);
    emit(f, "\u2518" EOL);

    emit(f, "\033[K\n");
    pad(f, left);
    emit(f, "\033[1m" TABLE_HDR EOL);
    for (unsigned i = 0; i < st->cantPlayers; i++) {
        const Player *p = &st->players[i];
        pad(f, left);
        /* el id va con el color vivo del jugador, hace de referencia del tablero */
        emit(f, "\033[38;5;%u;48;5;%um%2u" RESET, (unsigned)PLAYER_FG[i], (unsigned)PLAYER_HEAD[i], i);
        emit(f, "  %-16.16s  %6u  %7u  %9u   (%3u,%3u)   %s" EOL,
             p->playerName, p->playerScore,
             p->validMoves, p->invalidMoves,
             (unsigned)p->playerX, (unsigned)p->playerY,
             p->isBlocked ? "\033[1;31mBLOQUEADO" : "\033[32mactivo");
    }

    emit(f, "\033[0J");   /* borra todo lo que haya quedado mas abajo */
}


static bool readResult(size_t stateSize, GameResult *out) {
    int fd = shm_open(SHM_GAME_STATE_NAME, O_RDONLY, 0);
    if (fd == -1) {
        return false;
    }

    struct stat sb;
    size_t total = stateSize + sizeof(GameResult);
    if (fstat(fd, &sb) == -1 || (size_t)sb.st_size < total) {
        close(fd);
        return false;
    }

    unsigned char *mem = mmap(NULL, total, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        return false;
    }

    /* memcpy y no un puntero directo: stateSize puede ser impar y el struct
       quedaria desalineado */
    memcpy(out, mem + stateSize, sizeof(GameResult));
    munmap(mem, total);
    return out->ready;
}

/* Imprime centrado el resultado de el ganador */
static void renderResult(FrameBuf *f, const GameState *st, const GameResult *res) {
    char text[128];
    int n;
    bool hasWinner = res->hasWinner && res->winnerId < st->cantPlayers;

    if (hasWinner) {
        char suffix[32] = "";
        if (st->cantPlayers > 1 && res->margin > 0) {
            snprintf(suffix, sizeof suffix, ", %u de ventaja", res->margin);
        } else if (st->cantPlayers > 1) {
            snprintf(suffix, sizeof suffix, ", por desempate");
        }
        n = snprintf(text, sizeof text, "GANADOR: %.16s (jugador %u) con %u puntos%s",
                     st->players[res->winnerId].playerName, (unsigned)res->winnerId,
                     res->winnerScore, suffix);
    } else {
        n = snprintf(text, sizeof text, "EMPATE con %u puntos", res->winnerScore);
    }

    unsigned cols, rows;
    terminalSize(&cols, &rows);

    f->len = 0;
    pad(f, centerPad(cols, (n > 0) ? (unsigned)n : 0));
    if (hasWinner) {
        emit(f, "\033[1;38;5;%um%s" RESET "\n\n", (unsigned)PLAYER_HEAD[res->winnerId], text);
    } else {
        emit(f, "\033[1m%s" RESET "\n\n", text);
    }
}

/* si cortan con Ctrl-C hay que devolver el cursor, si no el terminal queda sin el */
static void onInterrupt(int sig) {
    (void)!write(STDOUT_FILENO, "\033[?25h\n", 7);
    _exit(128 + sig);
}

int main(int argc, char *argv[]) {
    if (argc != EXPECTED_ARGC) {
        fprintf(stderr, "Uso: %s <width> <height>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t width  = strtoul(argv[1], NULL, 10);
    size_t height = strtoul(argv[2], NULL, 10);

    if (width == 0 || height == 0) {
        fprintf(stderr, "Dimensiones de tablero invalidas\n");
        return EXIT_FAILURE;
    }

    size_t stateSize = sizeof(GameState) + width * height;
    GameState *state = mapShm(SHM_GAME_STATE_NAME, O_RDONLY, PROT_READ, stateSize);

    GameSync *sync = mapShm(SHM_GAME_SYNC_NAME, O_RDWR, PROT_READ | PROT_WRITE, sizeof(GameSync));

    FrameBuf frame;
    frame.cap  = (height + 60) * (width * 48 + 512) + 8192;
    frame.len  = 0;
    frame.data = malloc(frame.cap);
    if (frame.data == NULL) {
        perror("malloc");
        munmap(state, stateSize);
        munmap(sync, sizeof(GameSync));
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    sa.sa_handler = onInterrupt;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    (void)!write(STDOUT_FILENO, "\033[?25l", 6);   /* ocultar cursor */

    while(1) {
        /* Master dice que hay cambios */
        if (sem_wait(&sync->viewSignal) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("sem_wait");
            break;
        }
        renderFrame(&frame, state);
        (void)!write(STDOUT_FILENO, frame.data, frame.len);

        /* empezar a desarmar todo, y  valor del frame recien impreso */
        bool over = state->isGameOver;

        /* se le dice a  master que termino de imprimir */
        if (sem_post(&sync->viewUpdated) == -1) {
            perror("sem_post");
            break;
        }

        if (over) {
            break;
        }
    }

    /* baja  cursor para que el resumen del master no lo pise. */
    (void)!write(STDOUT_FILENO, "\033[?25h\n\n", 8);

    /* resultado que dejo el master al terminar: ganador y por cuanto */
    GameResult result;
    if (state->isGameOver && readResult(stateSize, &result)) {
        renderResult(&frame, state, &result);
        (void)!write(STDOUT_FILENO, frame.data, frame.len);
    }

    free(frame.data);
    munmap(state, stateSize);
    munmap(sync, sizeof(GameSync));
    return EXIT_SUCCESS;
}