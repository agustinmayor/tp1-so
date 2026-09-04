// Archivo donde estará la logica del master para manejar el juego
#include "gameInit.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

// Metodo para crear un proceso jugador y establecer la comunicacion con un pipe
// 
// Un proceso jugador recibe como argumentos el ancho y el alto del tablero, y su indice de jugador (0..8).

// pipe --> fork --> dup2 --> exec
//
// Retorna el pid del proceso jugador creado

pid_t spawnPlayer(const char * playerPath, unsigned short boardWidth, unsigned short boardHeight, int playerIndex, int * pipeFd) {
    
    int fd[2];
    // fd[0] -> extremo de lectura del pipe
    // fd[1] -> extremo de escritura del pipe
    
    // creo el pipe
    if(pipe(fd) == -1) {
        perror("pipe: Error al crear el pipe");
        exit(EXIT_FAILURE);
    }

    // el extremo de lectura no se hereda: si cada jugador conservara los pipes de sus
    // hermanos, el master nunca veria el EOF que marca a un jugador como bloqueado
    if(fcntl(fd[0], F_SETFD, FD_CLOEXEC) == -1) {
        perror("fcntl: Error al marcar el pipe como no heredable");
        exit(EXIT_FAILURE);
    }

    // forkeo
    pid_t pid = fork();
    if(pid == -1) {
        perror("fork: Error al crear el proceso jugador");
        exit(EXIT_FAILURE);
    }

    if(pid==0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO); // redirijo stdout al pipe
        close(fd[1]); // cierro el extremo de escritura del pipe en el hijo

        char boardWidthStr[16], boardHeightStr[16], playerIndexStr[16];

        // para hacer execl necesito los params como strings 
        snprintf(boardWidthStr, sizeof(boardWidthStr), "%hu", boardWidth);
        snprintf(boardHeightStr, sizeof(boardHeightStr), "%hu", boardHeight);
        snprintf(playerIndexStr, sizeof(playerIndexStr), "%d", playerIndex);

        execl(playerPath, playerPath, boardWidthStr, boardHeightStr, playerIndexStr, (char *)NULL);

        // si execl vuelve, es porque hubo un error
        perror("execl: Error al ejecutar el proceso jugador");
        exit(EXIT_FAILURE);

    }

    close(fd[1]); // cierro el extremo de escritura del pipe en el papa
    *pipeFd = fd[0];
    
    return pid;

}

// Metodo para crear un proceso vista

// El proceso vista recibe como argumentos el ancho y el alto del tablero.
pid_t spawnView(const char *viewPath, unsigned short boardWidth, unsigned short boardHeight) {

    pid_t pid = fork();

    if(pid == -1) {
        perror("fork: Error al crear el proceso vista");
        exit(EXIT_FAILURE);
    }

    if(pid == 0) {
        char boardWidthStr[16], boardHeightStr[16];

        // para hacer execl necesito los params como strings 
        snprintf(boardWidthStr, sizeof(boardWidthStr), "%hu", boardWidth);
        snprintf(boardHeightStr, sizeof(boardHeightStr), "%hu", boardHeight);

        execl(viewPath, viewPath, boardWidthStr, boardHeightStr, (char *)NULL);

        // si execl vuelve, es porque hubo un error
        perror("execl: Error al ejecutar el proceso vista");
        exit(EXIT_FAILURE);
    }

    return pid;
}