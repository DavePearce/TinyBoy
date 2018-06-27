#include <avr/io.h>
#include <util/delay.h>

#define SCK 0b00000100
#define MOSI 0b00000001

#define BUTTON_UP    0b00000010
#define BUTTON_DOWN  0b00001000
#define BUTTON_LEFT  0b00010000
#define BUTTON_RIGHT 0b00100000
#define BUTTON_MASK  0b00111010

#define PLAYING  0
#define LANDED 1
#define RESTART 2
#define COLLIDED 3

#define WHITE 0x00
#define INTERNAL_a 0b10101001
#define INTERNAL_b 0b10010101
#define BLACK 0xFF

int display[8][8];

const int I_PIECE = 0;
const int J_PIECE = 1;
const int L_PIECE = 2;
const int O_PIECE = 3;
const int S_PIECE = 4;
const int Z_PIECE = 5;

int piece_array[6][16] = {
  /* I Piece */
  {
   0,0,0,0,  
   1,1,1,1,
   0,0,0,0,
   0,0,0,0   
  },
  /* J Piece */
  {
   0,0,0,0,  
   1,1,1,0,
   0,0,1,0,
   0,0,0,0   
  },
  /* L Piece */
  {
   0,0,0,0,
   0,0,1,0,
   1,1,1,0,
   0,0,0,0
  },
  /* O Piece */
  {
   0,0,0,0,
   0,1,1,0, 
   0,1,1,0,
   0,0,0,0
  },
  /* S Piece */
  {
   0,0,0,0,
   0,1,1,0,
   1,1,0,0,
   0,0,0,0
  },
  /* Z Piece */
  {
   0,0,0,0,
   1,1,0,0,
   0,1,1,0,
   0,0,0,0
  }
};

int read_buttons() {
  return PORTB & BUTTON_MASK;
}

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

void clear() {
  for(int i=0;i!=8;++i) {
    for(int j=0;j!=8;++j) {
      display[i][j] = 0;
    }  
  }
}

void refresh() {
  for(int i=0;i<8;++i) {
    for(int k=0;k<8;++k) {
      for(int j=0;j<8;++j) {
	if(display[j][i]) {
	  if(k == 0 || k == 7) {
	    display_write(BLACK);
	  } else if((k%2) == 0) {
	    display_write(INTERNAL_a);
	  } else {
	    display_write(INTERNAL_b);
	  }
	} else {
	  display_write(WHITE);
	}
      }
    }
  }
}

/* === Tetris Functions === */

void initialise_piece(int data[], int piece) {
  for(int i=0;i!=16;++i) {
     data[i] = piece_array[piece][i]; 
  }
}

void rotate(int data[]) {
  int tmp[16];
  for(int i=0;i<16;++i) {
     tmp[i] = data[i]; 
  }
  for(int i=0;i<4;++i) {
    for(int j=0;j<4;++j) {
      data[(i*4)+j] = tmp[(j*4)+i];
    }   
  }
}

// Check whether or not the current piece has landed
// and or whether or not the arena is "full"
int next_state(int data[], int x, int y) {
   // Center the piece data
   int sx = x - 1;
   // Now read the display
   int is_full = 0;
   for(int i=0;i!=4;++i) {
     x = sx;
     for(int j=0;j!=4;++j) {
         // need to clip
         if(data[(i*4)+j] == 1) {   
            if(y == 1) { is_full = 1; }              
            if(y == 8) { 
              // In this case, have reached ground
               return LANDED; 
            } else if(x < 0 || x >= 8) { 
              // In this case, have collided with wall
               return COLLIDED; 
            } else if(y >= 0 && y < 8 && display[x][y] != WHITE) {   
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

void draw_at(int x, int y, int data[], int color) {
   // Center the piece data
   int sx = x - 1;
   int sy = y - 1;
   // Now blast it to the display
   y = sy;
   for(int i=0;i!=4;++i) {
     x = sx;
     for(int j=0;j!=4;++j) {
         // need to clip
         if(x >= 0 && x < 8 && y >= 0 && y < 8) {       
           if(data[(i*4)+j] == 1) {
             display[x][y] = color;
           } 
         }
         x = x + 1;
     }
     y = y + 1;
   }    
}

/* === Game Loop === */

int state = RESTART; 
int piece_num = 0;
int piece[16];
int x = 3;
int y = -2;

void setup() {
  // set SCLK, MOSI, MISO, SS to be output
  DDRB = SCK | MOSI;
  PORTB = 0b00000000;
}

int main (void){
  // Configure
  setup();
  clear();
  //
  while(1) {
    switch(state) {
    case RESTART: 
      clear();
      state = LANDED;
      break;
    case LANDED:
      //check_lines();
      initialise_piece(piece,piece_num);
      piece_num = piece_num + 1;
      if(piece_num >= 6) {
	piece_num = 0;
      }
      state = PLAYING;
      x = 3;
      y = -2;
      break;
    case PLAYING:
      // First, take piece off board
      draw_at(x,y,piece,WHITE);
      // Now, apply user actions
      int buttons = read_buttons();
      if(buttons & BUTTON_UP) {
	rotate(piece);
      } else if(buttons & BUTTON_DOWN) {
	while(next_state(piece,x,y) != LANDED) {
	  y = y + 1;
	}
      } else if((buttons & BUTTON_LEFT)) {
	if(next_state(piece,x-1,y) == PLAYING) {
	  x = x - 1;
	}
      } else if(buttons & BUTTON_RIGHT) {
	if(next_state(piece,x+1,y) == PLAYING) {
	  x = x + 1;
	}
      }
      // Update the game state
      state = next_state(piece,x,y);
      // Now, apply gravity (if possible)
      if(state == PLAYING) {
	// Gravity applies!
	y = y + 1;
      }
      // Third put piece on board in 
      // new position
      draw_at(x,y,piece,BLACK);
      // Refresh display
      refresh();
      _delay_ms(50);      
      break;
    }      
  }
  //
  return 0;
}
