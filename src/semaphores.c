
#include <stdio.h>
#include <stdlib.h>
#include "game_sync.h"

// Implementaciones de header game_sync.h

void semInit(GameSync * gs, int cantPlayers) {
    // usamos sem_init de POSIX para inicializar un semaforo sin nombre
    // int sem_init (sem_t * sem, int pshared, unsigned int value);

    // inicializamos semaforos y vemos si hay algun error
    // pshared = 1 para que sea compartido entre procesos
    if(sem_init(&gs->viewSignal, 1, 0) == -1 || sem_init(&gs->viewUpdated, 1, 0) == -1 ||
       sem_init(&gs->masterMutex, 1, 1) == -1 || sem_init(&gs->gameStateMutex, 1, 1) == -1 ||
       sem_init(&gs->readersMutex, 1, 1) == -1) 
    {
        perror("sem_init"); // error por stderr
        exit(EXIT_FAILURE);
    }

    gs->cantReaders = 0;

    // inicializo semaforos con cada player para que puedan enviar un movimiento
    for(int i = 0; i < cantPlayers; i++) {
        if(sem_init(&gs->playerTurn[i], 1, 1) == -1) {
            perror("sem_init playerTurn");
            exit(EXIT_FAILURE);
        }
    }
}


void semDestroy(GameSync * gs, int cantPlayers) {
    // usamos sem_destroy de POSIX para destruir un semaforo sin nombre
    // int sem_destroy (sem_t * sem);

    sem_destroy(&gs->viewSignal);
    sem_destroy(&gs->viewUpdated);
    sem_destroy(&gs->masterMutex);
    sem_destroy(&gs->gameStateMutex);
    sem_destroy(&gs->readersMutex);

    for(int i = 0; i < cantPlayers; i++) {
        sem_destroy(&gs->playerTurn[i]);
    }
}

// funciones para el master cuando va a escribir en el GameState
//      * mientras master escribe ningun jugador puede leer el GameState
//      * si el master esta esperando para escribir, ningun nuevo jugador puede "colarse"
void writerLock(GameSync * gs) {
    sem_wait(&gs->masterMutex);
    sem_wait(&gs->gameStateMutex);
    sem_post(&gs->masterMutex);
}

void writerUnlock(GameSync * gs){
    sem_post(&gs->gameStateMutex);
}

// funciones para los jugadores cuando van a leer el GameState
//      * mientras algun jugador lee, el master no puede escribir en el GameState
void readerLock(GameSync * gs) {

    // evito inanicion y doy prioridad al master si quiere escribir
    sem_wait(&gs->masterMutex);
    sem_post(&gs->masterMutex);

    sem_wait(&gs->readersMutex);
    gs->cantReaders++;

    if(gs->cantReaders == 1) { // si soy el primer lector
        // bloqueo al escritor hasta que todos los lectores terminen de leer
        sem_wait(&gs->gameStateMutex);
    }
    sem_post(&gs->readersMutex);
}

void readerUnlock(GameSync * gs) {
    sem_wait(&gs->readersMutex);
    gs->cantReaders--;

    if(gs->cantReaders == 0) { // si soy el ultimo lector
        // libero al escritor
        sem_post(&gs->gameStateMutex);
    }
    sem_post(&gs->readersMutex);
}