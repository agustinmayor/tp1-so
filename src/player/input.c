#include <errno.h>
#include <stdlib.h>

// para manejar la terminal
#include <termios.h>
#include <unistd.h>

#include "input.h"

static struct termios originalTermios;
static bool rawInputModeEnabled = false;

// devolvemos true si pudimos setear modo raw y false si hubo error
bool enableRawInputMode() {

    if(tcgetattr(STDIN_FILENO, &originalTermios) == -1) {
        return false;
    }

    struct termios raw = originalTermios;
    // desactivamos buffer de linea y echo
    raw.c_lflag &= ~(ICANON | ECHO);

    // seteamos la nueva config
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return false;
    }

    rawInputModeEnabled = true;
    return true;
}


void disableRawInputMode() {
    if(rawInputModeEnabled) {
        // seteamos devuelta config normal
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
        rawInputModeEnabled = false;
    }
}

// hacemos block y leemos caracteres hasta tener W, A, S o D (movimientos validos)
unsigned char chooseMoveFromInput(const GameSnapshot * snapshot, int myIndex) {

    // para bypaseear warnings
    (void)snapshot;
    (void)myIndex;

    while(1) {
        char c;
        ssize_t bytesRead = read(STDIN_FILENO, &c, 1);

        if(bytesRead == -1) {
            if(errno == EINTR) {
                continue;
            }

            exit(EXIT_FAILURE);
        }

        if(bytesRead == 0) {
            // stdin cerrado
            exit(EXIT_SUCCESS);
        }


        switch(c) {
            case 'w': case 'W': return 0; // arriba
            case 'd': case 'D': return 2; // derecha
            case 's': case 'S': return 4; // abajo
            case 'a': case 'A': return 6; // izquierda
            default: continue; // ignoramos cualquier otra tecla
        }
    }
}