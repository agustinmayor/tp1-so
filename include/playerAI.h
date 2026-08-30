#ifndef PLAYER_AI_H
#define PLAYER_AI_H

#include <structures.h>
#include <unistd.h>
#include <semaphores.h>
#include <shmState.h>
#include <shmSync.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

/* Busca el indice del jugador actual comparando PIDs. */
int findMyIndex(const GameState *state);

/* Calcula el mejor movimiento usando estrategia greedy (mayor valor adyacente). */
unsigned char findBestMove(const GameState *state, int myIndex);

/* Chequea la validez de los argumentos width y height. */
int checkArguments(char * argv[], size_t * width, size_t *height);

/* Ejecuta el bucle principal del jugador. */
void runLoop(const GameState *state, SyncData *sync, int myIndex);

#endif