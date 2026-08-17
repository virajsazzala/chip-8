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

#define OPCODES_PER_FRAME 12

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

SDL_Scancode keymap[16] = {
	SDL_SCANCODE_X, // 0
    SDL_SCANCODE_1, // 1
    SDL_SCANCODE_2, // 2
    SDL_SCANCODE_3, // 3
    SDL_SCANCODE_Q, // 4
    SDL_SCANCODE_W, // 5
    SDL_SCANCODE_E, // 6
    SDL_SCANCODE_A, // 7
    SDL_SCANCODE_S, // 8
    SDL_SCANCODE_D, // 9
    SDL_SCANCODE_Z, // A
    SDL_SCANCODE_C, // B
    SDL_SCANCODE_4, // C
    SDL_SCANCODE_R, // D
    SDL_SCANCODE_F, // E
    SDL_SCANCODE_V  // F
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

	waiting_for_key = false;
	key_being_waited = 0;

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

void draw(uint8_t vx, uint8_t vy, uint8_t n) {
	draw_flag = true;
	registers[0xF] = 0;

	uint8_t rx = vx % DIS_W;
	uint8_t ry = vy % DIS_H;

	for (int row = 0; row < n; row++) {
		int py = ry + row;
		if (py >= DIS_H) break;

		uint8_t nbyte = memory[i + row];
		for (int col = 0; col < 8; col++) {
			uint8_t pixel = (nbyte >> (7 - col)) & 1;
			if (!pixel) continue;

			int px = rx + col;
			if (px >= DIS_W) continue;

			int idx = py * DIS_W + px;
			if (display[idx]) {
				registers[0xF] = 1;
			}
			display[idx] ^= 1;
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

bool key_pressed(uint8_t key) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	return keys[keymap[key]];
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
		switch (nn) {
		// 00E0: Clear the display
		case 0xE0:
			clear_screen();
			break;

		// 00EE: Return from subroutine
		case 0xEE:
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
			registers[0xF] = 0;
			break;

		// 8XY1: Set VX = VX | VY
		case 1:
			registers[x] |= registers[y];
			registers[0xF] = 0;
			break;

		// 8XY2: Set VX = VX & VY
		case 2:
			registers[x] &= registers[y];
			registers[0xF] = 0;
			break;

		// 8XY3: Set VX = VX ^ VY
		case 3:
			registers[x] ^= registers[y];
			registers[0xF] = 0;
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
			registers[0xF] = (vx >= registers[y]) ? 1 : 0;
			break;
		}

		// 8XY6: Set VX = VX SHR 1
		// update: changing to set vy, ambiguity
		case 6: {
			uint8_t lsb = registers[y] & 1;
			registers[x] = registers[y] >> 1;
			registers[0xF] = lsb;
			break;
		}

		// 8XY7: Set VX = VY - VX, VF = not borrow
		case 7: {
			uint8_t vx = registers[x];
			registers[x] = registers[y] - vx;
			registers[0xF] = (registers[y] >= vx) ? 1 : 0;
			break;
		}

		// 8XYE: Set VX = VX SHL 1
		case 0xE: {
			uint8_t msb = (registers[y] >> 7) & 1;
			registers[x] = registers[y] << 1;
			registers[0xF] = msb;
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
		switch (nn)	{
		// EX9E: Skip next instruction if key w/ VX is pressed
		case 0x9E:
			if (key_pressed(registers[x])) {
				pc += 2;
			}

			break;

		// EXA1: Skip next instruction if key w/ VX is not pressed
		case 0xA1:
			if (!key_pressed(registers[x])) {
				pc += 2;
			}

			break;
		}
		break;

	case 0xF:
		switch (nn) {
		// FX07: VX = delay timer
		case 0x07:
			registers[x] = dt;
			break;

		// FX0A: wait for keypress+release, store in VX
		case 0x0A: {
			if (!waiting_for_key) {
				for (uint8_t k = 0; k < 16; k++) {
					if (key_pressed(k)) {
						key_being_waited = k;
						waiting_for_key = true;
						break;
					}
				}
				pc -= 2;
				break;
			} else {
				if (!key_pressed(key_being_waited)) {
					registers[x] = key_being_waited;
					waiting_for_key = false;
				} else {
					pc -= 2;
				}
			}

			break;
		}

		// FX15: delay timer = VX
		case 0x15:
			dt = registers[x];
			break;

		// FX18: sound timer = VX
		case 0x18:
			st = registers[x];
			break;

		// FX1E: I += VX
		case 0x1E:
			i += registers[x];
			break;

		// FX29: I = font address for digit VX
		case 0x29:
			i = registers[x] * 5;
			break;

		// FX33: BCD decode VX into memory[I], [I+1], [I+2]
		case 0x33:
			memory[i] = registers[x] / 100;
			memory[i+1] = (registers[x] / 10) % 10;
			memory[i+2] = registers[x] % 10;
			break;

		// FX55: store V0-VX into memory starting at I
		case 0x55:
			for (uint8_t r = 0; r <= x; r++) {
				memory[i + r] = registers[r];
			}
			i += x + 1;
			break;

		// FX65: load V0-VX from memory starting at I
		case 0x65:
			for (uint8_t r = 0; r <= x; r++) {
				registers[r] = memory[i + r];
			}
			i += x + 1;
			break;

		}
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

		// multiple opcodes per frame
		for (int j = 0; j < OPCODES_PER_FRAME; j++) {
			uint16_t opcode = fetch();
			decode_execute(opcode);
			if (draw_flag) break; // stop exec till next frame
		}

		// tick timers once per frame (60hz)
		if (dt > 0) dt--;
		if (st > 0) st--;

		if (draw_flag) {
			render(screen.renderer);
			draw_flag = false;
		}

		SDL_Delay(16); // ~60fps
	}

	kill_screen(&screen);

	return 0;
}
