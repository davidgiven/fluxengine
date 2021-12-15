use_cxx11()

add_executable(async_in async_in.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(async_in usbp)