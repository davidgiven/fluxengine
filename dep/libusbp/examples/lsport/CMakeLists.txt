use_cxx11()

add_executable(lsport lsport.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(lsport usbp)
