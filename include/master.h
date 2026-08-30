#ifndef MASTER_H
#define MASTER_H

#include <stdbool.h>

#define MAX_PLAYERS 9
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

#define SHM_GAME_STATE_NAME "/game_state"
#define SHM_GAME_SYNC_NAME "/game_sync"

// struct para los argumentos pasados al master
typedef struct {
    unsigned short width, height;
    unsigned int delay; // ms que espera el master entre cada impresion de estado, default 200 ms
    unsigned int timeout; // timeout en segundos para recibir solicitudes de movimientos validos, defaut 10 s
    unsigned int seed; // usada para generar el tablero, default time(NULL)
    char viewPath[256]; // path del archivo donde se imprimira el estado del juego, default sin vista
    bool hasView;
    char * playerPaths[MAX_PLAYERS]; // paths de los ejecutables de los jugadores
    int cantPlayers;
} MasterArgs;

#endif