ifeq ($(OS), Windows_NT)

EMU_SRCS = \
	dep/emu/fnmatch.c

EMU_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(EMU_SRCS))
$(EMU_OBJS): CFLAGS += -Idep/emu
EMU_LIB = $(OBJDIR)/libemu.a
$(EMU_LIB): $(EMU_OBJS)
EMU_CFLAGS = -Idep/emu
EMU_LDFLAGS = $(EMU_LIB)

else

EMU_LIB =
EMU_CFLAGS =
EMU_LDFLAGS =

endif

