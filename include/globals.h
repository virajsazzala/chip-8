#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdbool.h>

#define SCALE 10

#define PC_INIT 0x200
#define DIS_H 32
#define DIS_W 64
#define REG_COUNT 16
#define STK_CAPACITY 16
#define MEM_CAPACITY 4096

extern bool draw_flag;

extern bool waiting_for_key;
extern uint8_t key_being_waited;

extern uint8_t dt;	// delay timer
extern uint8_t st;	// sound timer
extern uint8_t sp;	// stack pointer
extern uint16_t i;	// a location in mem
extern uint16_t pc; // program counter

extern uint8_t memory[MEM_CAPACITY];
extern uint8_t display[DIS_H * DIS_W];
extern uint8_t registers[REG_COUNT];
extern uint16_t stack[STK_CAPACITY];

#endif // GLOBALS_H
