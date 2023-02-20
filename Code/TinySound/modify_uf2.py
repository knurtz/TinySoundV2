#!/bin/env python

"""
This tool modifies a UF2 file by cutting off all blocks outside a certain flash region, defined by a limit address.
The program looks for the first block of data with a target address bigger or equal to the limit and only copies the blocks before this one to a new UF2 file.
The input file therefore needs to have all blocks in sequential order to function properly.

Syntax: p<thon modify_uf2.py [FILENAME] [LIMIT_ADDRESS]        # address can either be DEC or 0xHEX
"""

import sys
import os
import time

address_limit = 0
filename = ""

# Wait for source UF2 to fnish being written
time.sleep(.2)

try:
    filename = sys.argv[1]
except:
    print("Please provide target filename as the first argument")
    exit()

try:
    address_limit_str = sys.argv[2]
    if "0x" in address_limit_str:
        address_limit = int(address_limit_str, base = 16)
    else:
        address_limit = int(address_limit_str, base = 10)    
except:
    print("Please provide address limit as the second argument")
    exit()

# "rb" / "wb" -> read / write binary data
source_uf2 = open(filename, "rb")
target_uf2 = open(filename[:-4] + "_short.uf2", "wb")

print("")   # Begin with empty line to seperate output
print("Modifying file: %s" % filename)
print("Source UF2 created %.2f seconds ago." % (time.time() - os.path.getmtime(filename)))
print("All sections from target address 0x%x and up will not be written to flash." % address_limit)

source_uf2.seek(24)             # total number of blocks is in bytes 24 - 27
total_blocks = int.from_bytes(source_uf2.read(4), "little")

old_addr = 0                    # remember the address of the last block we looked at
last_printed_block = 0          # remember the last block number that has been printed
wanted_blocks = 0               # count up the amount of blocks until address limit is reached

# Scan source file
for i in range(0, total_blocks):

    # get address of next block, every block is 512 bytes long
    source_uf2.seek(i * 512 + 12)    # address is in bytes 12 - 15
    new_addr = int.from_bytes(source_uf2.read(4), "little")

    # do something if there is more than a 256 bytes jump between blocks
    if new_addr - old_addr > 256:
        # print last block
        if i - last_printed_block > 1:
            print("...")    # ellipses for gap of one block or more
        if i != 0:          # special case for first block
            print("Block %i, address 0x%x" % (i - 1, old_addr))

        print("Block %i, address 0x%x" % (i, new_addr))
        last_printed_block = i
    
    old_addr = new_addr

    # check if block has to be written to flash
    if (new_addr < address_limit):
        wanted_blocks = wanted_blocks + 1

# print last block if it hasn't already
if last_printed_block < total_blocks - 1:
    print("...")
    print("Block %i, address 0x%x" % (total_blocks - 1, old_addr))    

# Create new UF2 file only containing blocks up to address limit
wanted_blocks_bytes = wanted_blocks.to_bytes(4, "little")
target_uf2.seek(0)

for i in range (wanted_blocks):
    source_uf2.seek(i * 512)                # go to the beginning of the block
    target_uf2.write(source_uf2.read(24))   # and copy the first 24 bytes
    target_uf2.write(wanted_blocks_bytes)   # write new total block count
    source_uf2.seek(i * 512 + 28)           # skip these 4 bytes in source
    target_uf2.write(source_uf2.read(484))  # and copy the rest of the block

source_uf2.close()
target_uf2.close()