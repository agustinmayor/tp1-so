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

// Ubica a los jugadores en el tablero
void locatePlayers(GameState * gs) {

}