#!/bin/bash

#compiler

CC = gcc
#flags
CFLAGS = -pthread
all: aubatch

aubatch:$(OBJS)
	@$(CC) $(CFLAGS) randomize.c help.c aubatch.c -o aubatch
