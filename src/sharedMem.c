#include "sharedMem.h"

void * createSharedMem(const char * name, size_t size, int * shmFd) {
    // Implementación pendiente para crear la memoria compartida
    return NULL;
}

void * connectSharedMem(const char * name, size_t size, int readAndWrite) {
    // Implementación pendiente para conectarse a la memoria compartida
    return NULL;
}

void disconnectSharedMem(void * ptr, size_t size) {
    // Implementación pendiente para desconectarse de la memoria compartida
}

void removeSharedMem(const char * name, int shmFd, void * ptr, size_t size) {
    // Implementación pendiente para eliminar la memoria compartida
}