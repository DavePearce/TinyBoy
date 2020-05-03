#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include "tinyboy.h"

#define ARENA_MIN_X 1
#define ARENA_MAX_X (DISPLAY_WIDTH-2)
#define ARENA_MIN_Y 3
#define ARENA_MAX_Y (DISPLAY_HEIGHT-2)
#define ARENA_WIDTH (ARENA_MAX_X - ARENA_MIN_X + 1)
#define ARENA_HEIGHT (ARENA_MAX_Y - ARENA_MIN_Y + 1)

// =======================================
// Sprites
// =======================================

#define SPACE 0x00
#define HEAD_N 0x01
#define HEAD_S 0x02
#define HEAD_E 0x03
#define HEAD_W 0x04
#define BODY_NS 0x05
#define BODY_EW 0x06
#define JOINT_NE 0x07
#define JOINT_SE 0x08
#define JOINT_NW 0x09
#define JOINT_SW 0x0A
#define PILL 0x0B
#define EATEN_PILL 0x0C
#define BORDER_LR 0x0D
#define BORDER_TB 0x0E
#define BORDER_CNR 0x0F

uint8_t arena_sprites[][4] = {
  {
    0,0,0,0 // all off
  },
  { // Head North
    0b0110,
    0b0110,
    0b1010,    
    0b0110
  }, 
  { // Head South
    0b0110,    
    0b1010,
    0b0110,
    0b0110    
  },
  { // Head East
    0b0100,
    0b1011,
    0b1111,    
    0b0000
  }, 
  { // Head West
    0b0010,
    0b1101,
    0b1111,    
    0b0000
  },  
  { // Body North/South
    0b0110,    
    0b0010,
    0b0100,
    0b0110    
  },
  { // Body East/West
    0b0000,    
    0b1101,
    0b1011,
    0b0000    
  },
  { // North-->East
    0b0000,    
    0b0011,
    0b0101,
    0b0110    
  },  
  { // South-->East
    0b0110,    
    0b0101,
    0b0011,
    0b0000    
  },
  { // North-->West
    0b0000,    
    0b1100,
    0b1010,
    0b0110    
  },
  { // South-->West
    0b0110,    
    0b1010,
    0b1100,
    0b0000
  },
  { // Pill
    0b0010,    
    0b0101,
    0b0010,
    0b0000    
  },
  { // Eaten Pill
    0b0110,    
    0b1011,
    0b1101,
    0b0110    
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
  { // Corner Border
   0b1111,
   0b1111,
   0b1111,
   0b1111,
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

// =========================================================
// Config
// =========================================================

typedef enum Direction { 
  NORTH = 0, 
  SOUTH = 1, 
  EAST = 2,
  WEST = 3 
} Direction;

typedef struct Point {
  int x;
  int y;
} Point;

/**
 * Represents a single section within a snake.
 */
typedef struct Section {
  Direction direction; // NORTH, SOUTH, EAST or WEST
  unsigned int length;    // number of blocks in this section
} Section;

/**
 * A snake is made up of one or more sections relative to a given 
 * "head" position.  For example, consider this snake:
 *
 *  OXXXX
 *      X
 *      X
 *      
 * This is made up of two sections: {{EAST,5},{SOUTH,2}}
 */
typedef struct Snake {
  /**
   * The head of the snake.  That is the point in the arena
   * where the snake starts from.
   */
  Point head;
  /**
   * The sections making up the snake.  Every snake has at 
   * least one section.
   */
  Section sections[10];
  /**
   * The number of sections in this snake.  Must be at least 
   * one or more.
   */
  unsigned int numberOfSections;
} Snake;

Snake snake;
// location of pills
Point pills[20] = {{.x=10,.y=7}};
// Number of pills in play
uint8_t numberOfPills = 1;
// Seed for location of next
uint8_t seed = 23;
// Game score
int score = 0;

// =========================================================
// Misc Stuff
// =========================================================

/**
 * Return the minimum of two integers
 */
int min(int x, int y) {
  return (x < y) ? x : y;
}

/**
 * Return the maximum of two integers
 */
int max(int x, int y) {
  return (x > y) ? x : y;
}

/**
 * Access the first section of the snake
 */
Section *firstSection() {
  return &snake.sections[0];
}

/**
 * Access the last section of the snake
 */
Section *lastSection() {
  return &snake.sections[snake.numberOfSections-1];
}

/**
 * Increase the length of a given section
 */
void lengthenSection(Section *section) {
  section->length += 1;
}

/**
 * Trim the tail of the snake as it moves.  The snake is made up of one of or more
 */
void trimSection(Section *section) {
  section->length -= 1;
}

/**
 * Remove the last section of the snake (e.g. because it no longer has any length)
 */
void removeLastSection() {  
    snake.numberOfSections = snake.numberOfSections - 1;
}

Direction getCurrentDirection() {
  return snake.sections[0].direction;
}

/**
 * Make sure point properly wraps the display
 */
Point wrap(Point p) {
  // Wrap x position
  while(p.x < ARENA_MIN_X) {
    p.x = ARENA_MAX_X;
  }
  while(p.x > ARENA_MAX_X) {
    p.x = ARENA_MIN_X;
  }
  // Wrap y position
  while(p.y < ARENA_MIN_Y) {
    p.y = ARENA_MAX_Y;
  }
  while(p.y > ARENA_MAX_Y) {
    p.y = ARENA_MIN_Y;
  }
  return p;
}

/**
 * Move a given point in a given direction
 */
Point movePoint(Direction direction, Point p) {
  switch (direction) {
    case NORTH:
      p.y -= 1;
      break;
    case SOUTH:
      p.y += 1;
      break;
    case EAST:
      p.x += 1;
      break;
    case WEST:
      p.x -= 1;
      break;
  }
  //
  return wrap(p);
}

void drawArena() {
  display_fill(SPACE);  
  // Draw left-right border
  for(int i=(ARENA_MIN_X-1);i<=ARENA_MAX_X;++i) {
    display_draw(i,ARENA_MIN_Y-1,BORDER_LR);    
    display_draw(i,ARENA_MAX_Y+1,BORDER_LR);
  }
  // Draw top-bottom border
  for(int i=(ARENA_MIN_Y-1);i<=ARENA_MAX_Y;++i) {
    display_draw(ARENA_MIN_X-1,i,BORDER_TB);    
    display_draw(ARENA_MAX_X+1,i,BORDER_TB);
  }
  // Draw corners
  display_draw(ARENA_MIN_X-1,ARENA_MIN_Y-1,BORDER_CNR);
  display_draw(ARENA_MAX_X+1,ARENA_MIN_Y-1,BORDER_CNR);
  display_draw(ARENA_MIN_X-1,ARENA_MAX_Y+1,BORDER_CNR);
  display_draw(ARENA_MAX_X+1,ARENA_MAX_Y+1,BORDER_CNR);  
}

void drawScore(int x, int y, int score) {
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

// =========================================================
// Draw Snake
// =========================================================

Point getEndPoint(Point start, Section section) {
  switch(section.direction) {
    case NORTH:
      start.y += section.length;
      break;
    case SOUTH:
      start.y -= section.length;
      break;
    case EAST:
      start.x -= section.length;
      break;
    case WEST:    
      start.x += section.length;
      break;
   }
   return start;
}

bool isPointInSection(Point p, Point from, Section section) {
  Point to = getEndPoint(from,section);
  if(p.x == from.x && p.x == to.x) {
    int s = min(from.y, to.y);
    int e = max(from.y, to.y);
    return s <= p.y && p.y <= e;
  } else if(p.y == from.y && p.y == to.y) {
    int s = min(from.x, to.x);
    int e = max(from.x, to.x);
    return s <= p.x && p.x <= e;
  } else {
    return false;
  }
}

/**
 * Check whether or not a given point is inside the snake
 */
bool isPointInSnake(Point p) {
  for(int i=0;i!=snake.numberOfSections;i++) {
    Section section = snake.sections[i];
    if(isPointInSection(p,snake.head,section)) {
      return true;
    }
  }
  return false;
}

/**
 * Check whether or not the snake is touching itself.
 */
bool isTouchingSelf() {
  Point head = snake.head;
  Point p = head;
  for(int i=0;i!=snake.numberOfSections;i++) {
    Section section = snake.sections[i];
    // Ignore first section, since the head always
    // intersects this!
    if(i != 0 && isPointInSection(head,p,section)) {
      return true;
    } else {
      p = getEndPoint(p,section);
    }
  }
  return false;
}

void drawSnakeBody(Point from, Point to) {
  int count;
  Direction dir;
  //
  if (from.x != to.x) {
    // horizontal setion
    count = from.x - to.x;
    dir = count < 0 ? EAST : WEST;
  } else {
    // vertical section
    count = from.y - to.y;
    dir = count < 0 ? SOUTH : NORTH;
  }
  // Normalise count
  count = count < 0 ? -count : count;
  // Update the display
  for (int i = 0; i < count; i = i + 1) {
    from = movePoint(dir,from);    
    display_draw(from.x,from.y,BODY_NS + (dir/2));
  }  
}

Point drawSnakeHead(Point from, Section s1) {
  Point to = getEndPoint(from,s1);
  drawSnakeBody(from,to);  
  display_draw(from.x,from.y,HEAD_N + s1.direction);
  return to;
}

#define NORTHEAST (NORTH + (EAST << 2))
#define WESTSOUTH (WEST + (SOUTH << 2))
#define SOUTHEAST (SOUTH + (EAST << 2))
#define WESTNORTH (WEST + (NORTH << 2))
#define NORTHWEST (NORTH + (WEST << 2))
#define EASTSOUTH (EAST + (SOUTH << 2))
#define SOUTHWEST (SOUTH + (WEST << 2))
#define EASTNORTH (EAST + (NORTH << 2))

void drawSnakeJoint(Point pt, Direction from, Direction to) {
  uint8_t idx = to + (from << 2);
  switch(idx) {
  case NORTHEAST:
  case WESTSOUTH:
    idx = 0;
    break;
  case SOUTHEAST:
  case WESTNORTH:
    idx = 1;
    break;
  case NORTHWEST:
  case EASTSOUTH:
    idx = 2;
    break;
  default:
  case SOUTHWEST:
  case EASTNORTH:
    idx = 3;
    break;
  }
  // Done
  display_draw(pt.x,pt.y,JOINT_NE + idx);
}

Point drawSnakeSection(Point from, Section s0, Section s1) {
   Point to = getEndPoint(from,s1);
   drawSnakeBody(from,to);
   drawSnakeJoint(from,s0.direction,s1.direction);
   return to;
}

void drawSnake() {
  // Check for change of direction
  //
  Point pos = snake.head;
  int n = snake.numberOfSections;
  // Draw the snake head
  pos = drawSnakeHead(pos,snake.sections[0]);
  // Draw remaining sections
  for (int i = 1; i < n; ++i) {
    pos = drawSnakeSection(pos,snake.sections[i-1],snake.sections[i]);
  }  
}

void drawPills() {
  for(int i=0;i<numberOfPills;++i) {
    Point pt = pills[i];
    if(i == 0) {
      display_draw(pt.x,pt.y,PILL);
    } else if(isPointInSnake(pt)) {
      display_draw(pt.x,pt.y,EATEN_PILL);
    } else {
      numberOfPills = numberOfPills-1;
      pills[i] = pills[numberOfPills];
    }
  }
}

// =========================================================
// Snake Stuff
// =========================================================

/**
   Move head of snake in current direction; trim tail as needed
*/
void moveSnake() {
  // First, move the head
  snake.head = movePoint(getCurrentDirection(),snake.head);
  // Second, update the first and last sections
  Section *first = firstSection();
  Section *last = lastSection();
  lengthenSection(first);  
  trimSection(last);
  if(last->length == 0) {
    // The last section of the snake has now reduced to have zero 
    // length, in which case it can be dropped altogether.
    removeLastSection();
  }
}

int lengthOfSnake() {
  int r = 0;
  for(int i=0;i!=snake.numberOfSections;++i) {
    r += snake.sections[i].length;
  }
  return r;
}

// =========================================================
// Pill Stuff
// =========================================================

/**
 * Check whether head of snake is at pill
 */
bool isEatingPill() {
  return (pills[0].x == snake.head.x) && (pills[0].y == snake.head.y); 
}

/**
 * Insert a pill to front of queue.
 */
void insertPill(Point pt) {
  for(int i=1;i<=numberOfPills;++i) {
    pills[i] = pills[i-1];
  }
  pills[0] = pt;
  numberOfPills++;
}

/**
 * Attempt to place pill in an empty location
 */
void placeNextPill() {
  seed = seed + 57;
  int gaps = (ARENA_WIDTH * ARENA_HEIGHT) - lengthOfSnake();
  int gap = seed % gaps;
  for(int x=ARENA_MIN_X;x<=ARENA_MAX_X;++x) {
    for(int y=ARENA_MIN_Y;y<=ARENA_MAX_Y;++y) {
      Point p = {.x=x,.y=y};
      if(gap == 0) {
	insertPill(p);
	return;
      } else if(!isPointInSnake(p)) {
	gap = gap - 1;
      }
    }
  }
}


// =========================================================
// Input
// =========================================================

/**
 * Push a new section onto the snake.
 */
void newSection(Direction direction) {
  // First, move all existing points one
  // position down the array.
  for (int i = snake.numberOfSections; i > 0; i = i - 1) {
    snake.sections[i] = snake.sections[i - 1];
  }
  // Second, put in the new head
  snake.sections[0].direction = direction;
  snake.sections[0].length = 0;
  // Finally, update the size of the snake
  snake.numberOfSections = snake.numberOfSections + 1;
}

/**
 *  Check whether any buttons are pressed which signals a change of direction.
 */
void updateDirection(int buttons) {
  Direction oDirection = getCurrentDirection();
  Direction nDirection;
  //  
  if (buttons & BUTTON_LEFT) {
    nDirection = WEST;
  } else if (buttons & BUTTON_RIGHT) {
    nDirection = EAST;
  } else if (buttons & BUTTON_UP) {
    nDirection = NORTH;
  } else if (buttons & BUTTON_DOWN) {
    nDirection = SOUTH;
  } else {
    return;
  }
  //
  if (oDirection != nDirection) {
    // Yes, there was a change of direction.
    // Therefore, push a new section onto the snake
    newSection(nDirection);
  }
}

// =========================================================
// Setup
// =========================================================

void setup() {
  // set SCLK, MOSI, MISO, SS to be output
  DDRB = SCK | MOSI;  
  //DDRB = 0b00001111;
  PORTB = 0b00000000;
}

void resetGame() {
  // Configure the snake
  snake.head.x = 4;
  snake.head.y = 4;
  snake.numberOfSections = 1;
  snake.sections[0].direction = EAST;
  snake.sections[0].length = 3;
  // Reset the score
  score = 0;
  // Redraw the display
  drawArena();
  drawScore(DISPLAY_WIDTH-4,0,score);
  drawScore(DISPLAY_WIDTH-4,1,score);  
}

// =========================================================
// Loop
// =========================================================

void gameOver() {
  resetGame();
  _delay_ms(1000);
}

/**
 * Clock another cycle of the game.
 */
void clock(int buttons) {
  // Time for the next frame!
  // Check for change of direction
  updateDirection(buttons);
  // Move snake in current direction
  moveSnake();
  // Draw Arena
  drawArena();  
  // Draw Score Line
  drawScore(DISPLAY_WIDTH-4,0,score);
  drawScore(DISPLAY_WIDTH-4,1,score);
  // Draw Snake
  drawSnake();
  // Draw pills
  drawPills();
  display_refresh_partial(0,1,digit_sprites_top);
  display_refresh_partial(1,2,digit_sprites_bottom);
  display_refresh_partial(2,DISPLAY_HEIGHT,arena_sprites);
  // Check for self collision
  if(isTouchingSelf()) {
    gameOver();
  } else if(isEatingPill()) {
    score = score + 10;
    // Length last section of snake
    lengthenSection(lastSection());
    // Place a new pill
    placeNextPill();
  }
}

int main() {
  setup();
  resetGame();
  //
  while(1) {
    int buttons = 0;
    // delay loop
    for(int i=0;i<500;++i) {
      for(int j=0;j<100;++j) {      
	// record any buttons pressed between frames
	buttons |= read_buttons();
      }
    }
    // refresh
    clock(buttons);
  }
  // Dead code
  return 0;
}
