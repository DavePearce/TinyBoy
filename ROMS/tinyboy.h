// Simple header file containing most useful stuff.

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
int display[8][8];

/**
 * Write 8 bits to display.
 */
void display_write(int c) {
  for(int i=0;i<8;++i) {
    PORTB = 0b00000000;
    if((c & 1) == 1) {
      PORTB = SCK | MOSI;
    } else {
      PORTB = SCK;
    }
    c = c >> 1;
  }
}

/**
 * Draw a given spite to the display at a given x and y location.
 */
void display_draw(int x, int y, int sprite) {
  // Santiy check location
  if(x >= 0 && x < 8 && y >= 0 && y < 8) {
    display[x][y] = sprite;
  }
}

int display_read(int x, int y) {
  // Santiy check location
  if(x >= 0 && x < 8 && y >= 0 && y < 8) {
    return display[x][y];
  } else {
    return -1;
  }
}

/**
 * Fill display with a given sprite.
 */
void display_fill(int c) {
  for(int i=0;i!=8;++i) {
    for(int j=0;j!=8;++j) {
      display[i][j] = c;
    }
  }
}

/**
 * Write out display contents to device, using a given set of sprites.
 */
void display_refresh(int sprites[][8]) {
  for(int y=0;y<8;++y) {
    for(int sy=0;sy<8;++sy) {
      for(int x=0;x<8;++x) {
	int *sprite = sprites[display[x][y]];
	display_write(sprite[sy]);
      }
    }
  }
}
