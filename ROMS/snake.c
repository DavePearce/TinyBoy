#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

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
// IO Functions
// =========================================================

int display[8][8];

#define BUTTON_UP 0b00000100
#define BUTTON_DOWN 0b00001000
#define BUTTON_LEFT 0b00010000
#define BUTTON_RIGHT 0b00100000

#define WHITE 0x00
#define BLACK 0xFF

int read_buttons() {
  return PORTB & 0b00111100;
}

void display_write(int c) {
  for(int i=0;i<8;++i) {
    PORTB = 0b00000000;
    if((c & 1) == 1) {
      PORTB = 0b00000011;
    } else {
      PORTB = 0b00000001;
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
	display_write(display[j][i]);	
      }
    }
  }
}

// =========================================================
// Misc Stuff
// =========================================================

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
  return p;
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
  if (from.x != to.x) {
    // horizontal setion
    int s = min(from.x, to.x);
    int e = max(from.x, to.x);
    for (int i = s; i <= e; i = i + 1) {
      display[from.y][i] = BLACK;            
    }
  } else {
    // vertical section
    int s = min(from.y, to.y);
    int e = max(from.y, to.y);
    for (int i = s; i <= e; i = i + 1) {  
      display[i][from.x] = BLACK;      
    }
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
  if ((buttons & BUTTON_LEFT) != 0) {
    nDirection = WEST;
  } else if ((buttons & BUTTON_RIGHT) != 0) {
    nDirection = EAST;
  } else if ((buttons & BUTTON_UP) != 0) {
    nDirection = NORTH;
  } else if ((buttons & BUTTON_DOWN) != 0) {
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
  snake.sections[0].length = 5;
}

// =========================================================
// Loop
// =========================================================

void gameOver() {
  clear();
  resetGame();
  _delay_ms(1000);
}

void main() {
    drawSnake();
    refresh();
  /*
  while(1) {
    // Time for the next frame!
    // Check for change of direction
    updateDirection();
    // Move snake in current direction
    moveSnake();        
    // Draw snake
    clear();
    drawSnake();
    refresh();
    // Check for self collision
    if(isTouchingSelf()) {
      gameOver();
    }
    //
    _delay_ms(1000);
  }
  */
}
