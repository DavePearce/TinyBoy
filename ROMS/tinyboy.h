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
uint8_t _display[16][8];

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
    int yd = y >> 1;
    int ym = y & 0b01;
    uint8_t val = _display[x][yd];
    if(ym == 1) {
      val &= 0b00001111;
      val |= (sprite << 4);
    } else {
      val &= 0b11110000;
      val |= sprite;
    }
    _display[x][yd] = val;
  }
}

uint8_t display_read(int x, int y) {
  // Santiy check location
  if(x >= 0 && x < 16 && y >= 0 && y < 16) {
    int yd = y >> 1;
    int ym = y & 0b01;
    uint8_t val = _display[x][yd];
    if(ym == 1) {
      return (val >> 4) & 0b00001111;
    } else {
      return val & 0b00001111;
    }
  } else {
    return -1;
  }
}

/**
 * Fill display with a given sprite.
 */
void display_fill(uint8_t c) {
  // Duplicate character  
  c = (c << 4) | c;
  // Bitblast it out
  for(int i=0;i<16;++i) {
    for(int j=0;j<8;++j) {
      _display[i][j] = c;
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
	uint8_t c1 = display_read(x,y);
	uint8_t c2 = display_read(x+1,y);
	uint8_t *sl = sprites[c1];
	uint8_t *sr = sprites[c2];
	int data = sr[sy] | (sl[sy] << 4);
	display_write(data);
      }
    }
  }
}
