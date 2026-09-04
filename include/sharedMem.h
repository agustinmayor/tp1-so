#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stddef.h>

#define SHM_READ_ONLY 0
#define SHM_READ_WRITE 1

// Función usada por el master para crear la memoria compartida
//   * retorna un puntero a la memoria compartida creada
//   * falla si ya existe

void * createSharedMem(const char * name, size_t size, int * shmFd);

// Funciones para conectarse y desconectarse a una shared memory ya existente
// usado por vista y jugadores

void * connectSharedMem(const char * name, size_t size, int accessMode);

void disconnectSharedMem(void * ptr, size_t size);

// Función usada por el master para eliminar la memoria compartida

void removeSharedMem(const char * name, int shmFd, void * ptr, size_t size);

#endif
