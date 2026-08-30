#ifndef GAME_STATE_H
#define GAME_STATE_H

// HEADER del estado del juego con sus respectivos structs

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_PLAYERS 9

typedef struct {
    char playerName[16];
    unsigned int playerScore;
    unsigned int validMoves;
    unsigned int invalidMoves;
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
    signed char board[]; // puntero al comienzo del tablero      // scores de 1-9, 0..-8=ocupado por jugador -v   
} GameState;

#endif