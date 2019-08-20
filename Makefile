PACKAGES = zlib sqlite3 libusb-1.0

export CFLAGS = -O3 -g --std=c++14 \
	-ffunction-sections -fdata-sections
export LDFLAGS = -O3

ifeq ($(OS), Windows_NT)
export CXX = /mingw32/bin/g++
export AR = /mingw32/bin/ar rcs
export STRIP = /mingw32/bin/strip
export CFLAGS += -I/mingw32/include/libusb-1.0
export LDFLAGS +=
export LIBS = -static -lz -lsqlite3 -lusb-1.0
export EXTENSION = .exe
else
export CXX = g++
export AR = ar rcs
export STRIP = strip
export CFLAGS += $(shell pkg-config --cflags $(PACKAGES))
export LDFLAGS +=
export LIBS = $(shell pkg-config --libs $(PACKAGES))
export EXTENSION =
endif

export CFLAGS += -Ilib -Idep/fmt -Iarch

export OBJDIR = .obj

all: .obj/build.ninja
	@ninja -f .obj/build.ninja -v

clean:
	@echo CLEAN
	@rm -rf $(OBJDIR)

.obj/build.ninja: mkninja.sh Makefile
	@echo MKNINJA $@
	@mkdir -p $(OBJDIR)
	@sh $< > $@

