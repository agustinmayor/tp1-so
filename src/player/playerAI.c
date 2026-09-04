#include "playerAI.h"

#include <stdlib.h>
#include <string.h>

bool createSnapshot(GameSnapshot * snapshot, unsigned short boardWidth, unsigned short boardHeight) {
    snapshot->boardWidth = boardWidth;
    snapshot->boardHeight = boardHeight;
    snapshot->cantPlayers = 0;
    snapshot->isGameOver = false;
    snapshot->amIBlocked = false;
    snapshot->board = malloc((size_t)boardWidth * boardHeight * sizeof(signed char));

    return snapshot->board != NULL;
}

void destroySnapshot(GameSnapshot * snapshot) {
    free(snapshot->board);
    snapshot->board = NULL;
}

void takeSnapshot(GameSnapshot * snapshot, const GameState * gs, int myIndex) {
    snapshot->cantPlayers = gs->cantPlayers;
    snapshot->isGameOver = gs->isGameOver;
    snapshot->amIBlocked = gs->players[myIndex].isBlocked;

    for(unsigned char i = 0; i < gs->cantPlayers; i++) {
        snapshot->playersX[i] = gs->players[i].playerX;
        snapshot->playersY[i] = gs->players[i].playerY;
    }

    memcpy(snapshot->board, gs->board, (size_t)snapshot->boardWidth * snapshot->boardHeight);
}

// Estrategia inicial: la celda adyacente libre con mayor recompensa
unsigned char chooseMove(const GameSnapshot * snapshot, int myIndex) {

    int myX = snapshot->playersX[myIndex];
    int myY = snapshot->playersY[myIndex];

    int bestDirection = 0;
    int bestReward = 0;

    for(int direction = 0; direction < DIRECTION_COUNT; direction++) {
        int targetX = myX + DIRECTION_DX[direction];
        int targetY = myY + DIRECTION_DY[direction];

        if(!isFreeCell(snapshot->board, snapshot->boardWidth, snapshot->boardHeight, targetX, targetY)) {
            continue;
        }

        int reward = cellAt(snapshot->board, snapshot->boardWidth, targetX, targetY);

        if(reward > bestReward) {
            bestReward = reward;
            bestDirection = direction;
        }
    }

    return (unsigned char)bestDirection;
}
