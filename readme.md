This project was supposed to allow for distributed computing between arduinos where the master device has a ram chip that allowed for larger problems than a single arduino can do on its own.


# huffman 
This is where the huffman transformation lives.

# main 
This is not actually the main folder.  This is where the communcation between the ram chips is located.  
It has a predefined buffer that is sends to the ram chip and then it can be read back.

# bwt
This is where the attempt at burrows wheeler transform is.  We were unable to get this working within the ram constaints of the arduino.

# Between boards
This is where the master and slave devices are setup to communicate between eachother.
Look at the reame to see how to use their lufa menus and how to compile the slaves.

For each of these in Between boards the sda and scl lines must be together and the boards must share a common ground. 

# upload.py
This is a python script used to send the input to the master board.  The input being the input to the problem that we would like the arduinos to do.
