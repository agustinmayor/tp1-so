#ifndef GAME_H
#define GAME_H

#include "game_state.h"
#include "game_sync.h"
#include "signals.h"

#include <sys/types.h>

void initBoard(GameState * gs, unsigned int seed);
void locatePlayers(GameState * gs);

pid_t spawnPlayer(const char * playerPath, unsigned short boardWidth, unsigned short boardHeight, int playerIndex, int * pipeFd);

#endif
