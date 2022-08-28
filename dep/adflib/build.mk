ADFLIB_SRCS = \
	dep/adflib/src/adf_bitm.c \
	dep/adflib/src/adf_cache.c \
	dep/adflib/src/adf_dir.c \
	dep/adflib/src/adf_disk.c \
	dep/adflib/src/adf_dump.c \
	dep/adflib/src/adf_env.c \
	dep/adflib/src/adf_file.c \
	dep/adflib/src/adf_hd.c \
	dep/adflib/src/adf_link.c \
	dep/adflib/src/adf_raw.c \
	dep/adflib/src/adf_salv.c \
	dep/adflib/src/adf_util.c \

ADFLIB_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(ADFLIB_SRCS))
$(ADFLIB_OBJS): CFLAGS += -Idep/adflib/src -Idep/adflib
ADFLIB_LIB = $(OBJDIR)/libadflib.a
$(ADFLIB_LIB): $(ADFLIB_OBJS)
ADFLIB_CFLAGS = -Idep/adflib/src
ADFLIB_LDFLAGS = $(ADFLIB_LIB)

