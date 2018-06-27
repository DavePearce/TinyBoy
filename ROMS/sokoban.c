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

// =========================================================
// IO Functions
// =========================================================

#define WHITE 0x00
#define GREY  0b10000001
#define BLACK 0xFF

void refresh() {
  for(int i=0;i<8;++i) {
    for(int k=0;k<8;++k) {
      for(int j=0;j<8;++j) {
	if(i == player_y && j == player_x) {
	  display_write(BLACK);
	} else if(i == rock_y && j == rock_x) {
	  if(k == 0 || k == 7) {
	    display_write(BLACK);
	  } else {
	    display_write(GREY);
	  }
	} else {
	  display_write(WHITE);
	}
      }
    }
  }
}

int withinBounds(int x, int y) {
  return (x >= 0 && x < 8) && (y >= 0 && y < 8);
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

int main() {
  // Setup stuff
  setup();
  // Run
  while(1) {
    int dx = 0;
    int dy = 0;
    // Read user input
    int buttons = read_buttons();    
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
    // Delay
    _delay_ms(50);
  }
}
