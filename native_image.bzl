def _native_image_impl(ctx):
    # Determine binary extension based on OS
    is_windows = ctx.configuration.host_path_separator == ";"
    out_name = ctx.label.name + (".exe" if is_windows else "")
    out_binary = ctx.actions.declare_file(out_name)

    # Collect JAR files from deploy_jar or java_binary input
    jar_file = ctx.file.jar

    # Build native-image arguments
    args = ctx.actions.args()
    args.add("-jar", jar_file.path)
    args.add("-H:Name=" + out_binary.path)
    
    for extra_arg in ctx.attr.extra_args:
        args.add(extra_arg)

    # Execute the native-image tool from the GraalVM toolchain/repo
    ctx.actions.run(
        outputs = [out_binary],
        inputs = [jar_file],
        executable = ctx.executable._native_image_tool,
        arguments = [args],
        mnemonic = "GraalVMNativeImage",
        progress_message = "Building GraalVM native image %s" % ctx.label.name,
        use_default_shell_env=True,
    )

    return [DefaultInfo(executable = out_binary)]

native_image = rule(
    implementation = _native_image_impl,
    executable = True,
    attrs = {
        "jar": attr.label(
            mandatory = True,
            allow_single_file = [".jar"],
            doc = "The deployable JAR file (e.g. :app_deploy.jar)",
        ),
        "extra_args": attr.string_list(
            default = [],
            doc = "Additional flags passed to native-image (e.g. --no-fallback)",
        ),
        "_native_image_tool": attr.label(
            default = Label("@graalvm//:bin/native-image"),
            allow_single_file = True,
            executable = True,
            cfg = "exec",
        ),
    },
)
