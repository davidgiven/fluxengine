CFLAGS = -g -Wall $(shell pkg-config --cflags libusb-1.0)
LDFLAGS = -g $(shell pkg-config --libs libusb-1.0)

SRCS = main.c usb.c
OBJS = $(patsubst %.c, .objs/%.o, $(SRCS))

fluxclient: $(OBJS) Makefile
	gcc -o fluxclient $(OBJS) $(LDFLAGS)

.objs/%.o: %.c
	@mkdir -p $(dir $@)
	gcc -c $(CFLAGS) -o $@ $<

$(OBJS): globals.h protocol.h Makefile
