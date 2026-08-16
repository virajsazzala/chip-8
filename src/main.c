#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "display.h"
#include "globals.h"

uint8_t fonts[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};


void init(const char *fpath) {
	/* reset state */
	dt = 0;
	st = 0;
	sp = 0;
	i = 0;
	memset(memory, 0, sizeof(memory));
	memset(display, 0, sizeof(display));
	memset(registers, 0, sizeof(registers));
	memset(stack, 0, sizeof(stack));

	/* set program counter to 0x200 */
	pc = PC_INIT;

	/* copy fonts to memory */
	size_t font_size = sizeof(fonts);
	memcpy(memory, fonts, font_size);

	/* copy program to memory */
	FILE *fp = fopen(fpath, "rb");
	assert(fp != NULL);

	struct stat st;
	stat(fpath, &st);
	size_t fsize = st.st_size;
	assert(fsize <= MEM_CAPACITY - PC_INIT);

	fread(memory + PC_INIT, 1, fsize, fp);

	fclose(fp);
}

uint16_t fetch() {
	uint16_t ins = (memory[pc] << 8) | memory[pc + 1];
	pc += 2;
	return ins;
}

void clear_screen() { memset(display, 0, (DIS_H * DIS_W)); }

void draw(uint8_t rx, uint8_t ry, uint8_t n) {
	registers[0xF] = 0;
	for (int row = 0; row < n; row++) {
		uint8_t nbyte = memory[i + row];
		for (int col = 0; col < 8; col++) {
			uint8_t pixel = (nbyte >> (7 - col)) & 1;
			if (pixel) {
				int px = (rx + col) % DIS_W;
				int py = (ry + row) % DIS_H;
				int idx = py * DIS_W + px;
				if (display[idx]) {
					registers[0xF] = 1;
				}
				display[idx] ^= 1;
			}
		}
	}
}

void print_display() {
	printf("\033[H\033[2J"); // move cursor to top, clear screen
	for (int y = 0; y < DIS_H; y++) {
		for (int x = 0; x < DIS_W; x++) {
			printf(display[y * DIS_W + x] ? "#" : ".");
		}
		printf("\n");
	}
}

void decode_execute(uint16_t ins) {
	uint8_t op = (ins >> 12) & 0xF;
	uint8_t x = (ins >> 8) & 0xF;
	uint8_t y = (ins >> 4) & 0xF;
	uint8_t n = ins & 0xF;
	uint8_t nn = ins & 0xFF;
	uint16_t nnn = ins & 0xFFF;

	switch (op) {
	case 0x0:
		switch (ins) {
		// 00E0: Clear the display
		case 0x00E0:
			clear_screen();
			break;

		// 00EE: Return from subroutine
		case 0x00EE:
			pc = stack[--sp];
			break;
	
		// 0NNN: Jump to machine code routine at NNN
		// Not implemented for now, due to rare use.
		}

		break;

	case 0x1:
		// 1NNN: Jump to location NNN
		pc = nnn;
		break;

	case 0x2:
		// 2NNN: Call subroutine at NNN
		stack[sp++] = pc;
		pc = nnn;
		break;

	case 0x3:
		// 3XNN: Skip next instruction if VX = NN
		if (registers[x] == nn) {
			pc += 2;
		}
		break;

	case 0x4:
		// 4XNN: Skip next instruction if VX != NN
		if (registers[x] != nn) {
			pc += 2;
		}
		break;

	case 0x5:
		// 5XY0: Skip next instruction if VX = VY
		if (registers[x] == registers[y]) {
			pc += 2;
		}	
		break;

	case 0x6:
		// 6XNN: Set VX = NN
		registers[x] = nn;
		break;

	case 0x7:
		// 7XNN: Set VX = VX + NN
		registers[x] += nn;
		break;

	case 0x8:
		switch (n) {
		// 8XY0: Set VX = VY
		case 0:
			registers[x] = registers[y];
			break;

		// 8XY1: Set VX = VX | VY
		case 1:
			registers[x] |= registers[y];
			break;

		// 8XY2: Set VX = VX & VY
		case 2:
			registers[x] &= registers[y];
			break;

		// 8XY3: Set VX = VX ^ VY
		case 3:
			registers[x] ^= registers[y];
			break;

		// 8XY4: Set VX = VX + VY, VF = carry
		case 4: {
			uint16_t sum = registers[x] + registers[y];
			registers[x] = sum & 0xFF;

			if (sum > 0xFF) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}

			break;
		}

		// 8XY5: Set VX = VX - VY, VF = not borrow
		case 5: {
			uint8_t vx = registers[x];
			registers[x] = vx - registers[y];
			registers[0xF] = (vx > registers[y]) ? 1 : 0;
			break;
		}

		// 8XY6: Set VX = VX SHR 1
		case 6: {
			uint8_t lsb = registers[x] & 1;

			if (lsb == 1) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}

			registers[x] /= 2;

			break;
		}

		// 8XY7: Set VX = VY - VX, VF = not borrow
		case 7: {
			uint8_t vx = registers[x];
			registers[x] = registers[y] - vx;
			registers[0xF] = (registers[y] > vx) ? 1 : 0;
			break;
		}

		// 8XYE: Set VX = VX SHL 1
		case 0xE: {
			uint8_t msb = (registers[x] >> 7) & 1;

			if (msb == 1) {
				registers[0xF] = 1;
			} else {
				registers[0xF] = 0;
			}

			registers[x] *= 2;

			break;
		}
		}

		break;

	case 0x9:
		// 9XY0: Skip next instruction if VX != VY
		if (registers[x] != registers[y]) {
			pc += 2;
		}	
		break;

	case 0xA:
		// ANNN: Set I = NNN
		i = nnn;
		break;

	case 0xB:
		// BNNN: Jump to location NNN + V0
		pc = nnn + registers[0];
		break;

	case 0xC: {
		// CXNN: Set VX = random byte AND NN
		uint8_t rnum =  rand() % 256; // 0-255
		registers[x] = rnum & nn;
		break;
	}

	case 0xD:
		// DXYN: Display n-byte sprite starting at memory I
		// at (VX, VY), set VF = collision
		draw(registers[x], registers[y], n);
		break;

	case 0xE:
		break;

	case 0xF:
		break;
	}
}

int main(int argc, char **argv) {
	assert(argc == 2);

	srand(time(NULL));
	init(argv[1]);

	Screen screen;

	if (init_screen(&screen) != 0) {
		return 0;
	}

	SDL_Event e;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}
		uint16_t opcode = fetch();
		decode_execute(opcode);
		render(screen.renderer);
		SDL_Delay(16);
	}

	kill_screen(&screen);

	return 0;
}
