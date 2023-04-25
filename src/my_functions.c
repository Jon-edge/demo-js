#include "mgos.h"
#include "mjs.h"
#include "mgos_vfs.h"
#include "mgos_neopixel.h"
#include "my_functions.h"

#include <stdint.h>
#include <string.h>

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

// Probably no longer needed.
void neopixel_print_string_wrapper(const char *text, int x_coord, int y_coord)
{
  if (s_strip)
  {
    neopixel_print_string(text, x_coord, y_coord);
  }
  else
  {
    LOG(LL_ERROR, ("NeoPixel strip not initialized"));
  }
}

void neopixel_print_string(const char *str, int x_offset, int y_offset)
{
  int *coords = (int *)malloc(CHAR_MAX_PIXELS * strlen(str) * sizeof(int) * 2);
  int coord_count = 0;
  int char_offset_x = 0;

  for (int i = 0; i < strlen(str); i++)
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
      char_offset_x += (CHAR_WIDTH + SPACE_BETWEEN_CHARS);
    }
    else
    {
      char_offset_x += SPACE_WIDTH;
    }
  }

  neopixel_print_pixels(coords, coord_count);

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
    char_offset = c - '0' + 52; // Numbers start at index 26
  }
  else
  {
    char_offset = c - 'A';
    if (char_offset > 25)
      char_offset = c - 'a';
  }

  return &FONT_DATA[char_offset * 5];
}

bool neopixel_print_pixels(int *coords, int count)
{
  mgos_neopixel_clear(s_strip);

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
    printf("x: %d, y: %d, init: %d, colBottom0: %d, colBottom1: %d, led_index: %d\n", x_coord, y_coord, init, col_bottom0, col_bottom1, led_index);

    if (led_index >= 0 && led_index < PANEL_WIDTH * PANEL_HEIGHT)
    {
      mgos_neopixel_set(s_strip, led_index, 25, 25, 25); // Full brightness white color
    }
    else
    {
      printf("Invalid strip index: %d\n", led_index);
      return false;
    }
  }

  mgos_neopixel_show(s_strip);

  // Print the matrix to the console in ASCII form for debugging
  print_matrix_ascii(x_coords, y_coords, count, 32, 16);

  free(x_coords);
  free(y_coords);

  return true;
}

// Debugging functions

void test_neopixel_print_pixels()
{
  int test_data[][2] = {
      {1, 15}, {2, 15}, {3, 15}, {0, 14}, {4, 14}, {0, 13}, {1, 13}, {2, 13}, {3, 13}, {4, 13}, {0, 12}, {4, 12}, {0, 11}, {4, 11}};

  int count = sizeof(test_data) / sizeof(test_data[0]);
  int *coords = (int *)test_data;

  bool result = neopixel_print_pixels(coords, count);
  printf("Result: %d\n", result);
}

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