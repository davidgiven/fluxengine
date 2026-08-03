workspace(name = "fluxengine_java")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# -----------------------------------------------------------------------------
# rules_jvm_external (manage Maven artifacts like Dagger, JUnit)
# Release: 7.1
# Download URL (browser):
# https://github.com/bazelbuild/rules_jvm_external/releases/download/7.1/rules_jvm_external-7.1.tar.gz
# -----------------------------------------------------------------------------
http_archive(
    name = "rules_jvm_external",
    url = "https://github.com/bazelbuild/rules_jvm_external/releases/download/7.1/rules_jvm_external-7.1.tar.gz",
)

load("@rules_jvm_external//:defs.bzl", "maven_install")

# Configure Maven dependencies that we'll use during incremental migration.
maven_install(
    name = "maven",
    artifacts = [
        "junit:junit:4.13.2",
        "com.google.dagger:dagger:2.44",
        "com.google.dagger:dagger-compiler:2.44",
        # Add more artifacts here as needed.
    ],
    repositories = [
        "https://repo1.maven.org/maven2",
    ],
)

# Notes on maven_install artifact labels
# - com.google.dagger:dagger -> @maven//:com_google_dagger_dagger
# - com.google.dagger:dagger-compiler -> @maven//:com_google_dagger_dagger_compiler
# - junit:junit -> @maven//:junit_junit

# -----------------------------------------------------------------------------
# rules_kotlin (Kotlin support for Bazel)
# Release: v4.0.0
# Download URL (browser):
# https://github.com/bazelbuild/rules_kotlin/releases/download/v4.0.0/rules_kotlin-v4.0.0.tar.gz
# -----------------------------------------------------------------------------
http_archive(
    name = "io_bazel_rules_kotlin",
    url = "https://github.com/bazelbuild/rules_kotlin/releases/download/v4.0.0/rules_kotlin-v4.0.0.tar.gz",
)

load("@io_bazel_rules_kotlin//kotlin:kotlin.bzl", "kotlin_repositories")

# Registers Kotlin toolchain. Call this to register kotlin toolchain and enable KAPT support.
# Note: rules_kotlin must be fetched successfully by bazel before this call will work.
kotlin_repositories()

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
# Use the official release archive rather than a git_repository so Bazel exposes
# the @rules_graalvm repository correctly during repository evaluation.
# See https://github.com/sgammon/rules_graalvm README for details.
# -----------------------------------------------------------------------------
http_archive(
    name = "rules_graalvm",
    urls = [
        "https://github.com/sgammon/rules_graalvm/releases/download/v0.12.0/rules_graalvm-0.12.0.zip",
    ],
    strip_prefix = "rules_graalvm-0.12.0",
    sha256 = "",  # TODO: add the release sha256 for hermeticity
)

# Load helper repository functions from rules_graalvm and register the
# repository-level dependencies. This makes @rules_graalvm visible to the
# main workspace and avoids the "unknown repo 'rules_graalvm'" error.
load("@rules_graalvm//graalvm:workspace.bzl", "rules_graalvm_repositories")

# Register the rules' own external repositories (examples, toolchain helpers, etc.).
rules_graalvm_repositories()

# NOTE: To use GraalVM as an SDK/toolchain you must declare a @graalvm SDK
# repository and register its toolchains. Example (uncomment and adjust):
# load("@rules_graalvm//graalvm:repositories.bzl", "graalvm_bindist_repository")
# graalvm_bindist_repository(
#     name = "graalvm",
#     version = "23.0.0",
#     url_template = "https://github.com/graalvm/graalvm-ce-builds/releases/download/v{version}/graalvm-ce-java17-linux-amd64-{version}.tar.gz",
#     sha256 = "<sha256-for-the-tarball>",
# )
#
# load("@rules_graalvm//graalvm:workspace.bzl", "register_graalvm_toolchains")
# register_graalvm_toolchains(name = "@graalvm")

# -----------------------------------------------------------------------------
# Notes
# - Omitting sha256 values reduces hermeticity. Replace these http_archive
#   blocks with versions that include sha256 values when you have the hashes.
# - After configuring the workspace properly (including a @graalvm SDK repo), run
#   `bazel fetch //...` then `bazel build` to populate external dependencies.
# -----------------------------------------------------------------------------
