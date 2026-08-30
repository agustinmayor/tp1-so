#include "sharedMem.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define ALL_PERMS 0666

// Función usada por el master para crear la memoria compartida
//   * retorna un puntero a la memoria compartida creada
//   * falla si ya existe

void * createSharedMem(const char * name, size_t size, int * shmFd) {
    
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, ALL_PERMS);

    if(fd == -1) { // si shm_open falla
        perror ("shm_open: Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    if(ftruncate(fd, (off_t)size) == -1) { // si creo el objeto pero no lo puedo dimensionar
        perror ("shm_open: Error al dimensionar la memoria compartida");
        close(fd);
        shm_unlink(name);
        exit(EXIT_FAILURE);
    }

    // si fue exitoso reservo memoria y mapeo el objeto antes abierto
    //  * NULL = dejo que el SO elija la dirección de memoria
    //  * size = tamaño en bytes de la memoria compartida a mapear
    //  * PROT_READ | PROT_WRITE = permisos de lectura y escritura
    //  * MAP_SHARED = cambios en la memoria compartida se reflejan en el objeto
    //  * 0 = offset en el objeto de memoria compartida (0 = desde el principio)

    void * memAddress = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(memAddress == MAP_FAILED) {
        perror ("mmap: Error al mapear la memoria compartida");
        close(fd);
        shm_unlink(name);
        exit(EXIT_FAILURE);
    }

    if(shmFd != NULL) {
        *shmFd = fd; // guardo el file descriptor si se pasa un puntero válido
    }

    return memAddress;
}

// Funciones para conectarse y desconectarse a una shared memory ya existente
// usado por vista y jugadores


void * connectSharedMem(const char * name, size_t size, int readAndWrite) {
    int flags = readAndWrite ? O_RDWR : O_RDONLY;
    int fd = shm_open(name, flags, ALL_PERMS);

    if(fd == -1) { // si shm_open falla
        perror ("shm_open: Error al conectarse a la memoria compartida");
        exit(EXIT_FAILURE);
    }

    int prot = readAndWrite ? (PROT_READ | PROT_WRITE) : PROT_READ;

    void * memAddress = mmap(NULL, size, prot, MAP_SHARED, fd, 0);

    close(fd);

    if(memAddress == MAP_FAILED) {
        perror ("mmap: Error al mapear y conectarse a la memoria compartida");
        exit(EXIT_FAILURE);
    }

    return memAddress;
}

void disconnectSharedMem(void * memAddress, size_t size) {
    // munmap = desmapear la memoria compartida
    if(munmap(memAddress, size) == -1) {
        perror("munmap: Error al desmapear la memoria compartida");
    }
}

// Función usada por el master para eliminar la memoria compartida

void removeSharedMem(const char * name, int shmFd, void * memAddress, size_t size) {
    disconnectSharedMem(memAddress, size);
    close(shmFd);

    if(shm_unlink(name) == -1) {
        perror("shm_unlink: Error al eliminar la memoria compartida");
    }
}