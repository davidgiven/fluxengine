use_cxx11()

add_executable(test_long_write test_long_write.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(test_long_write usbp)
