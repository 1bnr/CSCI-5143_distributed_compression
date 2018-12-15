#ifndef BUTTONS_H_
#define BUTTONS_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "common.h"

#define BUTTONA 3
#define BUTTONC 0

// Setup is for A and C only because button B does not have corresponding
// PCINT for easy interrupt programming.
//
// WARNING: ButtonC and RED LED share the pin. Do not use both.

#define INIT_BUTTONA _button_A = (IO_struct) { &DDRB, &PORTB, BUTTONA, &PINB };
#define INIT_BUTTONC _button_C = (IO_struct) { &DDRB, &PORTB, BUTTONC, &PINB };

IO_struct _button_A;
IO_struct _button_C;

#define BUTTON_MASK ((1<<BUTTONA) | (1<<BUTTONC))
int checkButtons();
void initialize_buttons();

/* Set up any of the buttons on the board.
 * This will set up for a PCINT ISR. Don't forget to call sei() in main.
 * parameter [out] : -1 for error in setting up. 1 otherwise.
 * parameter [in] IO_struct button : for A or C as defined above
*/
int SetUpButton(IO_struct * button);

/* Set up callback function for any of the buttons on the board.
 * parameter [in] IO_struct button : for A or C as defined above
 * parameter [in] release : 1 (true) = call function on release
 * parameter [in] callback : function to be called when event happens
 * parameter [out] : -1 for error in setting up. 1 otherwise.
 */
int SetUpButtonAction(IO_struct * button, int release, void(*callback)(void));

void EmptyFunction();

/* These function pointers can be used to generalize the PCINT code.
 * Rather than writing a new PCINT for every program to achieve different
 * behavior, you can always use the same PCINT code for buttons.
 * For example, I might write 2 functions that 1) blinks the red led every
 * time button A is released and 2) blinks the green led every time it is
 * pressed.
 * If I called these functions "void blink_red()" and "void blink_green()"
 * I would set fn_release_A = blink_red and fn_press_A = blink_green.
 * Note that the function signature (i.e. void fn(void)) must match.
 * If you don't want the buttons to have any functionality, then you can
 * set these function pointers to the EmptyFunction.
 */

void (*fn_release_A)(void);
void (*fn_press_A)(void);
void (*fn_release_C)(void);
void (*fn_press_C)(void);

volatile uint8_t pinb_previous;

#endif
