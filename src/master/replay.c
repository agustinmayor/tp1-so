/**
 * Implementación de las funciones de replay.h
 */

#include "replay.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>

#include "gameLoop.h"
#include "signals.h"

#define REPLAY_INITIAL_CAPACITY 1024
#define SEPARATORS " \t\r\n"

// inicio el sistema de replays
bool replayInit(ReplayLog * log, const GameState * gs, size_t stateSize) {
    memset(log, 0, sizeof(*log));

    log->initialState = malloc(stateSize);

    if(log->initialState == NULL) {
        return false;
    }

    memcpy(log->initialState, gs, stateSize);
    log->stateSize = stateSize;
    return true;
}

// guardo una nueva jugada en el log
void replayRecord(ReplayLog * log, int playerIdx, unsigned char move) {
    // la replay no esta habilitada
    if(log->initialState == NULL) {
        return;
    }

    // si tengo que aumentar la capacidad del log
    if(log->count == log->capacity) {
        // seteo una nueva capacidad que es la inicial o el doble de la actual
        size_t newCapacity = (log->capacity == 0) ? REPLAY_INITIAL_CAPACITY : log->capacity * 2;
        
        ReplayEntry * newEntries = realloc(log->entries, newCapacity * sizeof(ReplayEntry));

        // si no tengo memoria dejo que el juego siga pero desactivo la replay
        if(newEntries == NULL) {
            replayDestroy(log);
            return;
        }

        log->entries = newEntries;
        log->capacity = newCapacity;
    }

    // agrego la entrada a la lista
    log->entries[log->count].player = (unsigned char)playerIdx;
    log->entries[log->count].move = move;
    log->count++;
}

// libero la memoria usada por el log y la seteo como inhabilitada
void replayDestroy(ReplayLog * log) {
    free(log->initialState);
    free(log->entries);
    memset(log, 0, sizeof(*log));
}

// Aplica una entrada de replay al estado del juego
// lo uso para poder reproducir la replay desde la vista
static void appyEntry(GameState * gs, ReplayEntry entry) {
    
    if(entry.move == REPLAY_DISCONNECT) {
        gs->players[entry.player].isBlocked = true;
    }
    else {
        applyMove(gs, entry.player, entry.move);
    }

    markEnclosedPlayersAsBlocked(gs);
}

// espera entre cuadros con pselect y la mascara de señales de ejecucion
// para que las señales corten la espera como en el resto del master
static void waitBetweenFrames(unsigned int ms, const sigset_t * runningMask) {
    struct timespec wait = {
        .tv_sec = ms / 1000,
        .tv_nsec = (long)(ms % 1000) * 1000000L
    };

    pselect(0, NULL, NULL, NULL, &wait, runningMask);
}

static bool interrupted(void) {
    return sigtermReceived != 0 || sigusr1Received != 0;
}

// muestra los cuadros desde from a to
// guardo el estado del juego real para restaurar al final
// mantenemos el writerLock porque se va a guardar estados que son mentira

static void play(const ReplayLog * log, GameState * gs, GameSync * sync, const MasterArgs * args, const sigset_t * runningMask, size_t from, size_t to) {
    GameState * realState = malloc(log->stateSize);
    
    if(realState == NULL) { // no se pudo hacer el malloc
        fprintf(stderr, "Error: no se pudo reproducir la replay por falta de memoria\n");
        return;
    }

    memcpy(realState, gs, log->stateSize);

    // bloqueo a los procesos player para que no lean el estado "mentira"
    writerLock(sync);

    // primera vista
    memcpy(gs, log->initialState, log->stateSize);
    gs->isGamePaused = true; // en realidad esta PAUSADO

    size_t applied = 0;

    // aplico las entradas hasta llegar a "from"
    // me las salteo para no imprimirlas
    while(applied < from) {
        applyEntry(gs, log->entries[applied++]);
    }

    while(1) {
        notifyView(sync, args);

        if(applied == to || interrupted()) {
            break;
        }

        waitBetweenFrames(args->delay, runningMask);

        if(interrupted()) {
            break;
        }

        applyEntry(gs, log->entries[applied++]);
    }

    // restauramos el estado real del juego
    memcpy(gs, realState, log->stateSize);

    // desbloqueo a los procesos player
    writerUnlock(sync);

    free(realState);

    notifyView(sync, args); // vuelvo al estado real
}
