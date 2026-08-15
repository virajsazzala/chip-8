CC     = gcc
CFLAGS = -Wall -Wextra -g -O0 -Iinclude $(shell sdl2-config --cflags)
LIBS   = $(shell sdl2-config --libs)

SRC	   = src/main.c src/display.c
OBJS   = $(patsubst src/%.c, build/%.o, $(SRC))
BIN    = build/chip8

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

clean:
	rm -rf build	
