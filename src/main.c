#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PC_INIT 0x200
#define DIS_H 32
#define DIS_W 64
#define REG_COUNT 16
#define STK_CAPACITY 16
#define MEM_CAPACITY 4096

uint8_t dt;			   // delay timer
uint8_t st;			   // sound timer
uint8_t sp;			   // stack pointer
uint16_t i;			   // a location in mem
uint16_t pc = PC_INIT; // program counter

uint8_t memory[MEM_CAPACITY];
uint8_t display[DIS_H * DIS_W];
uint8_t registers[REG_COUNT];
uint16_t stack[STK_CAPACITY];

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
		if (ins == 0x00E0) {
			clear_screen();
		}
		break;

	case 0x1:
		// 1NNN - Jump to location nnn
		pc = nnn;
		break;

	case 0x2:
		break;

	case 0x3:
		break;

	case 0x4:
		break;

	case 0x5:
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
		break;

	case 0x9:
		break;

	case 0xA:
		// ANNN: Set I = NNN
		i = nnn;
		break;

	case 0xB:
		break;

	case 0xC:
		break;

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
	init(argv[1]);

	bool running = true;
	while (running) {
		if (pc >= MEM_CAPACITY) {
			// temporary to avoid inf loop
			running = false;
			break;
		}

		uint16_t opcode = fetch();
		printf("%04X\n", opcode);

		decode_execute(opcode);
		print_display();
	}

	printf("hello, world!\n");
	return 0;
}
