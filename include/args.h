#ifndef ARGS_H
#define ARGS_H

#include "master.h"

// Parsea la linea de comandos del master y aborta con un mensaje de uso si es invalida.
void parseArgs(int argc, char * argv[], MasterArgs * args);

#endif
