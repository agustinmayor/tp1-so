#ifndef BOARD_H
#define BOARD_H


#include "game_state.h"

#define DIRECTION_COUNT 8

// 0 = arriba, avanzando en sentido horario
extern const int DIRECTION_DX[DIRECTION_COUNT];
extern const int DIRECTION_DY[DIRECTION_COUNT];

bool isInsideBoard(unsigned short boardWidth, unsigned short boardHeight, int x, int y);

signed char cellAt(const signed char * board, unsigned short boardWidth, int x, int y);

bool isFreeCell(const signed char * board, unsigned short boardWidth, unsigned short boardHeight, int x, int y);

bool hasFreeNeighbour(const signed char * board, unsigned short boardWidth, unsigned short boardHeight, int x, int y);

#endif
