MAKENOT4 := $(if $(findstring 3.9999, $(lastword $(sort 3.9999 $(MAKE_VERSION)))),yes,no)

ifeq ($(MAKENOT4),yes)
$(error You need GNU Make 4.x for this (if you're on OSX, use gmake).)
endif

OBJ ?= .obj
PYTHON ?= python3
PKG_CONFIG ?= pkg-config
HOST_PKG_CONFIG ?= $(PKG_CONFIG)
ECHO ?= echo
CP ?= cp

HOSTCC ?= gcc
HOSTCXX ?= g++
HOSTAR ?= ar
HOSTCFLAGS ?= -g -Og
HOSTLDFLAGS ?= -g

CC ?= $(HOSTCC)
CXX ?= $(HOSTCXX)
AR ?= $(HOSTAR)
CFLAGS ?= $(HOSTCFLAGS)
LDFLAGS ?= $(HOSTLDFLAGS)

export PKG_CONFIG
export HOST_PKG_CONFIG

ifdef VERBOSE
	hide =
else
	ifdef V
		hide =
	else
		hide = @
	endif
endif

# If enabled, shows a nice display of how far through the build you are. This
# doubles Make startup time. Also, on Make 4.3 and above, rebuilds don't show
# correct progress information.
AB_ENABLE_PROGRESS_INFO ?= true

WINDOWS := no
OSX := no
LINUX := no
ifeq ($(OS),Windows_NT)
    WINDOWS := yes
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		LINUX := yes
    endif
    ifeq ($(UNAME_S),Darwin)
		OSX := yes
    endif
endif

ifeq ($(OS), Windows_NT)
	EXT ?= .exe
endif
EXT ?=

CWD=$(shell pwd)

ifeq ($(AB_ENABLE_PROGRESS_INFO),true)
	ifeq ($(PROGRESSINFO),)
	# The first make invocation here has to have its output discarded or else it
	# produces spurious 'Leaving directory' messages... don't know why.
	rulecount := $(strip $(shell $(MAKE) --no-print-directory -q $(OBJ)/build.mk PROGRESSINFO=1 > /dev/null \
		&& $(MAKE) --no-print-directory -n $(MAKECMDGOALS) PROGRESSINFO=XXXPROGRESSINFOXXX | grep XXXPROGRESSINFOXXX | wc -l))
	ruleindex := 1
	PROGRESSINFO = "[$(ruleindex)/$(rulecount)]$(eval ruleindex := $(shell expr $(ruleindex) + 1)) "
	endif
else
	PROGRESSINFO = ""
endif

PKG_CONFIG_HASHES = $(OBJ)/.pkg-config-hashes/target-$(word 1, $(shell $(PKG_CONFIG) --list-all | md5sum))
HOST_PKG_CONFIG_HASHES = $(OBJ)/.pkg-config-hashes/host-$(word 1, $(shell $(HOST_PKG_CONFIG) --list-all | md5sum))

$(OBJ)/build.mk : $(PKG_CONFIG_HASHES) $(HOST_PKG_CONFIG_HASHES)
$(PKG_CONFIG_HASHES) $(HOST_PKG_CONFIG_HASHES) &:
	$(hide) rm -rf $(OBJ)/.pkg-config-hashes
	$(hide) mkdir -p $(OBJ)/.pkg-config-hashes
	$(hide) touch $(PKG_CONFIG_HASHES) $(HOST_PKG_CONFIG_HASHES)

include $(OBJ)/build.mk

ifeq ($(OSX),yes)
	MAKEFLAGS += -r -j$(shell sysctl -n hw.logicalcpu)
else
	MAKEFLAGS += -r -j$(shell nproc)
endif

.DELETE_ON_ERROR:

.PHONY: update-ab
update-ab:
	@echo "Press RETURN to update ab from the repository, or CTRL+C to cancel." \
		&& read a \
		&& (curl -L https://github.com/davidgiven/ab/releases/download/dev/distribution.tar.xz | tar xvJf -) \
		&& echo "Done."

.PHONY: clean
clean::
	@echo CLEAN
	$(hide) rm -rf $(OBJ)

export PYTHONHASHSEED = 1
build-files = $(shell find . -name 'build.py') $(wildcard build/*.py) $(wildcard config.py)
$(OBJ)/build.mk: Makefile $(build-files) build/ab.mk
	@echo "AB"
	@mkdir -p $(OBJ)
	$(hide) $(PYTHON) -X pycache_prefix=$(OBJ)/__pycache__ build/ab.py -o $@ build.py \
		|| rm -f $@
