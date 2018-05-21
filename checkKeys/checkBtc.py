#/usr/bin/python

import sys
import os

if len(sys.argv) != 2 or sys.argv[1] == "--help":
    print("path_to_file\nFile format:\naddress wif")
    exit()

fname = sys.argv[1];

with open(fname) as f:
    content = f.readlines()

for line in content:
    line = line.strip()
    if line:
        addr, key = line.split(' ')
        #print(addr)
        #print(key)
        answer = os.popen("./bx wif-to-public " + key + " | ./bx ec-to-address").read().strip()
        if answer != addr:
            print("Ups " + addr)
        
print("End");
