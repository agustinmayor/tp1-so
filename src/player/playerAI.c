#include <playerAI.h>

/* Direcciones: 
** 0 = Norte
** 1 = Noreste
** 2 = Este
** 3 = Sureste
** 4 = Sur
** 5 = Suroeste
** 6 = Oeste
** 7 = Noroeste 
*/

static const int DY[] = {-1,    -1,    0,     1,    1,      1,      0,  -1};
static const int DX[] = {0,     1,     1,     1,    0,     -1,     -1,  -1};

unsigned char findBestMove(const GameState *state, int myIndex){

    int myX = state->players[myIndex].x;
    int myY = state->players[myIndex].y;
    int bestDir = -1;
    int bestValue = -1;

    /* Recorre todas las direcciones y calcula la mejor posibilidad para moverse. */
    for(int dir = 0; dir < DIRECTION_COUNT; dir++){
        int nx = myX + DX[dir];
        int ny = myY + DY[dir];

        if((nx < 0) || (nx >= state->width) || (ny < 0) || (ny >= state->height)){
            continue;
        }

        int cell = BOARD_AT(state->board, state->width, ny, nx);    /* Guardo el valor de la celda. */

        if((cell > 0) && (cell > bestValue)){
            /* Actualizo la mejor opción. */
            bestValue = cell;
            bestDir = dir;
        }
    }

    return ((bestDir == -1) ? 0 : (unsigned char)bestDir);     /* Si no hay movimientos válidos, devuelve 0 (Norte). */
    