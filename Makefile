# Source Files ###############################################
cnake/name  := cnake
cnake/srcs  := $(cnake/name)/*.c
cnake/exe   := $(cnake/name)/$(cnake/name)
cnake/debug := $(cnake/exe)_debug

# Flags ######################################################
CC          := gcc
CFLAGS      := -std=c9x -Og
PROD        := -DNDEBUG
DEBUG       := -g -Wall -Wextra
LIB         := -lncurses

# Compilation ################################################
.PHONY: $(cnake/exe) $(cnake/debug)

$(cnake/exe): $(cnake/srcs)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(PROD)

debug: $(cnake/debug)
$(cnake/debug): $(cnake/srcs)
	$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(DEBUG)

# Installation ##############################################
.PHONY: cnake/install cnake/debug

# ...

# Maintenance ###############################################
.PHONY: cnake/clean

clean: cnake/clean
cnake/clean:
	rm -f $(cnake/exe)
