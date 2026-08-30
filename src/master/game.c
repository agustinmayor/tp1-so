// Archivo donde estará la logica del master para manejar el juego
#include "game.h"

// Inicializa el tablero randomizando los valores de cada celda
void initBoard(GameState * gs, unsigned int seed){
    
    int cells = gs->boardWidth * gs->boardHeight;
    
    srand(seed);

    for(int i = 0; i < cells; i++) {
        // valores random entre 1 y 9 a partir de la seed
        gs->board[i] = (rand() % 9) + 1;
    }
}

// Ubica a los jugadores en el tablero formando un rectangulo con sus lados dos unidades
// mas chicas que el tablero

// ubicamos a los jugadores igualmente espaciados entre si para que el juego sea parejo
void locatePlayers(GameState * gs) {

    int players = gs->cantPlayers;
    int width = gs->boardWidth;
    int height = gs->boardHeight;

    int leftSide = 1;
    int rightSide = width - 2;
    int topSide = 1;
    int bottomSide = height - 2;

    int rectangleWidth = rightSide - leftSide;
    int rectangleHeight = bottomSide - topSide;
    int rectanglePerimeter = 2 * (rectangleWidth + rectangleHeight);

    for(int i = 0; i < players; i++) {
        
        int positionOnPerimeter = (i * rectanglePerimeter) / players;

        if (positionOnPerimeter < rectangleWidth) {
            // El player se tiene que ubicar en el lado de arriba del rectangulo 
            gs->players[i].playerX = leftSide + positionOnPerimeter;
            gs->players[i].playerY = topSide;

        } else if (positionOnPerimeter < rectangleWidth + rectangleHeight) {
            // El player se tiene que ubicar en el lado derecho del rectangulo
            gs->players[i].playerX = rightSide;
            gs->players[i].playerY = topSide + (positionOnPerimeter - rectangleWidth);

        } else if (positionOnPerimeter < 2 * rectangleWidth + rectangleHeight) {
            // El player se tiene que ubicar en el lado de abajo del rectangulo
            gs->players[i].playerX = rightSide - (positionOnPerimeter - (rectangleWidth + rectangleHeight));
            gs->players[i].playerY = bottomSide;

        } else {
            // El player se tiene que ubicar en el lado izquierdo del rectangulo
            gs->players[i].playerX = leftSide;
            gs->players[i].playerY = bottomSide - (positionOnPerimeter - (2 * rectangleWidth + rectangleHeight));
        }


        gs->players[i].playerScore = 0;
        gs->players[i].validMoves = 0;
        gs->players[i].invalidMoves = 0;
        gs->players[i].isBlocked = false;

        int playerX = gs->players[i].playerX;
        int playerY = gs->players[i].playerY;

        // Marco casilla respectiva del board como ocupada por el jugador
        gs->board[playerX + playerY * width] = (signed char)(-i);
    }

}