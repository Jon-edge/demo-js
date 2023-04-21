#include "mgos.h"
#include "mgos_vfs.h"
#include "mgos_neopixel.h"
#include "my_functions.h"

#include <stdint.h>
#include <string.h>

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define SPACE_BETWEEN_CHARS 1

#define PANEL_WIDTH 32
#define PANEL_HEIGHT 16
#define NUM_PIXELS PANEL_WIDTH * PANEL_HEIGHT
#define NEOPIXEL_PIN 5

static struct mgos_neopixel *s_strip = NULL;

enum mgos_app_init_result mgos_app_init(void) {
  // Initialize NeoPixel strip
  s_strip = mgos_neopixel_create(NEOPIXEL_PIN, NUM_PIXELS, MGOS_NEOPIXEL_ORDER_GRB);
  if (!s_strip) {
    LOG(LL_ERROR, ("Error initializing neopixel strip"));
    return MGOS_APP_INIT_ERROR;
  }

  // ...

  return MGOS_APP_INIT_SUCCESS;
}

/*
enum mgos_app_init_result mgos_app_init(void) {
  if (!load_font_data()) {
    printf("Failed to load font data.\n");
    // return MGOS_APP_INIT_ERROR; // Comment out this line
  }

  return MGOS_APP_INIT_SUCCESS; // Always return success to allow the app to continue running
}

bool load_font_data() {
  // Open the font data file
  FILE *font_file = fopen(FONT_FILE_PATH, "r");
  if (!font_file) {
    printf("Failed to open font data file: %s", FONT_FILE_PATH);
    return false;
  }

  // Allocate memory for font data
  font_data_size = CHAR_WIDTH * CHAR_HEIGHT * 3;  // 3 characters
  font_data = (uint8_t *)malloc(font_data_size);
  if (!font_data) {
    printf("Failed to allocate memory for font data");
    fclose(font_file);
    return false;
  }

  // Read font data from the file
  size_t index = 0;
  while (fscanf(font_file, "%02hhx,", &font_data[index]) == 1) {
    index++;
  }
  
  for (int i = 0; i < font_data_size; i++) {
    if (i % CHAR_WIDTH == 0) {
      printf("\n")
    }
    printf("0x%02x ", font_data[i]);
  }
  printf("\n");

  fclose(font_file);
  return true;
}
*/

// Function to get the font data for a specific character
const uint8_t *get_char_data(char c) {
  int index = 0;

  if (c >= 'A' && c <= 'C') {
    index = (c - 'A') * 5; // Assuming each character takes 5 bytes
  } else {
    // Return an empty character if the input character is not supported
    static const uint8_t empty_char[5] = {0};
    return empty_char;
  }

  return &FONT_DATA[index];
}

int get_pixel_coordinates(const char *text, char *coords_str, int coords_size) {
  int text_length = strlen(text);
  int coords_index = 0;

  for (int i = 0; i < text_length; i++) {
    char c = text[i];
    int char_offset_x = i * (CHAR_WIDTH + SPACE_BETWEEN_CHARS);
    const uint8_t *char_data = get_char_data(c);
    if (char_data == NULL) {
      printf("Error: char_data is NULL for character %c\n", c);
      continue;
    }

// printf("Character: %c\n", c);
// for (int i = 0; i < CHAR_HEIGHT; i++) {
//   printf("%02x ", char_data[i]);
// }
// printf("\n");
    for (int x = 0; x < CHAR_WIDTH; x++) {
      for (int y = 0; y < CHAR_HEIGHT; y++) {
        if (coords_index < coords_size) {
          if ((char_data[x] >> y) & 0x01) {
            printf("Bit value at (%d, %d): %d\n", x, y, (char_data[x] >> y) & 0x01);
            coords_str[coords_index * 2] = char_offset_x + x;
            coords_str[coords_index * 2 + 1] = y;
            coords_index++;
  
// // Inside the get_pixel_coordinates function, add the following line
// mgos_usleep(1000); // Add a small delay to avoid overflowing the console output buffer

// printf("Generated coordinates: %d, %d\n", char_offset_x + x, y);
          }
        }
      }
    }
  }

  return coords_index;
}

void neopixel_print_string_wrapper(const char *text, int x_coord, int y_coord) {
  if (s_strip) {
    neopixel_print_string(s_strip, text, x_coord, y_coord);
  } else {
    LOG(LL_ERROR, ("NeoPixel strip not initialized"));
  }
}


// The matrix is configured with the following strip_index configuration:
// 0  
// 1   .
// .   .
// .   .
// .  17
// 15 16
// 
// The x_coord and y_coordinate indexes map to:
// strip_index 0 = x_coord: 0, y_coord: 15
// strip_index 16 = x_coord: 1, y_coord: 0
void neopixel_print_string(struct mgos_neopixel *strip, const char *text, int x_coord, int y_coord) {

  int text_length = strlen(text);
  for (int i = 0; i < text_length; i++) {
    char c = text[i];
    printf("Processing character: %c\n", c);
    const uint8_t *char_data = get_char_data(c);
    for (int char_y = 0; char_y < CHAR_HEIGHT; char_y++) {
      for (int char_x = 0; char_x < CHAR_WIDTH; char_x++) {
          // Process one column
        if ((char_data[char_y] >> (CHAR_WIDTH - 1 - char_x)) & 0x01) {

          // Calculate the absolute coordinates of the character data
          int matrix_x = x_coord + char_x;
          int matrix_y = y_coord + char_y;

          // Transform the absolute coordinates into an linear LED index for a
          // continuous zigzag panel configuration.
          int strip_index;
          if (matrix_x % 2 == 0) {
            // Even columns start at the top
            strip_index = matrix_y * PANEL_WIDTH + matrix_x;
          } else {
            // Odd columns start at the bottom
            strip_index = (matrix_y + 1) * PANEL_WIDTH - matrix_x - 1;
          }

          // Print the variables for debugging
          printf("led_x: %d, led_y: %d, led_index: %d\n", matrix_x, matrix_y, strip_index);

          if (strip_index >= 0 && strip_index < PANEL_WIDTH * PANEL_HEIGHT) {
            mgos_neopixel_set(strip, strip_index, 0, 50, 0);
          } else {
            printf("Invalid strip index: %d\n", strip_index);
          }
        }
      }
    }
  }
  // Show the updated pixels on the strip
  mgos_neopixel_show(strip);
}

    /*int char_offset_x = i * (CHAR_WIDTH + SPACE_BETWEEN_CHARS);
    const uint8_t *char_data = get_char_data(c);

    for (int x = 0; x < CHAR_WIDTH; x++) {
      for (int y = 0; y < CHAR_HEIGHT; y++) {
        if ((char_data[x] >> y) & 0x01) {
          int led_x = x_coord + char_offset_x + x;
          int led_y = y_coord + y;
          
          // Calculate the index for the zig-zag configuration
          int led_index = led_y * 16;
          if (led_y % 2 == 0) {
            led_index += led_x;
          } else {
            led_index += 15 - led_x;
          }

          // Set the pixel color (change the color to your preference)
          mgos_neopixel_set(strip, led_index, 255, 0, 0);
        }
      }
    }*/
  