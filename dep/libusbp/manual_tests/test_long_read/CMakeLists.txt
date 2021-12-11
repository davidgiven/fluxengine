use_cxx11()

add_executable(test_long_read test_long_read.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(test_long_read usbp)
