LIBUSBP_SRCS = \
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
	
LIBUSBP_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(LIBUSBP_SRCS))
$(LIBUSBP_OBJS): CFLAGS += -Idep/libusbp/src -Idep/libusbp/include
LIBUSBP_LIB = $(OBJDIR)/libusbp.a
LIBUSBP_CFLAGS = -Idep/libusbp/include $(shell pkg-config --cflags libudev)
LIBUSBP_LDFLAGS = $(LIBUSBP_LIB) $(shell pkg-config --libs libudev)
$(LIBUSBP_LIB): $(LIBUSBP_OBJS)


