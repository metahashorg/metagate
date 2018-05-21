#/usr/bin/python

import sys
import os
import subprocess
import shlex

if len(sys.argv) != 2:
    print("path_to_file\nFile format:\naddress keyHex")
    exit()

fname = sys.argv[1];

with open(fname) as f:
    content = f.readlines()

countUps = 0
for line in content:
    line = line.strip()
    if line:
        addr, key = line.split(' ')
        answer = os.popen("echo " + key + " | xxd -r -p | openssl ec -inform DER -pubout -outform DER |tail -c 65|xxd -p -c 65").read().strip()
        #print("answer " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1]
        #print("answer2 " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -rmd160").read().strip()
        answer0 = "08" + answer.split(' ')[1]
        #print("answer3 " + answer0)
        answer = os.popen("echo " + answer0 + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1]
        #print("answer3 " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1][:8]
        #print("answer3 " + answer)
        ans = answer0 + answer
        if ans != addr:
            print("Ups " + addr + " " + ans)
            countUps += 1
        
print("End. Count ups " + str(countUps));
