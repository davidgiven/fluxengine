PACKAGES = zlib sqlite3 libusb-1.0

export CFLAGS = -Os -g --std=c++14 \
	-ffunction-sections -fdata-sections
export LDFLAGS = -Os

ifeq ($(OS), Windows_NTz)
export CXX = /mingw32/bin/g++
export AR = /mingw32/bin/ar rcs
export STRIP = /mingw32/bin/strip
export CFLAGS += -I/mingw32/include/libusb-1.0
export LDFLAGS +=
export LIBS = -static -lz -lsqlite3 -lusb-1.0
export EXTENSION = .exe
else

packages-exist = $(shell pkg-config --exists $(PACKAGES))
ifneq ($(packages-exist), 0)
$(warning These pkg-config packages are installed: $(shell pkg-config --list-all | awk '{print $$1}'))
$(error You must have these pkg-config packages installed: $(PACKAGES) But only)
endif

export CXX = g++
export AR = ar rcs
export STRIP = strip
export CFLAGS += $(shell pkg-config --cflags $(PACKAGES))
export LDFLAGS +=
export LIBS = $(shell pkg-config --libs $(PACKAGES))
export EXTENSION =
endif

CFLAGS += -Ilib -Idep/fmt -Iarch

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

