# Basic Serial Communication

(From Jonathan Walz - SENG5831 Spring 2016)

This is a basic example that enables serial communication with stdio.

(Note: I've only tested this on a mac for an ATmega32U4 for seng-5831S16)

# Download VirtualSerial and Set-Up Path

1. Download the [current VirtualSerial.tgz] (https://github.umn.edu/jpwalz/basic-lufa-serial/releases/latest) archive.
2. Extract the library somewhere on your machine and take note of the path. This will be assigned to VIRTUAL_SERIAL_PATH.
3. At the OS command prompt, add that path
  ```export VIRTUAL_SERIAL_PATH=/Users/amylarson/RTES/VirtualSerial```
4. Use stdin/stdout with the [stdio] (http://www.nongnu.org/avr-libc/user-manual/group__avr__stdio.html) library.


# Download LUFA, Set-Up Path, make library

1. Download [LUFA] (http://www.fourwalledcubicle.com/LUFA.php). ([Direct Download] (http://www.github.com/abcminiuser/lufa/archive/LUFA-151115.zip))
2. Extract LUFA somewhere on your machine and take note of the path. This will be assigned to LUFA_PATH.
3. At the OS command prompt, add that path
  ```export LUFA_PATH=/Users/amylarson/RTES/lufa-LUFA-151115/LUFA/```
4. Move into the 'basic-lufa' directory.
4. `make`

# Set-Up hardware for communication using LUFA and USB connection.

1. Plug in device with USB connector. At command prompt: 'ls /dev/tty*' to see device name options. (It should be something like 'dev/tty.usbmodem1411').
2. Open makefile in the 'example' folder of 'basic-lufa-menu'.
3. Set PORT to the device (e.g. 'PORT=/dev/tty.usbmodem1411').
4. Double-press reset on the board to put into bootloader mode.
5. At command prompt within "example" directory: `make program`

# Connect

1. Open a new terminal window (MAC and Linux only - I think).
2. Open connection using `screen /dev/tty.usbmodem1411 -f 9600`.
3. Test connection: `return` should display Hello message to the screen.
4. To exit connection: `ctrl-a` then `ctrl-\` then `y` (There must be an easier way :-)

If on Windows, need to use PutTy or something like it to make the connection.
