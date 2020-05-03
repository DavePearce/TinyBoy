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

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 48
#define DISPLAY_WIDTH (SCREEN_WIDTH >> 2)
#define DISPLAY_HEIGHT (SCREEN_HEIGHT >> 2)

/**
 * Display buffer holds current state of display waiting to be written
 * out.
 */
uint8_t _display[DISPLAY_WIDTH][DISPLAY_HEIGHT >> 1];

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

uint8_t display_read(int x, int y) {
  // Santiy check location
  int yd = y >> 1;
  int ym = y & 0b01;
  uint8_t val = _display[x][yd];
  if(ym == 1) {
    return (val >> 4) & 0b00001111;
  } else {
    return val & 0b00001111;
  }
}

/**
 * Fill display with a given sprite.
 */
void display_fill(uint8_t c) {
  // Duplicate character  
  c = (c << 4) | c;
  // Bitblast it out
  for(int i=0;i<DISPLAY_WIDTH;++i) {
    for(int j=0;j<(DISPLAY_HEIGHT>>1);++j) {
      _display[i][j] = c;
    }
  }
}

/**
 * Write out a single line to a given using a split point to switch sprites on.
 */
void display_refresh_split_line(int sx, int y, uint8_t sprites_a[][4], uint8_t sprites_b[][4]) {
  for(int sy=0;sy<4;++sy) {
    for(int x=0;x<sx;x+=2) {
      uint8_t c1 = display_read(x,y);
      uint8_t c2 = display_read(x+1,y);
      uint8_t *sl = sprites_a[c1];
      uint8_t *sr = sprites_a[c2];
      int data = sr[sy] | (sl[sy] << 4);
      display_write(data);
    }
    for(int x=sx;x<DISPLAY_WIDTH;x+=2) {
      uint8_t c1 = display_read(x,y);
      uint8_t c2 = display_read(x+1,y);
      uint8_t *sl = sprites_b[c1];
      uint8_t *sr = sprites_b[c2];
      int data = sr[sy] | (sl[sy] << 4);
      display_write(data);
    }    
  }
}

/**
 * Write out display contents to device, using a given set of sprites
 * and starting from a given Y position.  This allows one to use
 * different sprite sets for different lines.
 */
void display_refresh_partial(int y_start, int y_end, uint8_t sprites[][4]) {
  for(int y=y_start;y<y_end;++y) {
    for(int sy=0;sy<4;++sy) {
      for(int x=0;x<DISPLAY_WIDTH;x+=2) {
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

/**
 * Write out display contents to device, using a given set of sprites.
 */
void display_refresh(uint8_t sprites[][4]) {
  display_refresh_partial(0,DISPLAY_HEIGHT,sprites);
}

