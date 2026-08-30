#ifndef GAME_H
#define GAME_H

#include "game_state.h"
#include "game_sync.h"
#include "signals.h"

void initBoard(GameState * gs, unsigned int seed);
void locatePlayers(GameState * gs);

#endif
