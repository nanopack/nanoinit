all:	nanoinit

nanoinit: nanoinit.c nanoinit.h
	gcc -std=gnu11 -o nanoinit nanoinit.c