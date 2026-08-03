# Minimal WORKSPACE for Bazel Java/Kotlin/Protobuf builds for incremental migration.
# NOTE: Fill in the sha256 values for the http_archive rules before running bazel.
workspace(name = "fluxengine_java")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# rules_jvm_external: manage Maven artifacts (JUnit, Dagger, etc.)
# See https://github.com/bazelbuild/rules_jvm_external for latest versions and sha256.
http_archive(
    name = "rules_jvm_external",
    url = "https://github.com/bazelbuild/rules_jvm_external/releases/download/5.5/rules_jvm_external-5.5.tar.gz",
    sha256 = "<rules_jvm_external_sha256>",
)

load("@rules_jvm_external//:defs.bzl", "maven_install")
# Configure Maven dependencies that we'll use during incremental migration.
# Added dagger-compiler so annotation processing can run (when using java_rules that support
# annotation processors or when using aapt-like tools). You may also need to add
# annotation processor configuration to java_library / java_test rules depending on your setup.
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

# rules_kotlin: Kotlin compilation support
# See https://github.com/bazelbuild/rules_kotlin for details.
http_archive(
    name = "io_bazel_rules_kotlin",
    url = "https://github.com/bazelbuild/rules_kotlin/releases/download/v4.0.0/rules_kotlin-v4.0.0.tar.gz",
    sha256 = "<rules_kotlin_sha256>",
)
load("@io_bazel_rules_kotlin//kotlin:kotlin.bzl", "kotlin_repositories")
# Registers Kotlin toolchain. Uncomment the following line after filling sha256 above.
# Call it now to enable Kotlin toolchain registration (ensure the sha256 is valid).
kotlin_repositories()

# rules_proto: Protobuf support and Java protobuf generation
# See https://github.com/bazelbuild/rules_proto and https://github.com/bazelbuild/rules_protobuf
http_archive(
    name = "rules_proto",
    url = "https://github.com/bazelbuild/rules_proto/releases/download/4.0.0/rules_proto-4.0.0.tar.gz",
    sha256 = "<rules_proto_sha256>",
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies")
# Register proto toolchain (ensure sha256 above is filled first).
rules_proto_dependencies()

# If you want direct protobuf compiler binaries, add them here (or let rules_proto bring them in).

# Workspace notes
# - Fill in the sha256 values above before running bazel.
# - After filling sha256, run `bazel fetch //...` to download toolchains and artifacts.
