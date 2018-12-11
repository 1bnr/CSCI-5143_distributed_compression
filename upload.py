#! /usr/bin/env python3

import serial
import sys
import struct

DEVICE_PORT = "/dev/ttyACM0"

if len(sys.argv) < 2:
    print("Please specify a filename")
    exit()

fname = sys.argv[1]

f = open(fname, 'rb')
content = bytearray(f.read())

dev = serial.Serial(DEVICE_PORT)

print("writing %s to %s (%s bytes)" % (fname, dev.name, len(content)))

def pack_uint16(n):
    return struct.pack('<H', n)

dev.write('\r\n')
dev.write('p') # initialize pagewrite mode
dev.write('\r\ntest\r\n')
dev.write(pack_uint16(len(content))) # total amount of bytes to write
dev.write(content)

def printline(dev):
    print(dev.readline().decode('ascii'))

while True:
    printline(dev)
