PACKAGES = zlib sqlite3 libusb-1.0 protobuf

export CFLAGS = -x c++ --std=c++14 -ffunction-sections -fdata-sections
export LDFLAGS = -pthread

export COPTFLAGS = -Os
export LDOPTFLAGS = -Os

export CDBGFLAGS = -O0 -g
export LDDBGFLAGS = -O0 -g

ifeq ($(shell uname),Linux)
LIBS += -ludev
endif

ifeq ($(OS), Windows_NT)
export PROTOC = /mingw32/bin/protoc
export CXX = /mingw32/bin/g++
export AR = /mingw32/bin/ar rc
export RANLIB = /mingw32/bin/ranlib -c
export STRIP = /mingw32/bin/strip
export CFLAGS += -I/mingw32/include/libusb-1.0 -I/mingw32/include
export LDFLAGS +=
export LIBS += -L/mingw32/lib -static -lz -lsqlite3 -lusb-1.0 -lprotobuf
export EXTENSION = .exe
else

packages-exist = $(shell pkg-config --exists $(PACKAGES) && echo yes)
ifneq ($(packages-exist),yes)
$(warning These pkg-config packages are installed: $(shell pkg-config --list-all | sort | awk '{print $$1}'))
$(error You must have these pkg-config packages installed: $(PACKAGES))
endif

export PROTOC = protoc
export CXX = g++
export AR = ar rc
export RANLIB = ranlib -c
export STRIP = strip
export CFLAGS += $(shell pkg-config --cflags $(PACKAGES))
export LDFLAGS +=
export LIBS += $(shell pkg-config --libs $(PACKAGES))
export EXTENSION =
endif
export XXD = xxd

ifeq ($(shell uname),Darwin)
AR = ar rcS
RANLIB += -no_warning_for_no_symbols
endif

CFLAGS += -Ilib -Idep/fmt -Iarch

export OBJDIR = .obj

all: .obj/build.ninja
	@ninja -f .obj/build.ninja
	@(command -v cscope > /dev/null) && cscope -bRq

clean:
	@echo CLEAN
	@rm -rf $(OBJDIR)

.obj/build.ninja: mkninja.sh Makefile
	@echo MKNINJA $@
	@mkdir -p $(OBJDIR)
	@sh $< > $@

