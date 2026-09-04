#ifndef MASTER_H
#define MASTER_H

#include "game_state.h"
#include "game_sync.h"

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

// struct para los argumentos pasados al master
typedef struct {
    unsigned short width, height;
    unsigned int delay; // ms que espera el master entre cada impresion de estado, default 200 ms
    unsigned int timeout; // timeout en segundos para recibir solicitudes de movimientos validos, default 10 s
    unsigned int seed; // usada para generar el tablero, default time(NULL)
    char viewPath[256]; // path del binario de la vista, default sin vista
    bool hasView;
    char * playerPaths[MAX_PLAYERS]; // paths de los ejecutables de los jugadores
    int cantPlayers;
} MasterArgs;

#endif
