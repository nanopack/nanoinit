# -*- mode: make; tab-width: 4; indent-tabs-mode: 1; st-rulers: [80] -*-
# vim: ts=8 sw=8 ft=make noet

all:	nanoinit

nanoinit: nanoinit.c nanoinit.h
	gcc -std=gnu11 -o nanoinit nanoinit.c

build:
	docker run --rm -v `pwd`:/usr/src/myapp -w /usr/src/myapp gcc:4.9 gcc -std=gnu11 -o nanoinit nanoinit.c
