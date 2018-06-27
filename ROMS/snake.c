#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include "tinyboy.h"

// =======================================
// Sprites
// =======================================

int sprites[2][8] = {
  {
    0,0,0,0,0,0,0,0 // all off
  },
  {
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff // all on
  },
};

// =========================================================
// Config
// =========================================================

typedef enum Direction { 
  NORTH, 
  SOUTH, 
  EAST,
  WEST 
} Direction;

typedef struct Point {
  int x;
  int y;
} Point;

typedef int bool;

#define FALSE 0
#define TRUE 1

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

// =========================================================
// Misc Stuff
// =========================================================

#define WHITE 0x00
#define BLACK 0x01

int min(int x, int y) {
  if(x < y) {
    return x;
  } else {
    return y;
  }
}

int max(int x, int y) {
  if(x > y) {
    return x;
  } else {
    return y;
  }
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
  while(p.x < 0) {
    p.x = p.x + 8;
  }
  while(p.x > 7) {
    p.x = p.x - 8;
  }
  // Wrap y position
  while(p.y < 0) {
    p.y = p.y + 8;
  }
  while(p.y > 7) {
    p.y = p.y - 8;
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
    return FALSE;
  }
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
      return TRUE;
    } else {
      p = getEndPoint(p,section);
    }
  }
  return FALSE;
}

void drawBlockSection(Point from, Point to) {
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
    display[from.x][from.y] = BLACK;
  }  
}

Point drawSnakeSection(Point from, Section section) {
   Point to = getEndPoint(from,section);   
   drawBlockSection(from,to);
   return to;
}

void drawSnake() {
  // Check for change of direction
  //
  Point pos = snake.head;
  //
  for (int i = 0; i < snake.numberOfSections; ++i) {
    pos = drawSnakeSection(pos,snake.sections[i]);
  }
}

// =========================================================
// Move Snake
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
void updateDirection() {
  Direction oDirection = getCurrentDirection();
  Direction nDirection;
  //
  int buttons = read_buttons();
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
  DDRB = 0b00001111;
  PORTB = 0b00000000;
}

void resetGame() {
  // Configure the snake
  snake.head.x = 4;
  snake.head.y = 4;
  snake.numberOfSections = 1;
  snake.sections[0].direction = EAST;
  snake.sections[0].length = 7;
}

// =========================================================
// Loop
// =========================================================

void gameOver() {
  display_fill(0);
  resetGame();
  _delay_ms(1000);
}

void main() {
  setup();
  resetGame();
  //
  while(1) {
    // Time for the next frame!
    // Check for change of direction
    updateDirection();
    // Move snake in current direction
    moveSnake();        
    // Draw snake
    display_fill(0);
    drawSnake();
    display_refresh(sprites);
    // Check for self collision
    if(isTouchingSelf()) {
      gameOver();
    }
    //
    _delay_ms(100);
  }
}
