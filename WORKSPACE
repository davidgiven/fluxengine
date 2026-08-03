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
# Notes
# - Omitting sha256 values reduces hermeticity. Replace these http_archive blocks
#   with a version that includes sha256 values when you have the hashes.
# - If you later want to compute checksums locally, see previous commit messages or
#   run a local script to download and compute the shasums and paste them into WORKSPACE.
# - After configuring the workspace properly, run `bazel fetch //...` then `bazel build`.
# -----------------------------------------------------------------------------
