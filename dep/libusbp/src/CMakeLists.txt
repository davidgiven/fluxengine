use_c99()

# Settings for GCC
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # By default, symbols are not visible outside of the library.
  set (CMAKE_C_FLAGS "-fvisibility=hidden ${CMAKE_C_FLAGS}")

  # Avoid tons of errors from strsafe.h.
  set (CMAKE_C_FLAGS "-fgnu89-inline ${CMAKE_C_FLAGS}")
endif ()

# Define cross-platform source files.
set (sources
  async_in_pipe.c
  error.c
  error_hresult.c
  find_device.c
  list.c
  pipe_id.c
  string.c)

# Define operating system-specific source files.
if (WIN32)
  set (sources ${sources}
    windows/error_windows.c
    windows/device_windows.c
    windows/interface_windows.c
    windows/device_instance_id_windows.c
    windows/generic_interface_windows.c
    windows/list_windows.c
    windows/generic_handle_windows.c
    windows/async_in_transfer_windows.c
    windows/serial_port_windows.c
    ${CMAKE_CURRENT_BINARY_DIR}/info.rc)
elseif (LINUX)
  set (sources ${sources}
    linux/list_linux.c
    linux/device_linux.c
    linux/generic_interface_linux.c
    linux/generic_handle_linux.c
    linux/error_linux.c
    linux/udev_linux.c
    linux/usbfd_linux.c
    linux/async_in_transfer_linux.c
    linux/serial_port_linux.c)
elseif (APPLE)
  set (sources ${sources}
    mac/list_mac.c
    mac/device_mac.c
    mac/error_mac.c
    mac/generic_interface_mac.c
    mac/generic_handle_mac.c
    mac/async_in_transfer_mac.c
    mac/serial_port_mac.c
    mac/iokit_mac.c)
endif ()

add_library (usbp ${sources})

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
)

if (WIN32)
  target_link_libraries (usbp setupapi winusb uuid ole32)
  if (NOT BUILD_SHARED_LIBS)
    set (PC_MORE_LIBS "-lsetupapi -lwinusb -luuid -lole32")
  endif ()
elseif (LINUX)
  pkg_check_modules(LIBUDEV REQUIRED libudev)
  string (REPLACE ";" " " LIBUDEV_CFLAGS "${LIBUDEV_CFLAGS}")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBUDEV_CFLAGS}")
  target_link_libraries (usbp udev)
  if (NOT BUILD_SHARED_LIBS)
    set (PC_REQUIRES "libudev")
  endif ()
elseif (APPLE)
  set (link_flags "-framework IOKit -framework CoreFoundation ${link_flags}")
  if (NOT BUILD_SHARED_LIBS)
    set (PC_MORE_LIBS "${link_flags}")
  endif ()
endif ()

set_target_properties(usbp PROPERTIES
  OUTPUT_NAME usbp-${LIBUSBP_VERSION_MAJOR}
  SOVERSION ${LIBUSBP_VERSION}
  VERSION ${LIBUSBP_VERSION}
  DEFINE_SYMBOL LIBUSBP_EXPORTS
  LINK_FLAGS "${link_flags}"
)

configure_file (
  "libusbp_config.h.in"
  "libusbp_config.h"
)

configure_file (
  "info.rc.in"
  "info.rc"
)

configure_file (
  "libusbp.pc.in"
  "libusbp-${LIBUSBP_VERSION_MAJOR}.pc"
  @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libusbp-${LIBUSBP_VERSION_MAJOR}.pc"
  DESTINATION lib/pkgconfig)

install(TARGETS usbp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
