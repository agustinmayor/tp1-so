// main archivo de los procesos player por IA

#include "playerAI.h" // aca esta chooseMove
#include "playerUtils.h" // aca esta runPlayerLoop

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    return runPlayerLoop(argc, argv, chooseMove);
}
