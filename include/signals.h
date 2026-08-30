#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

// Recibimos dos tipos de señales:
// * SIGTERM: para terminar el juego
// * SIGUSR1: para pausar/resumir el juego

extern volatile sig_atomic_t sigtermReceived;
extern volatile sig_atomic_t sigusr1Received;

void initSignalHandlers(void);

#endif