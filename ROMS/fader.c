#include <avr/io.h>
#include <util/delay.h>


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

int main (void){
  DDRB = 0b00001111;
  PORTB = 0b00000000;
  //
  for(int i=0;i!=256;++i) {
    for(int j=0;j!=512;++j) {
      display_write(i);
    }
  }
  // Done
}
