# Student's Makefile for the CS:APP Performance Lab
TEAM = bovik
VERSION = 1
HANDINDIR = 

CC = gcc
CFLAGS = -Wall -O2 -msse4.2 -std=gnu99
LIBS = -lm

OBJS = driver.o kernels.o fcyc.o clock.o baselines.o

all: driver

driver: $(OBJS) fcyc.h clock.h defs.h config.h baselines.h
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o driver

handin:
	cp kernels.c $(HANDINDIR)/$(TEAM)-$(VERSION)-kernels.c

clean: 
	-rm -f $(OBJS) driver core *~ *.o


