#define _POSIX_C_SOURCE 200809L
#include "signals.h"
#include <string.h>

volatile sig_atomic_t sigtermReceived = 0;
volatile sig_atomic_t sigusr1Received = 0;

// Handlers de las dos tipos de señales

static void sigtermHandler(int sig) {
    (void)sig; // evitamos el warning de variable no utilizada
    sigtermReceived = 1;
}

static void sigusr1Handler(int sig) {
    (void)sig; // evitamos el warning de variable no utilizada
    sigusr1Received = 1;
}

void initSignalHandlers() {
    
    struct sigaction termAction;
    termAction.sa_handler = sigtermHandler;
    sigemptyset(&termAction.sa_mask);
    termAction.sa_flags = 0;
    sigaction(SIGTERM, &termAction, NULL);

    struct sigaction usr1Action;
    usr1Action.sa_handler = sigusr1Handler;
    sigemptyset(&usr1Action.sa_mask);
    usr1Action.sa_flags = 0;
    sigaction(SIGUSR1, &usr1Action, NULL);

}