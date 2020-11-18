# SPDX-License-Identifier: (MIT)
#
# Copyright (c) 2020 Institute for Control Engineering of Machine Tools and Manufacturing Units, University of Stuttgart
# Author: Philipp Neher <philipp.neher@isw.uni-stuttgart.de>
#

INC = -I. -I..
CC=gcc
CFLAGS=$(INC) -g

ODIR=obj
LDIR=../lib

LIBS=-lrt -pthread

_OBJ = demoreader.o demowriter.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: demoreader demowriter

demoreader: obj/demoreader.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

demowriter: obj/demowriter.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ demoreader demowriter