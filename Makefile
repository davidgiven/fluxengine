CFLAGS = -g -Wall $(shell pkg-config --cflags libusb-1.0) $(shell pkg-config --cflags sqlite3)
LDFLAGS = -g $(shell pkg-config --libs libusb-1.0) $(shell pkg-config --libs sqlite3)

SRCS = \
	main.c \
	usb.c \
	sql.c \
	encoder.c \
	fluxmap.c \
	cmd_rpm.c \
	cmd_usbbench.c \
	cmd_read.c \
	cmd_write.c \
	cmd_mfmdecode.c \
	cmd_testpattern.c \
	cmd_fluxdump.c \
	cmd_calibrate.c \
	cmd_getclock.c \

OBJS = $(patsubst %.c, .objs/%.o, $(SRCS))

fluxclient: $(OBJS) Makefile
	gcc -o fluxclient $(OBJS) $(LDFLAGS)

.objs/%.o: %.c
	@mkdir -p $(dir $@)
	gcc -c $(CFLAGS) -o $@ $<

$(OBJS): globals.h protocol.h Makefile
