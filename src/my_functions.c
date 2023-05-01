#include "mgos.h"
#include "mjs.h"
#include "mgos_vfs.h"
#include "mgos_neopixel.h"
#include "my_functions.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define CHAR_MAX_PIXELS (CHAR_WIDTH * CHAR_HEIGHT)
#define SPACE_BETWEEN_CHARS 1
#define SPACE_WIDTH 3

#define PANEL_WIDTH 32
#define PANEL_HEIGHT 16
#define NUM_PIXELS PANEL_WIDTH *PANEL_HEIGHT
#define NEOPIXEL_PIN 5

static struct mgos_neopixel *s_strip = NULL;

enum mgos_app_init_result mgos_app_init(void)
{
  // Initialize NeoPixel strip
  s_strip = mgos_neopixel_create(NEOPIXEL_PIN, NUM_PIXELS, MGOS_NEOPIXEL_ORDER_GRB);
  if (!s_strip)
  {
    LOG(LL_ERROR, ("Error initializing neopixel strip"));
    return MGOS_APP_INIT_ERROR;
  }

  return MGOS_APP_INIT_SUCCESS;
}

// Utils
void round_to_precision(char *result_str, double number, int precision, size_t result_size) {
  snprintf(result_str, result_size, "%.*f", precision, number);
}

/** 
 * Random integer, inclusive 
 * */
int rand_int(int min, int max, int seed) {
    // Seed the random number generator with the current time
    //srand(time(NULL));

    return seed % (max - min + 1) + min;
}

// Pixels

void neopixel_set_string(const char *str, int x_offset, int y_offset, int r, int g, int b)
{
  // HACK: Cap at 5 letters
  int strLen = strlen(str);
  if (strLen >= 5) {
    strLen = 5;
  }

  int *coords = (int *)malloc(CHAR_MAX_PIXELS * strLen * sizeof(int) * 2);
  int coord_count = 0;
  int char_offset_x = 0;

  for (int i = 0; i < strLen ; i++)
  {
    char c = str[i];

    if (c != ' ')
    {
      const uint8_t *char_data = get_char_data(c);

      for (int x = 0; x < CHAR_WIDTH; x++)
      {
        for (int y = 0; y < CHAR_HEIGHT; y++)
        {
          if ((char_data[x] >> (CHAR_HEIGHT - 1 - y)) & 0x01)
          {
            coords[coord_count * 2] = char_offset_x + x + x_offset;
            coords[coord_count * 2 + 1] = y + y_offset;
            coord_count++;
          }
        }
      }
      int charSpace = SPACE_BETWEEN_CHARS;
      if (c == '.') {
        charSpace = 0;
      }
      char_offset_x += (CHAR_WIDTH + charSpace);
    }
    else
    {
      char_offset_x += SPACE_WIDTH;
    }
  }

  neopixel_set_pixels(coords, coord_count, r, g, b);

  // Free the allocated memory for coords
  free(coords);
}

// Function to get the font data for a specific character
const uint8_t *get_char_data(char c)
{
  int index = 0;
  uint8_t char_offset;

  if (c >= '0' && c <= '9')
  {
    char_offset = c - '0' + 52; // Numbers start at index 52
  }
  else if (c >= 'A' && c <= 'Z')
  {
    char_offset = c - 'A';
  }
  else if (c >= 'a' && c <= 'z')
  {
    char_offset = c - 'a' + 26;
  }
  else
  {
    switch (c)
    {
      case '%': char_offset = 62; break;
      case '-': char_offset = 63; break;
      case '+': char_offset = 64; break;
      case '.': char_offset = 65; break;
      default: char_offset = 0; break; // Return 0 index if character not found
    }
  }

  return &FONT_DATA[char_offset * 5];
}

bool neopixel_set_pixels(int *coords, int count, int r, int g, int b)
{
  int *x_coords = (int *)malloc(count * sizeof(int));
  int *y_coords = (int *)malloc(count * sizeof(int));

  for (int i = 0; i < count; i++)
  {
    int x_coord = coords[i * 2];
    int y_coord = coords[i * 2 + 1];
    x_coords[i] = x_coord;
    y_coords[i] = y_coord;

    // Start from what we define as x:0, y:0
    // This is physically at the end of the strip, bottom left of the panel
    int led_index = PANEL_WIDTH * PANEL_HEIGHT - 1;
    int init = led_index;

    // Get to the bottom of the correct column.
    // Move back along the strip a column at a time.
    led_index -= x_coord * PANEL_HEIGHT;
    int col_bottom0 = led_index;

    // Handle the strip's zigzag layout on the panel. Every other column is
    // reversed.
    int col_bottom1 = led_index;
    if (x_coord % 2 == 0)
    {
      // Already starting at the bottom of this column.

      // Adjust for Y: Reducing led_index moves up the panel.
      // Subtract Y to move up as Y grows
      led_index -= y_coord;
    }
    else
    {
      // Started at the top of the panel. Adjust to get to X0 by further moving 
      // back along the strip.
      led_index -= PANEL_HEIGHT - 1;
      col_bottom1 = led_index;

      // Adjust for Y: Increasing led_index moves up the panel
      led_index += y_coord;
    }

    // debug
    // printf("x: %d, y: %d, init: %d, colBottom0: %d, colBottom1: %d, led_index: %d\n", x_coord, y_coord, init, col_bottom0, col_bottom1, led_index);

    if (led_index >= 0 && led_index < PANEL_WIDTH * PANEL_HEIGHT)
    {
      mgos_neopixel_set(s_strip, led_index, r, g, b);
    }
    else
    {
      printf("Invalid strip index: %d\n", led_index);
      return false;
    }
  }

  // Print the matrix to the console in ASCII form for debugging
  print_matrix_ascii(x_coords, y_coords, count, 32, 16);

  free(x_coords);
  free(y_coords);

  return true;
}

void neopixel_clear() {
  mgos_neopixel_clear(s_strip);
}

void neopixel_show() {
  mgos_neopixel_show(s_strip);
}

// Debugging functions

void print_matrix_ascii(int *x_coords, int *y_coords, int count, int width, int height)
{
  for (int y = height - 1; y >= 0; y--)
  {
    for (int x = 0; x < width; x++)
    {
      bool is_pixel_set = false;

      // Check each coord for a pixel
      for (int i = 0; i < count; i++)
      {
        if (x == x_coords[i] && y == y_coords[i])
        {
          printf("X");
          is_pixel_set = true;
          break;
        }
      }

      if (!is_pixel_set)
      {
        printf(".");
      }
    }
    printf("\n");
  }
}

bool neopixel_print_pixel(int led_index)
{
  mgos_neopixel_clear(s_strip);

  if (led_index >= 0 && led_index < PANEL_WIDTH * PANEL_HEIGHT)
  {
    mgos_neopixel_set(s_strip, led_index, 25, 0, 0);
    mgos_neopixel_show(s_strip);
    return true;
  }
  else
  {
    printf("Invalid strip index: %d", led_index);
    return false;
  }
}

