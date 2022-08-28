ifeq ($(shell $(PKG_CONFIG) stb; echo $$?), 0)

# System libstb present.

STB_LIB =
STB_CFLAGS := $(shell $(PKG_CONFIG) --cflags stb)
STB_LDFLAGS := $(shell $(PKG_CONFIG) --libs stb)

else

STB_SRCS = \
	dep/stb/stb_image_write.c
	
STB_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(STB_SRCS))
$(STB_OBJS): CFLAGS += -Idep/stb/src
STB_LIB = $(OBJDIR)/libstb.a
$(STB_LIB): $(STB_OBJS)
STB_CFLAGS =
STB_LDFLAGS = $(STB_LIB)
OBJS += $(STB_OBJS)

endif

