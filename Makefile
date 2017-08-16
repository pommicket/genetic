CC=gcc
CFLAGS=`pkg-config --libs --cflags sdl2 SDL2_ttf gtk+-3.0` -lm -lpthread -Wall -Wno-switch -Wno-parentheses
OUTPUT=bin/GENETIC

default: all

all: src/main.c
	$(CC) src/main.c $(CFLAGS) -o $(OUTPUT)

run: all
	./$(OUTPUT)
