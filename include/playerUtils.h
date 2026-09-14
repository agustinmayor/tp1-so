#ifndef PLAYER_UTILS_H
#define PLAYER_UTILS_H

#include "playerAI.h"

typedef unsigned char (*chooseMoveFunction)(const GameSnapshot * snapshot, int myIndex);

// corremos el loop generico de un jugador
// sin importar si es IA o si es el playerBonus
int runPlayerLoop(int argc, char * argv[], chooseMoveFunction chooseMoveFn);

#endif
