# Minimal WORKSPACE for Bazel Java/Kotlin/Protobuf builds for incremental migration.
# This file references specific release tarballs for Bazel rule sets without sha256 checksums.
# NOTE: Omitting sha256 makes fetches non-hermetic and is NOT recommended for long-term use.
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
# We fetch the rules_graalvm repository so we can use the `native_image` rule and
# toolchain helpers. This is pinned to a specific commit to keep the workspace
# reproducible; replace the commit with a release tag if you prefer.
# -----------------------------------------------------------------------------
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "rules_graalvm",
    remote = "https://github.com/sgammon/rules_graalvm.git",
    commit = "35d47821696b4d96aa579b184338fb5e1ea20885",
)

# NOTE: The rules_graalvm repository provides helper repository rules (graalvm_repository
# or graal_bindist_repository) to declare an SDK repository (commonly named @graalvm).
# You must declare a GraalVM SDK repository appropriate for your host platform and pin
# it to a specific GraalVM CE build in order for `native_image` targets to be fully
# functional. See the rules_graalvm docs for examples:
# https://github.com/sgammon/rules_graalvm/blob/main/docs/native-image.md

# Example (commented) — replace URL and sha256 with the GraalVM build you want to pin:
# load("@rules_graalvm//graalvm:repositories.bzl", "graalvm_bindist_repository")
# graalvm_bindist_repository(
#     name = "graalvm",
#     version = "22.3.0",
#     url_template = "https://github.com/graalvm/graalvm-ce-builds/releases/download/v{version}/graalvm-ce-java17-linux-amd64-{version}.tar.gz",
#     sha256 = "<sha256-for-the-tarball>",
# )

# After declaring a @graalvm SDK repository, register the GraalVM toolchains so
# the `native_image` rule resolves the native-image tool for the host/exec
# platform automatically:
# load("@rules_graalvm//graalvm:toolchain.bzl", "register_graalvm_toolchains")
# register_graalvm_toolchains(name = "@graalvm")

# -----------------------------------------------------------------------------
# Notes
# - Omitting sha256 values reduces hermeticity. Replace these http_archive/git_repository
#   blocks with versions that include sha256 values when you have the hashes.
# - After configuring the workspace properly (including a @graalvm SDK repo), run
#   `bazel fetch //...` then `bazel build` to populate external dependencies.
# -----------------------------------------------------------------------------
