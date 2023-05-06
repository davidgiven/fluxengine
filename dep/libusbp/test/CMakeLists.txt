INCLUDE (CheckIncludeFileCXX)

# If catch.hpp is not present, we want to simply skip compiling the tests.
# Download catch.hpp and put it in this directory:
# https://raw.githubusercontent.com/catchorg/Catch2/v2.x/single_include/catch2/catch.hpp
set (CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")
CHECK_INCLUDE_FILE_CXX (catch.hpp HAVE_CATCH_FRAMEWORK)
if (NOT HAVE_CATCH_FRAMEWORK)
  message (STATUS "The test suite will not be built.")
  return ()
endif ()

set(USE_TEST_DEVICE_A FALSE CACHE BOOL
  "Run tests that require Test Device A.")

set(USE_TEST_DEVICE_B FALSE CACHE BOOL
  "Run tests that require Test Device B.")

file(GLOB test_sources *.cpp)

if (APPLE)
  set (link_flags "-framework CoreFoundation ${link_flags}")
endif ()

add_executable(run_test ${test_sources})

set_target_properties(run_test PROPERTIES
  LINK_FLAGS "${link_flags}"
)

include_directories (
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/src"
)

target_link_libraries(run_test usbp)
