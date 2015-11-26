#!/bin/bash
gcc -I"/home/utnso/git/tp-2015-2c-so-didnt-c-that-coming/api" -I"/home/utnso/git/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/cpu.d" -MT"src/cpu.d" -o "src/cpu.o" "../src/cpu.c"

gcc -I"/home/utnso/git/tp-2015-2c-so-didnt-c-that-coming/api" -I"/home/utnso/git/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/interprete.d" -MT"src/interprete.d" -o "src/interprete.o" "../src/interprete.c"

gcc -L"/home/utnso/git/commons/Debug" -L"/home/utnso/git/tp-2015-2c-so-didnt-c-that-coming/api/Debug" -o "cpu"  ./src/cpu.o ./src/interprete.o   -lcommons -lapi -lpthread

