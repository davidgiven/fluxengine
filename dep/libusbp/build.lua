local sys_deps = {}
local sys_srcs = {}

if vars.PLATFORM == "WINDOWS" then
	sys_srcs = {
		"./src/windows/async_in_transfer_windows.c",
		"./src/windows/device_instance_id_windows.c",
		"./src/windows/device_windows.c",
		"./src/windows/error_windows.c",
		"./src/windows/generic_handle_windows.c",
		"./src/windows/generic_interface_windows.c",
		"./src/windows/interface_windows.c",
		"./src/windows/list_windows.c",
		"./src/windows/serial_port_windows.c",
	}
elseif vars.PLATFORM == "OSX" then
	sys_srcs = {
		"./src/mac/async_in_transfer_mac.c",
		"./src/mac/device_mac.c",
		"./src/mac/error_mac.c",
		"./src/mac/generic_handle_mac.c",
		"./src/mac/generic_interface_mac.c",
		"./src/mac/iokit_mac.c",
		"./src/mac/list_mac.c",
		"./src/mac/serial_port_mac.c",
	}
else
	dependency {
		name = "libudev_dep",
		pkg_config = "libudev"
	}

	sys_deps = { "+libudev_dep" }
	sys_srcs = {
		"./src/linux/async_in_transfer_linux.c",
		"./src/linux/device_linux.c",
		"./src/linux/error_linux.c",
		"./src/linux/generic_handle_linux.c",
		"./src/linux/generic_interface_linux.c",
		"./src/linux/list_linux.c",
		"./src/linux/serial_port_linux.c",
		"./src/linux/udev_linux.c",
		"./src/linux/usbfd_linux.c",
	}
end

clibrary {
	name = "libusbp",
	srcs = {
		"./src/async_in_pipe.c",
		"./src/error.c",
		"./src/error_hresult.c",
		"./src/find_device.c",
		"./src/list.c",
		"./src/pipe_id.c",
		"./src/string.c",
		sys_srcs,
	},
	hdrs = {
		["libusbp_config.h"] = "./include/libusbp_config.h",
		["libusbp.h"] = "./include/libusbp.h",
		["libusbp.hpp"] = "./include/libusbp.hpp",
	},
	deps = sys_deps,
	vars = {
		["+cflags"] = {
			"-I"..cwd().."/include",
			"-I"..cwd().."/src",
		}
	}
}

