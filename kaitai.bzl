load("@rules_java//java:defs.bzl", "JavaInfo", "java_common")

def _kaitai_java_library_impl(ctx):
    out_dir = ctx.actions.declare_directory(ctx.label.name + "_srcs")
    srcjar = ctx.actions.declare_file(ctx.label.name + ".srcjar")
    output_jar = ctx.actions.declare_file(ctx.label.name + ".jar")

    # 1. Action: Run Kaitai Struct Compiler to output into a directory
    args = ctx.actions.args()
    args.add("-t", "java")
    args.add("--outdir", out_dir.path)
    args.add("--java-package", ctx.attr.java_package)
    args.add("--read-write")
    args.add("--no-auto-read")
    args.add_all([f.path for f in ctx.files.srcs])

    ctx.actions.run(
        outputs = [out_dir],
        inputs = ctx.files.srcs,
        executable = ctx.executable._ksc,
        arguments = [args],
        progress_message = "Compiling KSY files to Java: %s" % ctx.label.name,
    )

    # 2. Action: Zip generated Java sources into a .srcjar
    # Using Bazel's built-in singlejar tool or jar utility
    ctx.actions.run_shell(
        inputs = [out_dir],
        outputs = [srcjar],
        command = "jar cf {srcjar} -C {dir} .".format(
            srcjar = srcjar.path,
            dir = out_dir.path,
        ),
        progress_message = "Packaging generated Java sources for %s" % ctx.label.name,
    )

    # 3. Action: Compile the .srcjar using java_common.compile
    deps_java_info = [dep[JavaInfo] for dep in ctx.attr.deps]
    java_provider = java_common.compile(
        ctx,
        source_jars = [srcjar],
        deps = deps_java_info,
        output = output_jar,
        java_toolchain = ctx.attr._java_toolchain[java_common.JavaToolchainInfo],
    )

    return [
        java_provider,
        DefaultInfo(files = depset([output_jar])),
    ]

kaitai_java_library = rule(
    implementation = _kaitai_java_library_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = [".ksy"],
            mandatory = True,
        ),
        "java_package": attr.string(
            mandatory = True,
        ),
        "deps": attr.label_list(
            providers = [JavaInfo],
        ),
        "_ksc": attr.label(
            default = Label("//:ksc_binary"),
            executable = True,
            cfg = "exec",
        ),
        "_java_toolchain": attr.label(
            default = Label("@bazel_tools//tools/jdk:current_java_toolchain"),
        ),
    },
    toolchains = ["@bazel_tools//tools/jdk:toolchain_type"],
    fragments = ["java"],
)
