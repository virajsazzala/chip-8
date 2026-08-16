#include "globals.h"
#include "display.h"

int init_screen(Screen *screen) {
	SDL_Init(SDL_INIT_VIDEO);
	
	screen->window = SDL_CreateWindow(
		"CHIP-8",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		DIS_W * SCALE,
		DIS_H * SCALE,
		SDL_WINDOW_SHOWN
	);

	screen->renderer = SDL_CreateRenderer(screen->window, -1, SDL_RENDERER_ACCELERATED);

	if (screen->renderer == NULL) {
		SDL_DestroyWindow(screen->window);
		printf("Failed to create rendered %s\n", SDL_GetError());
		return 1;
	}

	return 0;
}

void kill_screen(Screen *screen) {
	SDL_DestroyRenderer(screen->renderer);
	SDL_DestroyWindow(screen->window);
	SDL_Quit();
}

void render(SDL_Renderer *renderer) {
	// set screen to black
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// draw white pixel for all on pixels
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	for (int y = 0; y < DIS_H; y++) {
		for (int x = 0; x < DIS_W; x++) {
			if (display[y * DIS_W + x]) {
				SDL_Rect pixel = {
					x * SCALE, 	// screen x
					y * SCALE,	// screen y
					SCALE,		// width 
					SCALE		// height
				};
				SDL_RenderFillRect(renderer, &pixel);
			}
		}
	}

	SDL_RenderPresent(renderer);
}

