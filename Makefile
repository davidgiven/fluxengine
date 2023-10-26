CC = gcc
CXX = g++ -std=c++17
CFLAGS = -g -Os
LDFLAGS =

OBJ = .obj

# Special Windows settings.

ifeq ($(OS), Windows_NT)
	EXT ?= .exe
	MINGWBIN = /mingw32/bin
	CCPREFIX = $(MINGWBIN)/
	PKG_CONFIG = $(MINGWBIN)/pkg-config
	WX_CONFIG = /usr/bin/sh $(MINGWBIN)/wx-config --static=yes
	PROTOC = $(MINGWBIN)/protoc
	LDFLAGS += \
		-static
	CXXFLAGS += \
		-fext-numeric-literals \
		-Wno-deprecated-enum-float-conversion \
		-Wno-deprecated-enum-enum-conversion

	# Required to get the gcc run - time libraries on the path.
	export PATH := $(PATH):$(MINGWBIN)
endif

# Special OSX settings.

ifeq ($(shell uname),Darwin)
	LDFLAGS += \
		-framework IOKit \
		-framework Foundation 
endif

.PHONY: all
all: +all README.md
	
README.md: $(OBJ)/scripts+mkdocindex/scripts+mkdocindex$(EXT)
	@echo MKDOC $@
	@csplit -s -f$(OBJ)/README. README.md '/<!-- FORMATSSTART -->/' '%<!-- FORMATSEND -->%'
	@(cat $(OBJ)/README.00 && $< && cat $(OBJ)/README.01) > README.md

include build/ab.mk
