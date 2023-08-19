HFSUTILS_SRCS = \
	dep/hfsutils/libhfs/block.c \
	dep/hfsutils/libhfs/btree.c \
	dep/hfsutils/libhfs/data.c \
	dep/hfsutils/libhfs/file.c \
	dep/hfsutils/libhfs/hfs.c \
	dep/hfsutils/libhfs/low.c \
	dep/hfsutils/libhfs/medium.c \
	dep/hfsutils/libhfs/memcmp.c \
	dep/hfsutils/libhfs/node.c \
	dep/hfsutils/libhfs/record.c \
	dep/hfsutils/libhfs/version.c \
	dep/hfsutils/libhfs/volume.c \
	
HFSUTILS_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(HFSUTILS_SRCS))
$(HFSUTILS_OBJS): CFLAGS += -Idep/hfsutils/libhfs
HFSUTILS_LIB = $(OBJDIR)/libhfsutils.a
$(HFSUTILS_LIB): $(HFSUTILS_OBJS)
HFSUTILS_CFLAGS = -Idep/hfsutils/libhfs
HFSUTILS_LDFLAGS =
OBJS += $(HFSUTILS_OBJS)

