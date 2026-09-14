CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -Iinclude -D_POSIX_C_SOURCE=200809L -MMD -MP
LDFLAGS = -lrt -pthread

BIN_DIR = bin

MASTER_OBJ = src/master/master.o src/master/args.o src/master/gameInit.o src/master/gameLoop.o \
             src/board.o src/semaphores.o src/signals.o src/sharedMem.o
VIEW_OBJ   = src/view/view.o
PLAYER_OBJ = src/player/player.o src/player/playerAI.o src/player/playerUtils.o src/board.o src/semaphores.o src/sharedMem.o

PLAYER_BONUS_OBJ = src/player/playerBonus.o src/player/input.o src/player/playerUtils.o src/board.o src/semaphores.o src/sharedMem.o

OBJS = $(sort $(MASTER_OBJ) $(VIEW_OBJ) $(PLAYER_OBJ) $(PLAYER_BONUS_OBJ))
DEPS = $(OBJS:.o=.d)

all: master view player bonusPlayer

master: $(BIN_DIR)/master
view:   $(BIN_DIR)/view
player: $(BIN_DIR)/player
bonusPlayer: $(BIN_DIR)/bonusPlayer

$(BIN_DIR)/master: $(MASTER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(MASTER_OBJ) $(LDFLAGS)

$(BIN_DIR)/view: $(VIEW_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(VIEW_OBJ) $(LDFLAGS)

$(BIN_DIR)/player: $(PLAYER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_OBJ) $(LDFLAGS)

$(BIN_DIR)/bonusPlayer: $(PLAYER_BONUS_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(PLAYER_BONUS_OBJ) $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BIN_DIR) $(OBJS) $(DEPS)

-include $(DEPS)

.PHONY: all clean master view player bonusPlayer
