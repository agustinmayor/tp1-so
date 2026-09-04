#ifndef GAME_SYNC_H
#define GAME_SYNC_H

#include <semaphore.h>

// struct de semaforos para la sincronizacion

#define SHM_GAME_SYNC_NAME "/game_sync"

typedef struct {
   sem_t viewSignal; // el master le indica a la vista que hay cambios por imprimir (0)
   sem_t viewUpdated; // La vista le indica al master que termino de imprimir (0)
   sem_t masterMutex; // Mutex para evitar inanicion del master al acceder al estado (1)
   sem_t gameStateMutex; // Mutex para el estado del juego (1)
   sem_t readersMutex; // Mutex para proteger la siguiente variable (1)
   unsigned int cantReaders; // cant de jugadores leyendo el estado (0)
   sem_t playerTurn[9]; //indica a cada jugador que puede enviar 1 movimiento
} GameSync;


// inicializa todos los semaforos
void semInit(GameSync * gs, int cantPlayers);

// libera los recursos de los semaforos
// llamada por el master cuando se finaliza el juego
void semDestroy(GameSync * gs, int cantPlayers);

// funciones para el master cuando va a cambiar el GameState
void writerLock(GameSync * gs);
void writerUnlock(GameSync * gs);

// funciones para los jugadores cuando van a leer el GameState
void readerLock(GameSync * gs);
void readerUnlock(GameSync * gs);

#endif
