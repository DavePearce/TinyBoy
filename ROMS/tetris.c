#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "tinyboy.h"

#define PLAYING  0
#define LANDED 1
#define RESTART 2
#define COLLIDED 3

#define EMPTY 0x00
#define FULL 0x01
#define BOX_1 0x02
#define BOX_2 0x03
#define BOX_3 0x04
#define BORDER_LR 0x05
#define BORDER_TB 0x06
#define BORDER_BL 0x07
#define BORDER_BR 0x08

#define ARENA_MIN_X 1
#define ARENA_MAX_X (DISPLAY_WIDTH-10)
#define ARENA_MIN_Y 0
#define ARENA_MAX_Y (DISPLAY_HEIGHT-2)
#define ARENA_WIDTH (ARENA_MAX_X - ARENA_MIN_X + 1)
#define ARENA_HEIGHT (ARENA_MAX_Y - ARENA_MIN_Y + 1)

// =======================================
// Sprites
// =======================================

uint8_t arena_sprites[][4] = {
  {
    0,0,0,0 // all off
  },
  {
    0xf,0xf,0xf,0xf // all on
  },
  {
    0b1111,
    0b1001,
    0b1001,
    0b1111,
  },
  {
    0b1111,
    0b1000,
    0b1001,
    0b1010,
  },  
  {
    0b1010,
    0b0101,
    0b1010,
    0b0101
  },
  { // Horizontal Border
   0b0000,
   0b1111,
   0b1111,
   0b0000,
  },
  { // Vertical Border
   0b0110,
   0b0110,
   0b0110,
   0b0110,
  },
  { // Bottom Left, Corner
   0b0110,
   0b0111,
   0b0111,
   0b0000,
  },
  { // Bottom Right, Corner
   0b0110,
   0b1110,
   0b1110,
   0b0000,
  }
};

uint8_t digit_sprites_top[][4] = {
 {
  0,0,0,0 // all off
 },				  
 { // zero
  0b0000,
  0b0100,
  0b1010,
  0b1010,
 },
 { // one
  0b0000,
  0b0100,
  0b1100,
  0b0100,
 },
 { // two
  0b0000,
  0b1100,
  0b0010,
  0b0100,
 },
 { // three
  0b0000,
  0b1100,
  0b0010,
  0b0110,
 },
 { // four
  0b0000,
  0b1000,
  0b1000,
  0b1010,
 },
 { // five
  0b0000,
  0b1110,
  0b1000,
  0b1110,
 },
 { // six
  0b0000,
  0b1110,
  0b1000,
  0b1110,
 },
 { // seven
  0b0000,
  0b1110,
  0b0010,
  0b0100,
 },
 { // eight
  0b0000,
  0b1110,
  0b1010,
  0b1110,
 },
 { // nine
  0b0000,
  0b1110,
  0b1010,
  0b1110,
 }
};

uint8_t digit_sprites_bottom[][4] = {
 {
  0,0,0,0 // all off
 },
 { // zero
  0b1010,
  0b0100,
  0b0000,
  0b0000,   
 },
 { // one
  0b0100,  
  0b1110,
  0b0000,
  0b0000,
 },
 { // two
  0b1000,  
  0b1110,
  0b0000,
  0b0000,
 },
 { // three
   0b0010,  
   0b1100,
   0b0000,
   0b0000,   
 },
 { // four
   0b1110,  
   0b0010,
   0b0000,
   0b0000,
 },
 { // five
   0b0010,  
   0b1110,
   0b0000,
   0b0000,   
 },
 { // six
  0b1010,  
  0b1110,
  0b0000,
  0b0000,
 },
 { // seven
  0b0100,  
  0b0100,
  0b0000,
  0b0000,
 },
 { // eight
  0b1010,  
  0b1110,
  0b0000,
  0b0000,
 },
 { // nine
  0b0010,  
  0b1110,
  0b0000,
  0b0000,
 }
};

