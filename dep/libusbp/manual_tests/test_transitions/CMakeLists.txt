use_cxx11()

add_executable(test_transitions test_transitions.cpp)

include_directories (
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(test_transitions usbp)