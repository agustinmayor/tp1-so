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

#endif