uint8_t letter_sprites_top[][4] = {
 {
  0,0,0,0 // all off
 },				  
 { // C
  0b0000,
  0b1110,
  0b1000,
  0b1000,
 },				  
 { // E
  0b0000,
  0b1110,
  0b1000,
  0b1110,
 },				  
 { // I
  0b0000,
  0b1110,
  0b0100,
  0b0100,
 },				  
 { // L
  0b0000,
  0b1000,
  0b1000,
  0b1000,
 },				  
 { // N
  0b0000,
  0b1110,
  0b1010,
  0b1010,
 },				  
 { // O
  0b0000,
  0b1110,
  0b1010,
  0b1010,
 },				  
 { // R
  0b0000,
  0b1110,
  0b1000,
  0b1000,
 },				  
 { // S
  0b0000,
  0b1110,
  0b1000,
  0b1110,
 },
 { // :
  0b0000,
  0b0000,
  0b1000,
  0b0000,
 }
};

uint8_t letter_sprites_bottom[][4] = {
 {
  0,0,0,0 // all off
 },				  
 { // C
  0b1000,
  0b1110,
  0b0000,
  0b0000,  
 },				  
 { // E
  0b1000,
  0b1110,
  0b0000,
  0b0000,  
 },				  
 { // I
  0b0100,
  0b1110,
  0b0000,
  0b0000,  
 },				  
 { // L
  0b1000,
  0b1110,
  0b0000,
  0b0000,  
 },				  
 { // N
  0b1010,
  0b1010,
  0b0000,
  0b0000,  
 },				  
 { // O
  0b1010,
  0b1110,
  0b0000,
  0b0000,  
 },				  
 { // R
  0b1000,
  0b1000,
  0b0000,
  0b0000,  
 },				  
 { // S
  0b0010,
  0b1110,
  0b0000,
  0b0000,  
 },
 { // :
  0b1000,
  0b0000,
  0b0000,
  0b0000,  
 }

};

// =======================================
// Pieces
// =======================================

const int I_PIECE = 0;
const int J_PIECE = 1;
const int L_PIECE = 2;
const int O_PIECE = 3;
const int S_PIECE = 4;
const int Z_PIECE = 5;

uint8_t piece_array[6][4] = {
  /* I Piece */
  {
   0b0000,  
   0b1111,
   0b0000,
   0b0000   
  },
  /* J Piece */
  {
   0b0000,
   0b1110,
   0b0010,
   0b0000   
  },
  /* L Piece */
  {
   0b0000,
   0b0010,
   0b1110,
   0b0000
  },
  /* O Piece */
  {
   0b0000,
   0b0110, 
   0b0110,
   0b0000
  },
  /* S Piece */
  {
   0b0000,
   0b0110,
   0b1100,
   0b0000
  },
  /* Z Piece */
  {
   0b0000,
   0b1100,
   0b0110,
   0b0000
  }
};

// =======================================
// Words
// =======================================

/**
 * The sprite numbers representing the word "LINES:".
 */
uint8_t LINES_WORD[] = {4,3,5,2,8,9,0};

/**
 * The sprite numbers representing the word "SCORE:"
 */
uint8_t SCORE_WORD[] = {8,1,6,7,2,9,0};

// =======================================
// Global State
// =======================================

int sprite = 1;
int piece_num = 0;
uint8_t piece[4];
uint8_t next_piece[4];
int x = 3;
int y = -2;
int lines = 0;
int score = 0;

// =======================================
// Game Model
// =======================================

/**
 * Initialise a given piece buffer with the stencil for a given piece.
 * This essentially means copying the data from the stencil over into
 * the buffer.  The stencil in that buffer can then be rotated without
 * affecting the stencils for each piece (which should be constant).
 */
void initialise_piece(uint8_t data[4], int piece) {
  for(int i=0;i!=4;++i) {
     data[i] = piece_array[piece][i]; 
  }
}

/**
 * Rotate data array in a *counter*-clockwise fashion.  This uses a
 * temporary array to help with the rotation, with the end result
 * being the original data rotated in place.
 */
void rotate(uint8_t data[4]) {
  uint8_t tmp[4];
  for(int i=0;i<4;++i) {
     tmp[i] = data[i];
     data[i] = 0;
  }
  for(int i=0;i<4;++i) {
    for(int j=0;j<4;++j) {
      if(tmp[j] & (8>>i)) {
	data[i] |= (1<<j);
      }
    }   
  }
}

