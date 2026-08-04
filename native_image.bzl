def _native_image_impl(ctx):
    is_windows = ctx.configuration.host_path_separator == ";"
    
    # 1. Determine output binary file name
    out_name = ctx.label.name + (".exe" if is_windows else "")
    out_binary = ctx.actions.declare_file(out_name)

    # 2. Select the correct tool executable depending on host OS
    tool_file = ctx.file._native_image_win if is_windows else ctx.file._native_image_unix

    jar_file = ctx.file.jar

    args = ctx.actions.args()
    args.add("-jar", jar_file.path)
    args.add("-H:Name=" + out_binary.path)

    for extra_arg in ctx.attr.extra_args:
        args.add(extra_arg)

    ctx.actions.run(
        outputs = [out_binary],
        inputs = [jar_file],
        executable = tool_file,
        arguments = [args],
        mnemonic = "GraalVMNativeImage",
        progress_message = "Building GraalVM native image %s" % ctx.label.name,
        use_default_shell_env = True,
    )

    return [DefaultInfo(executable = out_binary)]

native_image = rule(
    implementation = _native_image_impl,
    executable = True,
    attrs = {
        "jar": attr.label(
            mandatory = True,
            allow_single_file = [".jar"],
        ),
        "extra_args": attr.string_list(default = []),
        "_native_image_unix": attr.label(
            default = Label("@graalvm//:bin/native-image"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
        "_native_image_win": attr.label(
            default = Label("@graalvm//:bin/native-image.cmd"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
