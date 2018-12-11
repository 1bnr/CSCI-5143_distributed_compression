This is the slave device.

In lufa.c there is a function slave_setup();
This function declares the address for the slave.  For the demonstration we would compile and send this to one slave with the address 0x51 and then change the address, recompile, then send it to the second slave.

Lufa menu z will wait for the master to send the buffer and then send it right back to the master.

Lufa menu w will send a predefined buffer to the master device.

Lufa menu r will wait for the master to send the buffer.
