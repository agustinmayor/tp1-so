#include "playerAI.h"

#include <stdlib.h>
#include <string.h>


// Estrategia: la celda adyacente libre con mayor recompensa
unsigned char chooseMove(const GameSnapshot * snapshot, int myIndex, const GameState * gs) {

    (void)gs; // para bypasear warning de unused

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
