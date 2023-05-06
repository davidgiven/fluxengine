cmake_minimum_required (VERSION 3.10.0)

# Fix behavior of CMAKE_CXX_STANDARD when targeting macOS.
if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif ()

# Fix a warning on macOS.
if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif ()

# Don't use -rdynamic since it breaks causes musl static linking.
if (POLICY CMP0065)
  cmake_policy(SET CMP0065 NEW)
endif ()

project (libusbp)

set (LIBUSBP_VERSION_MAJOR 1)
set (LIBUSBP_VERSION_MINOR 3)
set (LIBUSBP_VERSION_PATCH 0)

# Make 'Release' be the default build type, since the debug builds
# include exported symbols that might cause name conflicts.
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Options are Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif ()

option (BUILD_SHARED_LIBS "Build as shared library" TRUE)
if (NOT BUILD_SHARED_LIBS)
  add_definitions (-DLIBUSBP_STATIC)
  set (PC_MORE_CFLAGS "-DLIBUSBP_STATIC")
endif ()

set(ENABLE_EXAMPLES FALSE CACHE BOOL
  "True if you want to build the examples.")

set(ENABLE_TESTS FALSE CACHE BOOL
  "True if you want to build the tests.")

set(LIBUSBP_LOG FALSE CACHE BOOL
  "Output log messages to stderr for debugging.")

set(VBOX_LINUX_ON_WINDOWS FALSE CACHE BOOL
  "Skip tests known to cause problems on a Linux VirtualBox guest on Windows.")

set(ENABLE_GCOV FALSE CACHE BOOL
  "Compile with special options needed for gcov.")

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

set (LIBUSBP_VERSION ${LIBUSBP_VERSION_MAJOR}.${LIBUSBP_VERSION_MINOR}.${LIBUSBP_VERSION_PATCH})

if (CMAKE_VERSION VERSION_GREATER "2.8.10")
  string(TIMESTAMP YEAR "%Y")
endif ()

find_package(PkgConfig)

# Put libraries and executables in the top level of the build directory
# so that the executables can find the libraries and it is easy to run
# everything.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Warn about everything.
set (CMAKE_C_FLAGS "-Wall -Wextra -pedantic ${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")

if (ENABLE_GCOV)
  set (CMAKE_C_FLAGS "-fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}")
endif ()

if (WIN32)

  # Enable correct behavior for the return value of vsnprintf.
  add_definitions (-D__USE_MINGW_ANSI_STDIO=1)

  # Enable functions only available in Windows Vista and later,
  # such as StringCompareEx.
  add_definitions (-D_WIN32_WINNT=0x0600 -DNTDDI_VERSION=0x06000000)

endif ()

# Detect Linux.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set (LINUX 1)
endif ()

# Install the header files into include/
install(FILES include/libusbp.h include/libusbp.hpp
  DESTINATION "include/libusbp-${LIBUSBP_VERSION_MAJOR}")

add_subdirectory (src)

if (ENABLE_TESTS)
  add_subdirectory (test)
  add_subdirectory (manual_tests)
endif ()

if (ENABLE_EXAMPLES)
  add_subdirectory (examples)
endif ()

if (WIN32)
  add_subdirectory (install_helper)
endif ()
