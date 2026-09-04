#include "board.h"

const int DIRECTION_DX[DIRECTION_COUNT] = {  0,  1,  1,  1,  0, -1, -1, -1 };
const int DIRECTION_DY[DIRECTION_COUNT] = { -1, -1,  0,  1,  1,  1,  0, -1 };

bool isInsideBoard(unsigned short boardWidth, unsigned short boardHeight, int x, int y) {
    return x >= 0 && x < (int)boardWidth && y >= 0 && y < (int)boardHeight;
}

signed char cellAt(const signed char * board, unsigned short boardWidth, int x, int y) {
    return board[(size_t)y * boardWidth + (size_t)x];
}

bool isFreeCell(const signed char * board, unsigned short boardWidth, unsigned short boardHeight, int x, int y) {
    return isInsideBoard(boardWidth, boardHeight, x, y) && cellAt(board, boardWidth, x, y) > 0;
}

bool hasFreeNeighbour(const signed char * board, unsigned short boardWidth, unsigned short boardHeight, int x, int y) {
    for(int direction = 0; direction < DIRECTION_COUNT; direction++) {
        if(isFreeCell(board, boardWidth, boardHeight, x + DIRECTION_DX[direction], y + DIRECTION_DY[direction])) {
            return true;
        }
    }
    return false;
}
