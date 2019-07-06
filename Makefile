PACKAGES = zlib slite3 libusb-1.0

ifeq ($(OS), Windows_NT)
export CXX = i686-w64-mingw32-g++
export AR = i686-w64-mingw32-ar rcs
export STRIP = i686-w64-mingw32-strip
export CFLAGS = -Og -g --std=c++14 -I/usr/i686-w64-mingw32/sys-root/mingw/include/libusb-1.0
export LDFLAGS = -Og
export LIBS = -static -lz -lsqlite3 -Wl,-Bdynamic -lusb-1.0
else
export CXX = g++
export AR = ar rcs
export STRIP = strip
export CFLAGS = -Og -g --std=c++14 $(pkg-config --cflags $(PACKAGES))
export LDFLAGS = -Og
export LIBS = $(pkg-config --libs $(PACKAGES))
endif

CFLAGS += -Ilib -Idep/fmt

export OBJDIR = .obj

all: .obj/build.ninja
	@ninja -f .obj/build.ninja -v

clean:
	@echo CLEAN
	@rm -rf $(OBJDIR)

.obj/build.ninja: mkninja.sh
	@echo MKNINJA $@
	@mkdir -p $(OBJDIR)
	@sh $< > $@

