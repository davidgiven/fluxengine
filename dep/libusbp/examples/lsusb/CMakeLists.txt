use_cxx11()

add_executable(lsusb lsusb.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(lsusb usbp)