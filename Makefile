CC = gcc
CXX = g++
CFLAGS = -g -Os

OBJ = .obj

.PHONY: all
all: +all
	
include build/ab.mk
