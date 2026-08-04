def _native_image_impl(ctx):
    is_windows = ctx.configuration.host_path_separator == ";"
    
    # 1. Ensure out_name ends with .exe on Windows (without duplicating it)
    base_name = ctx.label.name
    if is_windows and not base_name.lower().endswith(".exe"):
        out_name = base_name + ".exe"
    else:
        out_name = base_name

    out_binary = ctx.actions.declare_file(out_name)

    # 2. Strip .exe for -H:Name on Windows because native-image auto-appends .exe on Windows
    h_name_path = out_binary.path
    if is_windows and h_name_path.lower().endswith(".exe"):
        h_name_path = h_name_path[:-4]

    jar_file = ctx.file.jar

    args = ctx.actions.args()
    args.add("-jar", jar_file.path)
    args.add("-H:Name=" + h_name_path)

    for extra_arg in ctx.attr.extra_args:
        args.add(extra_arg)

    ctx.actions.run(
        outputs = [out_binary],
        inputs = [jar_file],
        executable = ctx.executable._native_image_tool,
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
        "_native_image_tool": attr.label(
            default = Label("@graalvm//:native_image_tool"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
