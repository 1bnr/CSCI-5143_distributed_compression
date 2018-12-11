#ifdef VIRTUAL_SERIAL
#include <VirtualSerial.h>
#else
#warning VirtualSerial not defined, USB IO may not work
#define SetupHardware();
#define USB_Mainloop_Handler();
#endif

void handleInput( char c );
void handleCommand( char *command );
