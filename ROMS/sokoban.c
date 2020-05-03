#include <avr/io.h>
#include <util/delay.h>
#include "tinyboy.h"

// =========================================================
// Types
// =========================================================

// =========================================================
// STATE
// =========================================================

int player_x;
int player_y;
int rock_x;
int rock_y;

// =======================================
// Sprites
// =======================================

uint8_t sprites[5][4] = {
  {
    0,0,0,0 // all off
  },
  {
    0b1111,
    0b1001,
    0b1001,    
    0b1111
  },
  { 
    0b0111,
    0b1100,
    0b1100,    
    0b0111
  }, 
  { 
    0b1110,
    0b0011,
    0b0011,    
    0b1110
  }, 
  {
    0b1000,
    0b0010,
    0b0100,    
    0b0010
  } 
};

// =========================================================
// IO Functions
// =========================================================

void refresh() {
  // Empty display
  display_fill(0);
  display_draw(player_x,player_y,2);
  /* display_draw(rock_x,rock_y,1); */
  display_refresh(sprites);
}

int withinBounds (int x, int y) {
  return (x >= 0 && x < 16) && (y >= 0 && y < 16);
}

void setup() {
  // set SCLK, MOSI, MISO, SS to be output
  DDRB = 0b00001111;
  PORTB = 0b00000000;
  //
  player_x = 4;
  player_y = 4;
  rock_x = 1;
  rock_y = 1;
}

void clock(int buttons) {
  int dx = 0;
  int dy = 0;
  //
  switch(buttons) {
  case BUTTON_UP:
    dy = -1;
    break;
  case BUTTON_DOWN:
    dy = +1;
    break;
  case BUTTON_LEFT:
    dx = -1;
    break;
  case BUTTON_RIGHT:
    dx = +1;
    break;
  }
  //
  int nx = player_x + dx;
  int ny = player_y + dy;    
  // Attempt to make the move
  if(withinBounds(nx,ny)) {
    // Check about moving rock
    if(nx == rock_x && ny == rock_y) {
      int rx = rock_x + dx;
      int ry = rock_y + dy;    	
      // Attemp to push rock
      if(withinBounds(rx,ry)) {
	// All looks good.
	player_x = nx;
	player_y = ny;
	rock_x = rx;
	rock_y = ry;	
      }
    } else {
      player_x = nx;
      player_y = ny;
    }
    // Refresh Display
    refresh();    
  }
}

int main() {
  setup();
  // Setup stuff
  setup();
  // Run
  while(1) {
    int buttons = 0;
    // delay loop
    for(int i=0;i<1000;++i) {
      for(int j=0;j<100;++j) {      
	// record any buttons pressed between frames
	buttons |= read_buttons();
      }
    }
    // refresh
    clock(buttons);
  }

  return 1;
}
