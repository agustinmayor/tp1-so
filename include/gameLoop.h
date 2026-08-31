#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "game_state.h"
#include "game_sync.h"
#include "master.h"

// Método principal del juego.
// Acá se corre toda la lógica del loop principal del juego.

void runGame(GameState * gs, GameSync * sync, const MasterArgs * args, int playersFds[], int * lastPlayerPlayed);


// Método que se llama cuando el juego termina.
// Se encarga de: 
//   * notificar a los jugadores y a la vista que el juego termino
//   * esperar a que los jugadores y la vista terminen de correr
//   * limpiar los recursos de sincronización (semaforos)

void gameOver(GameState * gs, GameSync * sync, const MasterArgs * args, pid_t playersPids[], int playersFds[], pid_t viewPid);

#endif
