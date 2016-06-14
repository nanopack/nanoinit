# -*- mode: make; tab-width: 4; indent-tabs-mode: 1; st-rulers: [80] -*-
# vim: ts=8 sw=8 ft=make noet

.PHONY: all deb

all:	nanoinit

nanoinit: nanoinit.c nanoinit.h
	gcc -std=gnu11 -o nanoinit nanoinit.c

deb: nanoinit
	install -d deb/bin
	install -s nanoinit deb/bin/nanoinit
	fakeroot dpkg-deb --build deb nanoinit_0.0.1_amd64.deb