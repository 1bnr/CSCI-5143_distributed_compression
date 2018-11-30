#! /usr/bin/env python3

import serial
import sys

DEVICE_PORT = "/dev/ttyACM0"
BLOCK_SIZE = 64 # 64 bytes per line

if len(sys.argv) < 2:
    print("Please specify a filename")
    exit()

fname = sys.argv[1]

f = open(fname, 'rb')
content = bytearray(f.read())

dev = serial.Serial(DEVICE_PORT)

print("writing %s to %s (%s bytes)" % (fname, dev.name, len(content)))

dev.write(b'p\r\n') # initialize pagewrite mode
dev.write(str(len(content)).encode('ascii')) # total amount of bytes to write
dev.write(b'\r\n')
dev.write(str(BLOCK_SIZE).encode('ascii')) # blocksize
dev.write(b'\r\n')

def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]

for block in chunks(content, BLOCK_SIZE):
    dev.write(block)
    dev.write(b'\r\n')

def printline(dev):
    print(dev.readline().decode('ascii'), end='')

while True:
    printline(dev)