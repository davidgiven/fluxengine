"""Generates version.properties containing fluxengine.version from Bazel stamp."""

def _version_properties_impl(ctx):
    out = ctx.actions.declare_file("version.properties")
    inputs = []
    if ctx.attr.stamp:
        inputs.append(ctx.info_file)
    ctx.actions.run_shell(
        outputs = [out],
        inputs = inputs,
        command = """
            VER="1.0.0-dev"
            if [ "{stamp}" = "True" ] && [ -f "{info_file}" ]; then
                STABLE=$(grep STABLE_VERSION "{info_file}" | cut -d' ' -f2 || true)
                if [ -n "$STABLE" ]; then
                    VER="$STABLE"
                fi
            fi
            echo "fluxengine.version=$VER" > "{out}"
        """.format(
            stamp = str(ctx.attr.stamp),
            info_file = ctx.info_file.path if ctx.attr.stamp else "",
            out = out.path,
        ),
        mnemonic = "VersionProperties",
        progress_message = "Generating version.properties %{label}",
    )
    return [DefaultInfo(files = depset([out]))]

version_properties = rule(
    implementation = _version_properties_impl,
    attrs = {
        "stamp": attr.bool(
            default = True,
            doc = "If true, read STABLE_VERSION from stable-status (requires --stamp).",
        ),
    },
)
