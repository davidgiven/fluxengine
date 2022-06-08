libusbp_includes = include_directories('include')

if target_machine.system() == 'linux' 
  system_sources = [
    'src/linux/async_in_transfer_linux.c',
    'src/linux/device_linux.c',
    'src/linux/error_linux.c',
    'src/linux/generic_handle_linux.c',
    'src/linux/generic_interface_linux.c',
    'src/linux/list_linux.c',
    'src/linux/serial_port_linux.c',
    'src/linux/udev_linux.c',
    'src/linux/usbfd_linux.c',
  ]

  system_deps = [
    dependency('libudev')
  ]
elif target_machine.system() == 'windows'
  system_sources = [
    'src/windows/async_in_transfer_windows.c',
    'src/windows/device_instance_id_windows.c',
    'src/windows/device_windows.c',
    'src/windows/error_windows.c',
    'src/windows/generic_handle_windows.c',
    'src/windows/generic_interface_windows.c',
    'src/windows/interface_windows.c',
    'src/windows/list_windows.c',
    'src/windows/serial_port_windows.c',
  ]

  system_deps = [
    declare_dependency(
      link_args: [
        '-lsetupapi',
        '-lwinusb',
        '-lole32',
        '-luuid'
      ]
    )
  ]
endif

libusbp_dep = declare_dependency(
    include_directories: [
      libusbp_includes,
    ],
    dependencies: [
      system_deps
    ],
    link_with: [
      static_library(
        'usbp',
        sources: [
          system_sources,
          'src/async_in_pipe.c',
          'src/error.c',
          'src/error_hresult.c',
          'src/find_device.c',
          'src/list.c',
          #'src/mac/async_in_transfer_mac.c',
          #'src/mac/device_mac.c',
          #'src/mac/error_mac.c',
          #'src/mac/generic_handle_mac.c',
          #'src/mac/generic_interface_mac.c',
          #'src/mac/iokit_mac.c',
          #'src/mac/list_mac.c',
          #'src/mac/serial_port_mac.c',
          'src/pipe_id.c',
          'src/string.c',
        ],
        include_directories: [
          libusbp_includes,
          'src',
        ]
      )
    ]
)


