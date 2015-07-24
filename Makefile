# -*- mode: make; tab-width: 4; indent-tabs-mode: 1; st-rulers: [80] -*-
# vim: ts=8 sw=8 ft=make noet

all:	nanoinit

nanoinit: nanoinit.c nanoinit.h
	gcc -std=gnu11 -o nanoinit nanoinit.c