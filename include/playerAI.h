#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include "board.h"
#include "game_state.h"

// Copia local del estado. Se toma dentro del lock de lectura para que la seccion
// critica sea lo mas corta posible y decidir el movimiento no frene al master.
typedef struct {
    unsigned short boardWidth;
    unsigned short boardHeight;
    unsigned char cantPlayers;
    unsigned short playersX[MAX_PLAYERS];
    unsigned short playersY[MAX_PLAYERS];
    bool isGameOver;
    bool amIBlocked;
    signed char * board;
} GameSnapshot;

bool createSnapshot(GameSnapshot * snapshot, unsigned short boardWidth, unsigned short boardHeight);

void destroySnapshot(GameSnapshot * snapshot);

void takeSnapshot(GameSnapshot * snapshot, const GameState * gs, int myIndex);

// Devuelve la direccion elegida, en el rango [0, 7]
unsigned char chooseMove(const GameSnapshot * snapshot, int myIndex);

#endif
