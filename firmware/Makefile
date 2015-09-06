# Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
# 
# This is free software: you can redistribute it and/or modify
# it under the terms of the Artistic License 2.0 as published by
# The Perl Foundation.
# 
# This source is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# Artistic License 2.0 for more details.
# 
# You should have received a copy of the Artistic License 2.0
# along the source as a COPYING file. If not, obtain it from
# http://www.perlfoundation.org/artistic_license_2_0.

BINARY := firmware

SOURCES := firmware.c
SOURCES += mod_cdcacm.c
SOURCES += jbig85.c
SOURCES += jbig_ar.c

RELEASE := 0x0001
REVISION := 0
VERSION_MAJOR := 0
VERSION_MINOR := 1
VERSION_PATCH := 0
VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)-Rev$(REVISION)

CFLAGS := -DREVISION=$(REVISION)
CFLAGS += -DVERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS += -DVERSION_MINOR=$(VERSION_MINOR)
CFLAGS += -DVERSION_PATCH=$(VERSION_PATCH)
CFLAGS += -DVERSION="\"$(VERSION)\""

SRCLIBDIR = ./libopencm3
DEVICE := stm32f303cc
LDSCRIPT := ./F303CC.ld

all: $(BINARY).bin

## This is the place where the translation DEVICE->LDSCRIPT will be executed
#include $(SRCLIBDIR)/ld/Makefile.linker

include ./libopencm3.target.mk

lib:
	$(Q)$(MAKE) -C libopencm3

dfu: $(BINARY).dfu

%.dfu: %.bin
	dfuse_pack \
		-o $@ \
		-f $(RELEASE) \
		-v 0x0483 \
		-p 0xdf11 \
			-n "Chimaera USB $(VERSION). Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch). Released under Artistic License 2.0.  By Open Music Kontrollers (http://open-music-kontrollers.ch)." \
			-m 0x08000000 -i $< \
			-a 0

download:	$(BINARY).bin
	dfu-util -a 0 -d $(USB_VENDOR):$(USB_PRODUCT) -s 0x08000000:leave -D $<
