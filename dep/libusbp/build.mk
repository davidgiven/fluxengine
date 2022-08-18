LIBUSBP_SRCS = \
	dep/libusbp/src/async_in_pipe.c \
	dep/libusbp/src/error.c \
	dep/libusbp/src/error_hresult.c \
	dep/libusbp/src/find_device.c \
	dep/libusbp/src/list.c \
	dep/libusbp/src/pipe_id.c \
	dep/libusbp/src/string.c \
	
ifeq ($(OS), Windows_NT)

LIBUSBP_LDFLAGS += -lsetupapi -lwinusb -lole32 -luuid
LIBUSBP_SRCS += \
	dep/libusbp/src/windows/async_in_transfer_windows.c \
	dep/libusbp/src/windows/device_instance_id_windows.c \
	dep/libusbp/src/windows/device_windows.c \
	dep/libusbp/src/windows/error_windows.c \
	dep/libusbp/src/windows/generic_handle_windows.c \
	dep/libusbp/src/windows/generic_interface_windows.c \
	dep/libusbp/src/windows/interface_windows.c \
	dep/libusbp/src/windows/list_windows.c \
	dep/libusbp/src/windows/serial_port_windows.c \

else ifeq ($(shell uname),Darwin)

LIBUSBP_SRCS += \
	dep/libusbp/src/mac/async_in_transfer_mac.c \
	dep/libusbp/src/mac/device_mac.c \
	dep/libusbp/src/mac/error_mac.c \
	dep/libusbp/src/mac/generic_handle_mac.c \
	dep/libusbp/src/mac/generic_interface_mac.c \
	dep/libusbp/src/mac/iokit_mac.c \
	dep/libusbp/src/mac/list_mac.c \
	dep/libusbp/src/mac/serial_port_mac.c \

else

LIBUSBP_CFLAGS += $(shell pkg-config --cflags libudev)
LIBUSBP_LDFLAGS += $(shell pkg-config --libs libudev)
LIBUSBP_SRCS += \
	dep/libusbp/src/linux/async_in_transfer_linux.c \
	dep/libusbp/src/linux/device_linux.c \
	dep/libusbp/src/linux/error_linux.c \
	dep/libusbp/src/linux/generic_handle_linux.c \
	dep/libusbp/src/linux/generic_interface_linux.c \
	dep/libusbp/src/linux/list_linux.c \
	dep/libusbp/src/linux/serial_port_linux.c \
	dep/libusbp/src/linux/udev_linux.c \
	dep/libusbp/src/linux/usbfd_linux.c \

endif

LIBUSBP_OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(LIBUSBP_SRCS))
$(LIBUSBP_OBJS): CFLAGS += -Idep/libusbp/src -Idep/libusbp/include
LIBUSBP_LIB = $(OBJDIR)/libusbp.a
LIBUSBP_CFLAGS += -Idep/libusbp/include
LIBUSBP_LDFLAGS += $(LIBUSBP_LIB)
$(LIBUSBP_LIB): $(LIBUSBP_OBJS)


