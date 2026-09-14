/**
 * 
 * BONUS:
 * 
 * Implementamos un jugador que no es IA sino que
 * es el usuario de la terminal
 * 
 * Se reciben los movimientos que son W, A, S y D
 * 
 * Limitacion: no se puede ir en diagonal 
 */

 #include "input.h"
 #include "playerUtils.h"

 #include <stdio.h>
 #include <stdlib.h>
 #include <signal.h>

 static void handleTerminatingSignals(int sig) {
    disableRawInputMode();

    signal(sig, SIG_DFL);
    raise(sig);
 }

 int main(int argc, char * argv[]) {
    if(!enableRawInputMode()) {
        // no se pudo setear la terminal para jugar
        perror("tcsetattr: No se pudo configurar la terminal para jugar en modo raw");
        return EXIT_FAILURE;
    }

    // al salir del programa se restaura la config normal de la terminal
    atexit(disableRawInputMode);

    struct sigaction sa;
    sa.sa_handler = handleTerminatingSignals;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // delego a playerUtils.c el loop del jugador
    // le paso mi manera de decidir los movimientos via WASD
    return runPlayerLoop(argc, argv, chooseMoveFromInput);
 }