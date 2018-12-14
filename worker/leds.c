#include "leds.h"

void initialize_leds() {
  INIT_YELLOW;
  INIT_GREEN;
  //INIT_RED;
  INIT_RED_E;
  INIT_GREEN_E;
}

// Syntax for using struct to access registers inspired by
// http://www.avrfreaks.net/forum/io-ports-mapped-struct
void flash_led(IO_struct * color, int inverted) {
  if (!inverted) {
    SET_BIT(*color->port, color->pin);
  } else {
    CLEAR_BIT(*color->port, color->pin);
  }
  _delay_ms(200);
  TOGGLE_BIT(*color->port, color->pin);
  _delay_ms(200);
}

void light_show() {
  // Flash them all to ensure they are working.
  int i;
  for (i = 0; i < 2; i++) {
		flash_led(&_yellow, 0);
	//	flash_led(&_red, 1);
		flash_led(&_green, 1);
  }
}

void busy_light(){
  SET_BIT(*(&_red_e)->port, _red_e.pin ); // set busy light
  CLEAR_BIT(*(&_green)->port, _green.pin ); // clear ready light
}
void ready_light() {
    SET_BIT(*(&_green)->port, _green.pin ); // set ready light
    CLEAR_BIT(*(&_red_e)->port, _red_e.pin ); // clear busy light
}
