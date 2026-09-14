/*
    Header para el manejo del playerBonus

    manejamos los inputs hechos por teclado por el usuario
*/

#ifndef INPUT_H
#define INPUT_H

#include "playerAI.h"

// seteamos la entrada estandar sin buff de linea y sin eco
// para poder leer las teclas sueltas sin esperar un enter
bool enableRawInputMode();

// volvemos a la config original de la terminal
void disableRawInputMode();

// blockeamos hasta que llegue W, A, S o D
// cuando llega devolvemos direccion
unsigned char chooseMoveFromInput(const GameSnapshot * snapshot, int myIndex, const GameState * gs);

#endif