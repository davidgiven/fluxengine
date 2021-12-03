cmake_minimum_required(VERSION 3.1)

project(snowhouse)

option(SNOWHOUSE_BUILD_TESTS    "Build the Snowhouse tests"                 OFF)
option(SNOWHOUSE_RUN_TESTS      "Run the Snowhouse tests"                   OFF)
set(SNOWHOUSE_CXX_STANDARD "C++11" CACHE STRING "The C++ standard the examples are compiled with")
set_property(CACHE SNOWHOUSE_CXX_STANDARD PROPERTY STRINGS "C++11" "C++14" "C++17")

add_library(snowhouse INTERFACE)
target_include_directories(snowhouse INTERFACE include)

if(SNOWHOUSE_CXX_STANDARD STREQUAL "C++11")
  set(CMAKE_CXX_STANDARD 11)
elseif(SNOWHOUSE_CXX_STANDARD STREQUAL "C++14")
  set(CMAKE_CXX_STANDARD 14)
elseif(SNOWHOUSE_CXX_STANDARD STREQUAL "C++17")
  set(CMAKE_CXX_STANDARD 17)
else()
  message(WARNING "C++ standard \"${SNOWHOUSE_CXX_STANDARD}\" not known, falling back to default")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./bin)

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /MP ")
else()
  # Assume GCC-style arguments
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Wall -Wextra -pedantic -Wdeprecated -Wdeprecated-declarations -Wnon-virtual-dtor \
    -Wshadow -Wfloat-equal -Wundef -Wendif-labels -Wno-error=unknown-pragmas")
endif()

message(STATUS "C++ compiler flags: ${CMAKE_CXX_FLAGS}")

if (SNOWHOUSE_BUILD_TESTS)
    FILE(GLOB SnowhouseSpecSourceFiles example/*.cpp)
    add_executable(snowhouse-tests ${SnowhouseSpecSourceFiles})
    target_link_libraries(snowhouse-tests PRIVATE snowhouse)
endif()

if (SNOWHOUSE_BUILD_TESTS AND SNOWHOUSE_RUN_TESTS)
    add_custom_command(TARGET snowhouse-tests
                       POST_BUILD
                       COMMAND snowhouse-tests
                       WORKING_DIRECTORY ./bin)
elseif (SNOWHOUSE_RUN_TESTS)
    message(WARNING "Unable to run snowhouse tests - set:\n  option(SNOWHOUSE_BUILD_TESTS, \"Build the Snowhouse tests\" ON)\nand clear your CMakeCache.txt")
endif()
