use_c99()

add_library (install_helper SHARED install_helper_windows.c dll.def)

target_link_libraries (install_helper setupapi msi)

set_target_properties (install_helper PROPERTIES
  OUTPUT_NAME usbp-install-helper-${LIBUSBP_VERSION_MAJOR}
  LINK_FLAGS "-Wl,--enable-stdcall-fixup -static"
)

install (TARGETS install_helper
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