// Check whether or not the current piece has landed
// and or whether or not the arena is "full"
int next_state(uint8_t data[], int x, int y) {
   // Center the piece data
   int sx = x - 1;
   // Now read the display
   int is_full = 0;
   for(int i=0;i!=4;++i) {
     x = sx;
     for(int j=0;j!=4;++j) {
         // need to clip
       if(data[i] & (1<<j)) {   
	 if(y == 1) { is_full = 1; }              
	 if(y > ARENA_MAX_Y) { 
	   // In this case, have reached ground
	   return LANDED; 
	 } else if(x < ARENA_MIN_X || x > ARENA_MAX_X) { 
	   // In this case, have collided with wall
	   return COLLIDED; 
	 } else if(y >= ARENA_MIN_Y && y <= ARENA_MAX_Y && display_read(x,y) != EMPTY) {   
	   // In this case, have touched existing piece
	   return LANDED + is_full;               
	 }
       }
       x = x + 1;
     }
     y = y + 1;
   }
   return PLAYING;
}

/**
 * Check whether a given line in the arena is full or not.  That is,
 * whether or not there are any "gaps".
 */
bool line_full(int y) {
  for(int x=ARENA_MIN_X;x<=ARENA_MAX_X;++x) {
    if(display_read(x,y) == EMPTY) {
      return false;
    }
  }
  //
  return true;
}

void shift_down(int y) {
  for(int x = ARENA_MIN_X; x<=ARENA_MAX_X;++x) {
    // Shift down element in the given column
    for(int i = (y-1); i >= ARENA_MIN_Y; --i) {
      uint8_t s = display_read(x,i);
      display_draw(x,i+1,s);
    }
    // First row already empty after shift.
    display_draw(x,ARENA_MIN_Y,EMPTY);
  }
}

/**
 * Update game score based on lines removed using scoring table from
 * Gameboy tetris.
 */
void add_score(int lines) {
  switch(lines) {
  case 1:
    score += 10;
    break;
  case 2:
    score += 100;
    break;
  case 3:
    score += 300;
    break;
  case 4:
    score += 1200;
    break;    
  }
}

/**
 * Check and remove any full lines in the arena.
 */
void check_lines() {
  // Records number of lines removed
  int count = 0;
  // Go from bottom up to facilitate shift
  for(int y=ARENA_MAX_Y;y>=ARENA_MIN_Y;--y) {
    if(line_full(y)) {
      // Shift everything above down one
      shift_down(y);
      // Continue on this line for now
      y = y + 1;
      // Increment count
      count = count + 1;
    } 
  }
  // Account for score
  add_score(count);
  // Account for lines removed
  lines += count;
}

// =======================================
// Game View
// =======================================

/**
 * Draw a given piece centered at a given position on the display.
 */
void draw_at(int x, int y, uint8_t data[], uint8_t color) {
   // Center the piece data
   int sx = x - 1;
   int sy = y - 1;
   // Now blast it to the display
   y = sy;
   for(int i=0;i!=4;++i) {
     x = sx;
     if(y >= ARENA_MIN_Y) {     
       for(int j=0;j!=4;++j) {
         // need to clip
	 if(data[i] & (1 << j)) {
	   display_draw(x,y,color);
	 } 
         x = x + 1;
       }
     }
     y = y + 1;
   }    
}

void draw_arena() {
  display_fill(EMPTY);
  // Draw left-right border
  for(int i=ARENA_MIN_X;i<=ARENA_MAX_X;++i) {
    display_draw(i,ARENA_MAX_Y+1,BORDER_LR);
  }
  // Draw top-bottom border
  for(int i=ARENA_MIN_Y;i<=ARENA_MAX_Y;++i) {
    display_draw(ARENA_MIN_X-1,i,BORDER_TB);    
    display_draw(ARENA_MAX_X+1,i,BORDER_TB);
  }
  // Draw corners
  display_draw(ARENA_MIN_X-1,ARENA_MAX_Y+1,BORDER_BL);
  display_draw(ARENA_MAX_X+1,ARENA_MAX_Y+1,BORDER_BR);
}

/**
 * Fill the arena with a given sprite.
 */
void fill_arena(int sprite) {
  for(int x=ARENA_MIN_X;x<=ARENA_MAX_X;++x) {
    for(int y=ARENA_MIN_Y;y<=ARENA_MAX_Y;++y) {
      display_draw(x,y,sprite);
    }
  }
}

/**
 * Draw an arbtirary length sequence of characters
 */
void draw_word(int x, int y, uint8_t digits[]) {
  int i = 0;
  uint8_t c;
  while((c = digits[i]) != 0) {
    display_draw(x,y,c);
    x = x + 1;
    i = i + 1;
  }
}

