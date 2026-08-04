_GRAALVM_URLS = {
    "linux_x86_64": {
        "url": "https://download.oracle.com/graalvm/21/archive/graalvm-jdk-21.0.2_linux-x64_bin.tar.gz",
        "strip_prefix": "graalvm-jdk-21.0.2+13.1",
    },
    "linux_aarch64": {
        "url": "https://download.oracle.com/graalvm/21/archive/graalvm-jdk-21.0.2_linux-aarch64_bin.tar.gz",
        "strip_prefix": "graalvm-jdk-21.0.2+13.1",
    },
    "macos_x86_64": {
        "url": "https://download.oracle.com/graalvm/21/archive/graalvm-jdk-21.0.2_macos-x64_bin.tar.gz",
        "strip_prefix": "graalvm-jdk-21.0.2+13.1/Contents/Home",
    },
    "macos_aarch64": {
        "url": "https://download.oracle.com/graalvm/21/archive/graalvm-jdk-21.0.2_macos-aarch64_bin.tar.gz",
        "strip_prefix": "graalvm-jdk-21.0.2+13.1/Contents/Home",
    },
    "windows_x86_64": {
        "url": "https://download.oracle.com/graalvm/21/archive/graalvm-jdk-21.0.2_windows-x64_bin.zip",
        "strip_prefix": "graalvm-jdk-21.0.2+13.1",
    },
}

def _graalvm_repository_impl(ctx):
    os_name = ctx.os.name.lower()
    arch = ctx.os.arch.lower()

    # Normalize OS name
    if "mac" in os_name or "darwin" in os_name:
        os_key = "macos"
    elif "win" in os_name:
        os_key = "windows"
    else:
        os_key = "linux"

    # Normalize architecture
    if arch in ["aarch64", "arm64"]:
        arch_key = "aarch64"
    else:
        arch_key = "x86_64"

    key = "%s_%s" % (os_key, arch_key)
    if key not in _GRAALVM_URLS:
        fail("Unsupported platform for GraalVM: %s" % key)

    info = _GRAALVM_URLS[key]

    # Download and extract the platform archive
    ctx.download_and_extract(
        url = info["url"],
        stripPrefix = info["strip_prefix"],
    )

    # Expose binary files and executables to Bazel
    ctx.file(
        "BUILD.bazel",
        """
package(default_visibility = ["//visibility:public"])
exports_files(glob(["**/*"]))

filegroup(
    name = "java_home",
    srcs = glob(["**/*"]),
)
""",
    )

graalvm_repository = repository_rule(
    implementation = _graalvm_repository_impl,
)
