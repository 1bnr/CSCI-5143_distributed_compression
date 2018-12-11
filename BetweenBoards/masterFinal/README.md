This is the master device.

It uses lufa with menu option z to send a preset buffer to the two slave devices.

The slave devices receive the buffer and the length of the buffer and will send the buffer back to the master.

Lufa menu w will send a predefined buffer to slave with address 0x51.
Lufa menu r will receive a buffer from slave with address 0x52.
