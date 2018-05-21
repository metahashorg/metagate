#/usr/bin/python

import sys
import os
import subprocess
import shlex

if len(sys.argv) != 2:
    print("path_to_file\nFile format:\naddress")
    exit()

fname = sys.argv[1];

with open(fname) as f:
    content = f.readlines()

countUps = 0
for line in content:
    line = line.strip()
    if line:
        answer = os.popen("openssl ec -in ./mth/0x" + line + ".ec.priv -pubout -outform DER  -passin pass:1 |tail -c 65|xxd -p -c 65").read().strip()
        #print("answer " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1]
        #print("answer2 " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -rmd160").read().strip()
        answer0 = "00" + answer.split(' ')[1]
        #print("answer3 " + answer0)
        answer = os.popen("echo " + answer0 + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1]
        #print("answer3 " + answer)
        answer = os.popen("echo " + answer + " | xxd -r -p | openssl dgst -sha256").read().strip()
        answer = answer.split(' ')[1][:8]
        #print("answer3 " + answer)
        ans = answer0 + answer
        if ans != line:
            print("Ups " + line)
            countUps += 1
        
print("End. Count ups " + str(countUps));
