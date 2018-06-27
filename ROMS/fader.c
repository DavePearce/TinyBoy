#include <avr/io.h>
#include <util/delay.h>
#include "tinyboy.h"

int main (void){
  DDRB = 0b00001111;
  PORTB = 0b00000000;
  //
  for(int i=16;i>0;i=i-3) {
    for(int j=0;j!=512;++j) {
      int byte = 0;
      for(int k=0;k!=8;++k) {
	int pixel = (j*8) + k;
	if((pixel % i) == 0) {
	  byte |= 1 << k;
	}
      }
      display_write(byte);
    }
  }
  // Done
}
