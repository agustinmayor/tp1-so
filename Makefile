CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -Iinclude -D_POSIX_C_SOURCE=200809L -MMD -MP
LDFLAGS = -lrt -pthread

BIN_DIR = bin

MASTER_OBJ = src/master/master.o src/master/gameInit.o src/semaphores.o src/signals.o src/sharedMem.o
VIEW_OBJ   = src/view/view.o
PLAYER_OBJ = src/player/player.o src/semaphores.o src/sharedMem.o

OBJS = $(sort $(MASTER_OBJ) $(VIEW_OBJ) $(PLAYER_OBJ))
DEPS = $(OBJS:.o=.d)

all: master view player

master: $(BIN_DIR)/master
view:   $(BIN_DIR)/view
player: $(BIN_DIR)/player

$(BIN_DIR)/master: $(MASTER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(MASTER_OBJ) $(LDFLAGS)

$(BIN_DIR)/view: $(VIEW_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(VIEW_OBJ) $(LDFLAGS)

$(BIN_DIR)/player: $(PLAYER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_OBJ) $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BIN_DIR) $(OBJS) $(DEPS)

-include $(DEPS)

.PHONY: all clean master view player


## Es para  testear con 9 players 
##docker run -it --rm -v "/Users/gabo/Otros/Proyectos/UNI/SO/tp1-so:/tp" -v "/Users/gabo/Desktop:/provisto:ro" -w /tmp agodio/itba-so-multiarch:3.1 bash -c 'cp /provisto/ChompChamps-2 /tmp/cc && chmod +x /tmp/cc && : > 24 && : > 12 && for i in 0 1 2 3 4 5 6 7 8; do a=$((i % 8)); b=$(((i + 2) % 8)); : > $i; for n in $(seq 1 12); do printf "\\x0$a\\x0$b" >> $i; done; done && make -C /tp clean && make -C /tp view && /tmp/cc -v /tp/bin/view -p /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat /usr/bin/cat -i -s 7 -w 24 -h 12 -d 80'