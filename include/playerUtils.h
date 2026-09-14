#ifndef PLAYER_UTILS_H
#define PLAYER_UTILS_H

#include "playerAI.h"

// funcion que decide el mov de cada player
// lo elige cada player segun si es IA o si es playerBonus
typedef unsigned char (*chooseMoveFunction)(const GameSnapshot * snapshot, int myIndex, const GameState * gs);

// corremos el loop generico de un jugador
// sin importar si es IA o si es el playerBonus
int runPlayerLoop(int argc, char * argv[], chooseMoveFunction chooseMoveFn);


// Funciones para tomar snapshots de el estado del juego
bool createSnapshot(GameSnapshot * snapshot, unsigned short boardWidth, unsigned short boardHeight);

void destroySnapshot(GameSnapshot * snapshot);

void takeSnapshot(GameSnapshot * snapshot, const GameState * gs, int myIndex);
#endif
