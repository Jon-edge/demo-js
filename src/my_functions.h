// src/my_functions.h

#pragma once

#include "mgos.h"
#include "mgos_neopixel.h"

#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H

// bool load_font_data(void);

const uint8_t *get_char_data(char c);
int get_pixel_coordinates(const char *text, char *coords_str, int coords_size);

void neopixel_print_string(struct mgos_neopixel *strip, const char *text, int x_coord, int y_coord);
void neopixel_print_string_wrapper(const char *text, int x_coord, int y_coord);


// Font definition
static const uint8_t FONT_DATA[] = {
  0x7C, 0x12, 0x12, 0x12, 0x7C, // Character A
  0x7E, 0x4A, 0x4A, 0x4A, 0x34, // Character B
  0x3C, 0x42, 0x42, 0x42, 0x24  // Character C
};



// void print_string(const char *input);

#endif // MY_FUNCTIONS_H