void draw_num(int x, int y, int score) {
  int base = 1000;
  //
  for(int i=0;i!=4;++i) {
    int d = score / base;
    display_draw(x,y,d+1);    
    score = score - (d * base);
    base = base / 10;
    x = x + 1;
  }
}

/**
 * Refresh the display.  Unfortunately, this is more complicated than
 * might be expected because we need to handle the score carefully.
 * In particular, we have to use a different sprite set for drawing
 * the score unfortunately.
 */
void refresh() {
  display_refresh_split_line(12,0,arena_sprites,letter_sprites_top);
  display_refresh_split_line(12,1,arena_sprites,letter_sprites_bottom);    
  display_refresh_split_line(12,2,arena_sprites,digit_sprites_top);
  display_refresh_split_line(12,3,arena_sprites,digit_sprites_bottom);
  display_refresh_split_line(12,4,arena_sprites,letter_sprites_top);
  display_refresh_split_line(12,5,arena_sprites,letter_sprites_bottom);    
  display_refresh_split_line(12,6,arena_sprites,digit_sprites_top);
  display_refresh_split_line(12,7,arena_sprites,digit_sprites_bottom);  
  display_refresh_partial(8,DISPLAY_HEIGHT,arena_sprites);
}

// =======================================
// Game Loop
// =======================================

void setup() {
  // set SCLK, MOSI, MISO, SS to be output
  DDRB = SCK | MOSI;
  PORTB = 0b00000000;
}

/**
 * Move the piece on the board
 */
int move_piece(int buttons) {
  // Now, apply user actions
  if(buttons & BUTTON_UP) {
    rotate(piece);
  }
  if(buttons & BUTTON_DOWN) {
    while(next_state(piece,x,y) != LANDED) {
      y = y + 1;
    }
  }
  if((buttons & BUTTON_LEFT)) {
    if(next_state(piece,x-1,y) == PLAYING) {
      x = x - 1;
    }
  }
  if(buttons & BUTTON_RIGHT) {
    if(next_state(piece,x+1,y) == PLAYING) {
      x = x + 1;
    }
  }
  // Update the game state
  int state = next_state(piece,x,y);
  // Now, apply gravity (if possible)
  if(state == PLAYING) {
    // Gravity applies!
    y = y + 1;
  }  
  //
  return state;
}

void land_piece() {
  // Check whether any lines taken
  check_lines();
  //
  initialise_piece(piece,piece_num);
  piece_num = (piece_num + 1) % 6;
  // Clear old next piece
  draw_at(DISPLAY_WIDTH-5,9,next_piece,EMPTY);    
  initialise_piece(next_piece,piece_num);
  sprite = (sprite + 1) % 3;
  // Draw new next piece
  draw_at(DISPLAY_WIDTH-5,9,next_piece,1 + ((sprite+1) % 3));
  // Draw score
  draw_word(ARENA_MAX_X+2,0,SCORE_WORD);
  draw_word(ARENA_MAX_X+2,1,SCORE_WORD);    
  draw_num(DISPLAY_WIDTH-4,2,score);
  draw_num(DISPLAY_WIDTH-4,3,score);
  // Draw lines
  draw_word(ARENA_MAX_X+2,4,LINES_WORD);
  draw_word(ARENA_MAX_X+2,5,LINES_WORD);    
  draw_num(DISPLAY_WIDTH-4,6,lines);
  draw_num(DISPLAY_WIDTH-4,7,lines);    
  // Configure position of next piece
  x = (ARENA_MIN_X + ARENA_MAX_X) >> 1;
  y = ARENA_MIN_Y - 2;
}

void restart_game() {
  draw_arena();    
  fill_arena(BOX_3);
  refresh();
  _delay_ms(50);
  fill_arena(EMPTY);    
  lines = 0;
  score = 0;
  //
  land_piece();
}

void clock(int buttons){
  // Take piece off board
  draw_at(x,y,piece,EMPTY);      
  // Move piece
  int state = move_piece(buttons);
  // Draw piece in new position
  draw_at(x,y,piece,sprite+1);
  // Refresh display
  refresh();
  // See what happened
  switch(state) {
  case RESTART:
    restart_game();
    break;
  case LANDED:
    land_piece();
    break;
  }      
}

int main() {
  setup();
  restart_game();
  //
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
