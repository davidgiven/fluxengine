workspace(name = "fluxengine_java")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


# -----------------------------------------------------------------------------
# rules_proto (Protobuf support)
# Release: 4.0.0
# Download URL (browser):
# https://github.com/bazelbuild/rules_proto/releases/download/4.0.0/rules_proto-4.0.0.tar.gz
# -----------------------------------------------------------------------------
http_archive(
    name = "rules_proto",
    url = "https://github.com/bazelbuild/rules_proto/releases/download/4.0.0/rules_proto-4.0.0.tar.gz",
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies")

# Register proto toolchain if desired by uncommenting the following line:
# rules_proto_dependencies()

# -----------------------------------------------------------------------------
# rules_graalvm (GraalVM native-image integration)
# Use the official release archive and register the SDK via graalvm_repository
# as requested. Follow the rules_graalvm README for details.
# -----------------------------------------------------------------------------
http_archive(
    name = "rules_graalvm",
    sha256 = "3ef2f1583a4849d03209a43b0b507f172299c3045e585b6ffa7144a2bc12ae18",
    strip_prefix = "rules_graalvm-0.12.0",
    urls = [
        "https://github.com/sgammon/rules_graalvm/releases/download/v0.12.0/rules_graalvm-0.12.0.zip",
    ],
)

load("@rules_graalvm//graalvm:repositories.bzl", "graalvm_repository")

graalvm_repository(
    name = "graalvm",
    distribution = "ce",
    java_version = "23",
    version = "23.0.0",
)

load(
    "@rules_graalvm//graalvm:workspace.bzl",
    "register_graalvm_toolchains",
    "rules_graalvm_repositories",
)

rules_graalvm_repositories()

register_graalvm_toolchains()

# -----------------------------------------------------------------------------
# Notes
# - Omitting sha256 values reduces hermeticity. Replace these http_archive
#   blocks with versions that include sha256 values when you have the hashes.
# - After configuring the workspace properly (including a @graalvm SDK repo), run
#   `bazel fetch //...` then `bazel build` to populate external dependencies.
# -----------------------------------------------------------------------------
