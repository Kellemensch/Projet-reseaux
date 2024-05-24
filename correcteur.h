#ifndef CORRECTEUR
#define CORRECTEUR

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define POLYNOME 0xD5

uint16_t chg_nth_bit(int n, uint16_t m);

uint16_t set_nth_bit(int n, uint16_t m);

uint8_t get_nth_bit(int n, uint8_t m);

void print_binary_8bit(uint8_t value);

void print_binary_16bit(uint16_t value);

void print_word(int k, uint16_t value);

uint16_t encode_G(uint16_t m);

int cardinal_bit(uint16_t m);

int min_hamming_distance(uint16_t generator);

uint8_t crcGeneration(uint8_t m);

uint8_t crcVerif(uint16_t m);

int crc_error_amount(uint16_t m);

#endif