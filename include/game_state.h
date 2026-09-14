#ifndef GAME_STATE_H
#define GAME_STATE_H

// HEADER del estado del juego con sus respectivos structs

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_PLAYERS 9

#define SHM_GAME_STATE_NAME "/game_state"

typedef struct {
    char playerName[16];
    unsigned int playerScore;
    unsigned int invalidMoves;
    unsigned int validMoves;
    unsigned short playerX, playerY; // coords del jugador en el tablero
    pid_t pid;
    bool isBlocked;
} Player;

typedef struct {
    unsigned short boardWidth;
    unsigned short boardHeight;
    unsigned char cantPlayers;
    Player players[MAX_PLAYERS]; // lista de players
    bool isGameOver;
    bool isGamePaused;
    signed char board[]; // celda libre = recompensa 1..9, celda capturada = -id del dueño
} GameState;


/* Resultado final del juego. El master lo escribe a continuacion del tablero,
 dentro de la misma /game_state, y la vista lo lee cuando termina el juego. */
 
typedef struct {
    unsigned int winnerScore;   // puntaje del ganador
    unsigned int margin;        // puntos de ventaja sobre el segundo
    unsigned char winnerId;     // indice del ganador en players[]
    bool hasWinner;             // false si hubo empate en todos los criterios
    bool ready;                 // true cuando el master ya lo escribio
} GameResult;

#endif
