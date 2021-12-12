use_cxx11()

add_executable(test_async_in test_async_in.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(test_async_in usbp)
