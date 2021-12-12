use_cxx11()

add_executable(port_name port_name.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(port_name usbp)
