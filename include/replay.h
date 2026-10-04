#ifndef REPLAY_H
#define REPLAY_H

/**
 * Funciones y Struct para registrar las jugadas en orden
 */

#include <signal.h>
#include <stddef.h>
#include "game_state.h"
#include "game_sync.h"
#include "master.h"

// Valor de "move" para registrar que un jugador se desconecto (EOF en su pipe)
#define REPLAY_DISCONNECT 0xFF

// ReplayEntry: cada jugada registrada en el replay
typedef struct {
    unsigned char player; // jugador de la jugada (id)
    unsigned char move;  // jugada que hizo, direccion
} ReplayEntry;

// ReplayLog: log de todas las jugadas desde el estado incial
typedef struct {
    GameState * initialState;
    size_t stateSize;
    ReplayEntry * entries;
    size_t count;
    size_t capacity;
} ReplayLog;

// Funciones para manejar todo el log de jugadas
bool replayInit(ReplayLog * log, const GameState * gs, size_t stateSize);
void replayRecord(ReplayLog * log, int playerIdx, unsigned char move);
void replayDestroy(ReplayLog * log);

// Parsea y ejecuta un comando pasado por stdin
// valida numeros de jugada y reproduce la replay pedida
// allowOver habilita el comando "over" (solo despues de terminado el juego)
// devuelve true si el comando fue "over"
bool replayHandleCommand(char * line, ReplayLog * log, GameState * gs, GameSync * sync, const MasterArgs * args, const sigset_t * runningMask, bool allowOver);

#endif