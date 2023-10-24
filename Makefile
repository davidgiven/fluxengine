CC = gcc
CXX = g++ -std=c++17
CFLAGS = -g -Os

OBJ = .obj

.PHONY: all
all: +all README.md
	
README.md: $(OBJ)/scripts+mkdocindex/scripts+mkdocindex
	@echo MKDOC $@
	@csplit -s -f$(OBJ)/README. README.md '/<!-- FORMATSSTART -->/' '%<!-- FORMATSEND -->%'
	@(cat $(OBJ)/README.00 && $< && cat $(OBJ)/README.01) > README.md

include build/ab.mk
