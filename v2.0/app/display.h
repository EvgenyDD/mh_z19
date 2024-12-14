#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DIG_COUNT 4

//                                       ________B__FA____GC__EDH________
#define PATTRN_CHR_0 0x00982600 //     0b00000000100110000010011000000000
#define PATTRN_CHR_1 0x00802000 //     0b00000000100000000010000000000000
#define PATTRN_CHR_2 0x00884600 //     0b00000000100010000100011000000000
#define PATTRN_CHR_3 0x00886200 //     0b00000000100010000110001000000000
#define PATTRN_CHR_4 0x00906000 //     0b00000000100100000110000000000000
#define PATTRN_CHR_5 0x00186200 //     0b00000000000110000110001000000000
#define PATTRN_CHR_6 0x00186600 //     0b00000000000110000110011000000000
#define PATTRN_CHR_7 0x00882000 //     0b00000000100010000010000000000000
#define PATTRN_CHR_8 0x00986600 //     0b00000000100110000110011000000000
#define PATTRN_CHR_9 0x00986200 //     0b00000000100110000110001000000000
//                                     ________B__FA____GC__EDH________
#define PATTRN_CHR_A 0x00986400 //     0b00000000100110000110010000000000
#define PATTRN_CHR_B 0x00106600 //     0b00000000000100000110011000000000
#define PATTRN_CHR_C 0x00180600 //     0b00000000000110000000011000000000
#define PATTRN_CHR_D 0x00806600 //     0b00000000100000000110011000000000
#define PATTRN_CHR_E 0x00184600 //     0b00000000000110000100011000000000
#define PATTRN_CHR_F 0x00184400 //     0b00000000000110000100010000000000
#define PATTRN_CHR_h 0x00106400 //     0b00000000000100000110010000000000
#define PATTRN_CHR_H 0x00906400 //     0b00000000100100000110010000000000
#define PATTRN_CHR_L 0x00100600 //     0b00000000000100000000011000000000
#define PATTRN_CHR_n 0x00006400 //     0b00000000000000000110010000000000
#define PATTRN_CHR_o 0x00006600 //     0b00000000000000000110011000000000
#define PATTRN_CHR_P 0x00984400 //     0b00000000100110000100010000000000
#define PATTRN_CHR_r 0x00004400 //     0b00000000000000000100010000000000
#define PATTRN_CHR_t 0x00104600 //     0b00000000000100000100011000000000
#define PATTRN_CHR_u 0x00002600 //     0b00000000000000000010011000000000
#define PATTRN_CHR_Y 0x00906200 //     0b00000000100100000110001000000000
//                                     ________B__FA____GC__EDH________
#define PATTRN_CHR_MINUS 0x00004000 // 0b00000000000000000100000000000000
#define PATTRN_CHR_UNDSC 0x00000200 // 0b00000000000000000000001000000000
#define PATTRN_CHR_UPPSC 0x00080000 // 0b00000000000010000000000000000000

void display_init(void);

void display_set_pwm(uint32_t digit, float level);
void display_reset(uint32_t digit);
void display_set_num(uint32_t digit, uint8_t num);
void display_set_symbol(uint32_t digit, uint32_t symbol);
void display_set_dot(uint32_t digit, bool state);

extern const uint32_t pattrn_dig[10];

#endif // DISPLAY_H