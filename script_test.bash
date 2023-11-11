#!/bin/bash
echo "-------------------------------Start--------------------------------"
echo " "
gcc --std=gnu99 -o smallsh smallsh.c 

chmod +x ./p3testscript
./p3testscript 2>&1
echo " "
echo "--------------------------------End--------------------------------"