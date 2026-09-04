#include "args.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void exitWithUsage(const char * programName) {
    fprintf(stderr, "El formato debe ser: %s [-w width][-h height][-d delay][-t timeout][-s seed][-v vista] -p jugador1 [jugador2 ...]\n", programName);
    exit(EXIT_FAILURE);
}

static unsigned int parseUnsigned(const char * text, const char * optionName, const char * programName) {
    char * firstInvalidChar;
    unsigned long value = strtoul(text, &firstInvalidChar, 10);

    if(*text == '\0' || *firstInvalidChar != '\0') {
        fprintf(stderr, "Valor invalido para %s: %s\n", optionName, text);
        exitWithUsage(programName);
    }

    return (unsigned int)value;
}

static void addPlayerPath(MasterArgs * args, char * path, const char * programName) {
    if(args->cantPlayers >= MAX_PLAYERS) {
        fprintf(stderr, "Cantidad de jugadores invalida. El maximo es %d\n", MAX_PLAYERS);
        exitWithUsage(programName);
    }
    args->playerPaths[args->cantPlayers++] = path;
}

void parseArgs(int argc, char * argv[], MasterArgs * args) {
    args->width = DEFAULT_WIDTH;
    args->height = DEFAULT_HEIGHT;
    args->delay = DEFAULT_DELAY;
    args->timeout = DEFAULT_TIMEOUT;
    args->seed = (unsigned int)time(NULL);
    args->viewPath[0] = '\0';
    args->hasView = false;
    args->cantPlayers = 0;

    int opt;

    while((opt = getopt(argc, argv, "w:h:d:t:s:v:p:i")) != -1) {
        switch(opt) {
            case 'w': args->width = (unsigned short)parseUnsigned(optarg, "-w", argv[0]); break;
            case 'h': args->height = (unsigned short)parseUnsigned(optarg, "-h", argv[0]); break;
            case 'd': args->delay = parseUnsigned(optarg, "-d", argv[0]); break;
            case 't': args->timeout = parseUnsigned(optarg, "-t", argv[0]); break;
            case 's': args->seed = parseUnsigned(optarg, "-s", argv[0]); break;
            case 'v':
                strncpy(args->viewPath, optarg, sizeof(args->viewPath) - 1);
                args->viewPath[sizeof(args->viewPath) - 1] = '\0';
                args->hasView = true;
                break;
            case 'p':
                addPlayerPath(args, optarg, argv[0]);
                while(optind < argc && argv[optind][0] != '-') {
                    addPlayerPath(args, argv[optind], argv[0]);
                    optind++;
                }
                break;
            case 'i': // los chequeos de consistencia son del master provisto, aca se acepta y se ignora
                break;
            default:
                exitWithUsage(argv[0]);
        }
    }

    if(args->width < DEFAULT_WIDTH) {
        args->width = DEFAULT_WIDTH;
    }
    if(args->height < DEFAULT_HEIGHT) {
        args->height = DEFAULT_HEIGHT;
    }
    if(args->cantPlayers <= 0) {
        fprintf(stderr, "Hay que indicar al menos un jugador con -p\n");
        exitWithUsage(argv[0]);
    }
}
