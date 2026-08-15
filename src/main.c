#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
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

int main(int argc, char **argv) {
	assert(argc == 2);
	init(argv[1]);
	
	bool running = true;
	while (running) {
		uint16_t opcode = fetch();
		printf("%04X\n", opcode);
	}
	
	printf("hello, world!\n");
	return 0;
}
