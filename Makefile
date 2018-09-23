CFLAGS = -Wall

SRCS = main.c windows.c
OBJS = $(patsubst %.c, .objs/%.o, $(SRCS))

fluxclient: $(OBJS)
	gcc -o fluxclient $(OBJS)

.objs/%.o: %.c
	@mkdir -p $(dir $@)
	gcc -c $(CFLAGS) -o $@ $<

$(OBJS): globals.h protocol.h
