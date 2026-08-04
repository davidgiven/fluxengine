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

