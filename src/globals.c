#include "globals.h"

uint8_t dt;
uint8_t st;
uint8_t sp;
uint16_t i;
uint16_t pc;
uint8_t memory[MEM_CAPACITY];
uint8_t display[DIS_H * DIS_W];
uint8_t registers[REG_COUNT];
uint16_t stack[STK_CAPACITY];
