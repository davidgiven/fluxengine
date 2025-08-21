from build.c import cxxlibrary
from build.pkg import package

package(name="libglfw", package="glfw3")
package(name="libopengl", package="opengl")

cxxlibrary(
    name="imgui",
    srcs=[
        "./backends/imgui_impl_glfw.cpp",
        "./backends/imgui_impl_glfw.h",
        "./backends/imgui_impl_opengl3.cpp",
        "./backends/imgui_impl_opengl3.h",
        "./backends/imgui_impl_opengl3_loader.h",
        "./imgui.cpp",
        "./imgui_demo.cpp",
        "./imgui_draw.cpp",
        "./imgui_internal.h",
        "./imgui_tables.cpp",
        "./imgui_widgets.cpp",
        "./imstb_rectpack.h",
        "./imstb_textedit.h",
        "./imstb_truetype.h",
    ],
    hdrs={
        "imconfig.h": "./imconfig.h",
        "imgui.h": "./imgui.h",
        "imgui_impl_glfw.h": "./backends/imgui_impl_glfw.h",
        "imgui_impl_opengl3.h": "./backends/imgui_impl_opengl3.h",
    },
    deps=[".+libglfw", ".+libopengl"],
)
