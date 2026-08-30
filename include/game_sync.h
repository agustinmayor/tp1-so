#ifndef GAME_SYNC_H
#define GAME_SYNC_H

#include <semaphore.h>

// struct de semaforos para la sincronizacion

typedef struct {
   sem_t viewSignal; // el master le indica a la vista que hay cambios por imprimir (0)
   sem_t viewUpdated; // La vista le indica al master que termino de imprimir (0)
   sem_t masterMutex; // Mutex para evitar inanicion al master que termino de imprimir (0)
   sem_t gameStateMutex; // Mutex para el estado del juego (1)
   sem_t readersMutex; // Mutex para proteger la siguiente variable (1)
   unsigned int cantReaders; // cant de jugadores leyendo el estado (0)
   sem_t playerAllowed[9]; //indica a cada jugador que puede enviar 1 movimiento 
} GameSync;


#endif