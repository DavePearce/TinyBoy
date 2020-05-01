// Simple header file containing most useful stuff.
#include <stdint.h> 

#define SCK  0b00000100
#define MOSI 0b00000001

#define BUTTON_UP    0b00000010
#define BUTTON_DOWN  0b00001000
#define BUTTON_LEFT  0b00010000
#define BUTTON_RIGHT 0b00100000
#define BUTTON_MASK  0b00111010

/**
 * Read button states from tinyboy
 */
int read_buttons() {
  return PINB & BUTTON_MASK;
}

/**
 * Display buffer holds current state of display waiting to be written
 * out.
 */
uint8_t display[16][16];

/**
 * Write 8 bits to display.
 */
void display_write(uint8_t c) {
  for(int i=0;i<8;++i) {
    PORTB = 0b00000000;
    if((c & 0x80) == 0x80) {
      PORTB = SCK | MOSI;
    } else {
      PORTB = SCK;
    }
    c = c << 1;
  }
}

/**
 * Draw a given spite to the display at a given x and y location.
 */
void display_draw(int x, int y, uint8_t sprite) {
  // Santiy check location
  if(x >= 0 && x < 16 && y >= 0 && y < 16) {
    display[x][y] = sprite;
  }
}

uint8_t display_read(int x, int y) {
  // Santiy check location
  if(x >= 0 && x < 16 && y >= 0 && y < 16) {
    return display[x][y];
  } else {
    return -1;
  }
}

/**
 * Fill display with a given sprite.
 */
void display_fill(uint8_t c) {
  for(int i=0;i<16;++i) {
    for(int j=0;j<16;++j) {
      display[i][j] = c;
    }
  }
}

/**
 * Write out display contents to device, using a given set of sprites.
 */
void display_refresh(uint8_t sprites[][4]) {
  for(int y=0;y<16;++y) {
    for(int sy=0;sy<4;++sy) {
      for(int x=0;x<16;x+=2) {
	uint8_t *sl = sprites[display[x][y]];
	uint8_t *sr = sprites[display[x+1][y]];
	int data = sr[sy] | (sl[sy] << 4);
	display_write(data);
      }
    }
  }
}
