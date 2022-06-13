ifeq ($(shell $(PKG_CONFIG) stb; echo $$?), 0)

# System libstb present.

STB_LIB =
STB_CFLAGS := $(shell $(PKG_CONFIG) --cflags stb)
STB_LDFLAGS := $(shell $(PKG_CONFIG) --libs stb)

else

STB_SRCS = \
	dep/libusbp/src/async_in_pipe.c \
	dep/libusbp/src/error.c \
	dep/libusbp/src/error_hresult.c \
	dep/libusbp/src/find_device.c \
	dep/libusbp/src/linux/async_in_transfer_linux.c \
	dep/libusbp/src/linux/device_linux.c \
	dep/libusbp/src/linux/error_linux.c \
	dep/libusbp/src/linux/generic_handle_linux.c \
	dep/libusbp/src/linux/generic_interface_linux.c \
	dep/libusbp/src/linux/list_linux.c \
	dep/libusbp/src/linux/serial_port_linux.c \
	dep/libusbp/src/linux/udev_linux.c \
	dep/libusbp/src/linux/usbfd_linux.c \
	dep/libusbp/src/list.c \
	dep/libusbp/src/pipe_id.c \
	dep/libusbp/src/string.c \
	
STB_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(STB_SRCS))
$(STB_OBJS): CFLAGS += -Idep/libusbp/src
STB_LIB = $(OBJDIR)/libubsp.a
$(STB_LIB): $(STB_OBJS)
STB_CFLAGS =
STB_LDFLAGS = $(STB_LIB)

endif

