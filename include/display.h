#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
	SDL_Renderer *renderer;
	SDL_Window *window;
} Screen;

int init_screen(Screen *);
void kill_screen(Screen *);
void render(SDL_Renderer *);

#endif // DISPLAY_H
