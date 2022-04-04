PACKAGES = zlib sqlite3 protobuf

export CFLAGS = \
	-ffunction-sections -fdata-sections
export CXXFLAGS = $(CFLAGS) \
	-x c++ --std=gnu++2a \
	-Wno-deprecated-enum-enum-conversion \
	-Wno-deprecated-enum-float-conversion
export LDFLAGS = -pthread

export COPTFLAGS = -Os
export LDOPTFLAGS = -Os

export CDBGFLAGS = -O0 -g
export LDDBGFLAGS = -O0 -g

ifeq ($(OS), Windows_NT)
else
ifeq ($(shell uname),Darwin)
else
	PACKAGES += libudev
endif
endif

ifeq ($(OS), Windows_NT)
export PROTOC = /mingw32/bin/protoc
export CC = /mingw32/bin/gcc
export CXX = /mingw32/bin/g++
export AR = /mingw32/bin/ar rc
export RANLIB = /mingw32/bin/ranlib
export STRIP = /mingw32/bin/strip
export CFLAGS += -I/mingw32/include
export CXXFLAGS += $(shell wx-config --cxxflags --static=yes)
export GUILDFLAGS += -lmingw32
export LIBS += -L/mingw32/lib -static\
	-lsqlite3 -lz \
	-lsetupapi -lwinusb -lole32 -lprotobuf -luuid
export GUILIBS += -L/mingw32/lib -static -lsqlite3 \
	$(shell wx-config --libs --static=yes core base) -lz \
	-lcomctl32 -loleaut32 -lspoolss -loleacc -lwinspool \
	-lsetupapi -lwinusb -lole32 -lprotobuf -luuid
export EXTENSION = .exe
else

packages-exist = $(shell pkg-config --exists $(PACKAGES) && echo yes)
ifneq ($(packages-exist),yes)
$(warning These pkg-config packages are installed: $(shell pkg-config --list-all | sort | awk '{print $$1}'))
$(error You must have these pkg-config packages installed: $(PACKAGES))
endif
wx-exist = $(shell wx-config --cflags > /dev/null && echo yes)
ifneq ($(wx-exist),yes)
$(error You must have these wx-config installed)
endif

export PROTOC = protoc
export CC = gcc
export CXX = g++
export AR = ar rc
export RANLIB = ranlib
export STRIP = strip
export CFLAGS += $(shell pkg-config --cflags $(PACKAGES)) $(shell wx-config --cxxflags)
export LDFLAGS +=
export LIBS += $(shell pkg-config --libs $(PACKAGES))
export GUILIBS += $(shell wx-config --libs core base)
export EXTENSION =

ifeq ($(shell uname),Darwin)
AR = ar rcS
RANLIB += -c -no_warning_for_no_symbols
export CC = clang
export CXX = clang++
export COBJC = clang
export LDFLAGS += -framework IOKit -framework CoreFoundation
export CFLAGS += -Wno-deprecated-declarations
endif

endif
export XXD = xxd

CFLAGS += -Ilib -Idep/fmt -Iarch

export OBJDIR = .obj

all: .obj/build.ninja
	@ninja -f .obj/build.ninja
	@if command -v cscope > /dev/null; then cscope -bRq; fi

clean:
	@echo CLEAN
	@rm -rf $(OBJDIR)

.obj/build.ninja: mkninja.sh Makefile
	@echo MKNINJA $@
	@mkdir -p $(OBJDIR)
	@sh $< > $@

compdb:
	@ninja -f .obj/build.ninja -t compdb > compile_commands.json
