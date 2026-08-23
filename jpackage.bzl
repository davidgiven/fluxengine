_JPACKAGE_TYPES = ["deb", "rpm", "msi", "dmg", "unsupported"]

def _add_launcher_args(launchers):
    # launchers is a list of (name, properties_file) pairs.
    args = []
    for name, f in launchers:
        if name.endswith(".properties"):
            name = name[: -len(".properties")]
        args.append("--add-launcher %s=%s" % (name, f.path))
    return " ".join(args)

# Returns [(launcher_name, properties_file)] for --add-launcher. If an icon is
# set, each properties file gets a copy with an "icon=" line appended, pointing
# at the icon's execroot-relative path (jpackage resolves relative paths
# against its working directory, which for these actions is the execroot).
def _launcher_properties(ctx):
    icon = ctx.file.launcher_icon
    launchers = []
    for f in ctx.files.extra_launchers:
        name = f.basename
        if icon:
            out = ctx.actions.declare_file("with-icon-" + f.basename)
            ctx.actions.run_shell(
                outputs = [out],
                inputs = [f, icon],
                command = "cp {props} {out} && echo >> {out} && echo 'icon={icon}' >> {out}".format(
                    props = f.path,
                    out = out.path,
                    icon = icon.path,
                ),
            )
            launchers.append((name, out))
        else:
            launchers.append((name, f))
    return launchers


def _jpackage_impl(ctx):
    # Locate jpackage via the configured Java toolchain's runtime, so the rule
    # works with whatever JDK Bazel is using (e.g. remotejdk_21).
    java_runtime = ctx.toolchains["@bazel_tools//tools/jdk:toolchain_type"].java.java_runtime
    jpackage_path = java_runtime.java_home + "/bin/jpackage"

    package_type = ctx.attr.package_type
    if package_type == "unsupported":
        # Not the platform this installer targets (jpackage can't cross
        # compile); produce an empty target so `bazel build //java/...` still
        # works everywhere. Trying to actually use the output on the wrong
        # platform will just find nothing.
        return [DefaultInfo()]

    extension = {
        "deb": "deb",
        "rpm": "rpm",
        "msi": "msi",
        "dmg": "dmg",
    }[package_type]

    jar = ctx.file.jar
    out = ctx.actions.declare_file(ctx.attr.package_name + "_" + ctx.attr.app_version + "." + extension)
    launchers = _launcher_properties(ctx)

    # jpackage writes a lot of scratch state (a jlink runtime image and an app
    # image) and chmods files in it. Do all the scratch work in a plain
    # directory under the execroot (which is writable in the sandbox) and only
    # declare the final package as an output. The sandbox input jar is a
    # symlink to a read-only file, so dereference it (cp -L) and make the copy
    # writable.
    #
    # rpmbuild (invoked by jpackage for --type rpm) creates its temp scripts in
    # /var/tmp by default, which is read-only in the sandbox, so point it at the
    # scratch dir via a ~/.rpmmacros file.
    ctx.actions.run_shell(
        outputs = [out],
        inputs = [jar] + [f for (_, f) in launchers] +\
                 ([ctx.file.launcher_icon] if ctx.file.launcher_icon else []),
        tools = [java_runtime.files],
        use_default_shell_env = True,
        command = """
            rm -rf workdir
            mkdir -p workdir/input workdir/tmp workdir/dest workdir/home workdir/rpmbuild
            cp -L "{jar}" workdir/input/
            chmod u+w workdir/input/*
            if [ "{package_type}" = "rpm" ]; then
                WORKTMP="$(pwd)/workdir/tmp"
                RPMPREFIX="$(pwd)/workdir/rpmbuild"
                cat > workdir/home/.rpmmacros <<EOF
%_tmppath $WORKTMP
%_builddir $RPMPREFIX/BUILD
%_buildrootdir $RPMPREFIX/BUILDROOT
%_sourcedir $RPMPREFIX/SOURCES
%_specdir $RPMPREFIX/SPECS
%_srcrpmdir $RPMPREFIX/SRPMS
%_rpmdir $RPMPREFIX/RPMS
EOF
            fi
            TMPDIR="$(pwd)/workdir/tmp"
            HOME="$(pwd)/workdir/home"
            export TMPDIR HOME
            "{jpackage}" -J-Djava.io.tmpdir="$(pwd)/workdir/tmp" --type {package_type} \
                --name "{package_name}" \
                --app-version "{app_version}" \
                --input "$(pwd)/workdir/input" \
                --main-jar "{main_jar}" \
                --main-class "{main_class}" \
                {add_launcher_args} \
                --jlink-options "--strip-debug --no-header-files --no-man-pages --strip-native-commands" \
                --dest "$(pwd)/workdir/dest"
            cp workdir/dest/*.{extension} "{out}"
            rm -rf workdir
        """.format(
            jpackage = jpackage_path,
            package_type = package_type,
            extension = extension,
            package_name = ctx.attr.package_name,
            app_version = ctx.attr.app_version,
            jar = jar.path,
            main_jar = jar.basename,
            main_class = ctx.attr.main_class,
            add_launcher_args = _add_launcher_args(launchers),
            out = out.path,
        ),
        mnemonic = "Jpackage" + package_type.title(),
        progress_message = "Building %s installer %s" % (package_type, ctx.label.name),
    )

    return [DefaultInfo(files = depset([out]))]

def _jpackage_app_image_impl(ctx):
    # Locate jpackage via the configured Java toolchain's runtime, so the rule
    # works with whatever JDK Bazel is using (e.g. remotejdk_21).
    java_runtime = ctx.toolchains["@bazel_tools//tools/jdk:toolchain_type"].java.java_runtime
    jpackage_path = java_runtime.java_home + "/bin/jpackage"

    jar = ctx.file.jar
    out = ctx.actions.declare_file(ctx.attr.package_name + "_" + ctx.attr.app_version + ".tar.xz")
    launchers = _launcher_properties(ctx)

    # jpackage --type app-image writes a directory (with a jlink runtime image
    # and the app launcher) and chmods files in it. Do the scratch work in a
    # plain directory under the execroot (which is writable in the sandbox),
    # then tar the result up. The sandbox input jar is a symlink to a read-only
    # file, so dereference it (cp -L) and make the copy writable.
    ctx.actions.run_shell(
        outputs = [out],
        inputs = [jar] + [f for (_, f) in launchers] +\
                 ([ctx.file.launcher_icon] if ctx.file.launcher_icon else []),
        tools = [java_runtime.files],
        use_default_shell_env = True,
        command = """
            rm -rf workdir
            mkdir -p workdir/input workdir/tmp workdir/dest workdir/home
            cp -L "{jar}" workdir/input/
            chmod u+w workdir/input/*
            TMPDIR="$(pwd)/workdir/tmp"
            HOME="$(pwd)/workdir/home"
            export TMPDIR HOME
            "{jpackage}" -J-Djava.io.tmpdir="$(pwd)/workdir/tmp" --type app-image \
                --name "{package_name}" \
                --app-version "{app_version}" \
                --input "$(pwd)/workdir/input" \
                --main-jar "{main_jar}" \
                --main-class "{main_class}" \
                {add_launcher_args} \
                --jlink-options "--strip-debug --no-header-files --no-man-pages --strip-native-commands" \
                --dest "$(pwd)/workdir/dest"
            # jpackage leaves the runtime files read-only (Windows sets the +R
            # attribute), which makes tar fail with "Cannot open: Permission
            # denied". Make everything writable before taring.
            chmod -R u+w "$(pwd)/workdir/dest"
            tar cJf "{out}" -C "$(pwd)/workdir/dest" "{package_name}"
            rm -rf workdir
        """.format(
            jpackage = jpackage_path,
            package_name = ctx.attr.package_name,
            app_version = ctx.attr.app_version,
            jar = jar.path,
            main_jar = jar.basename,
            main_class = ctx.attr.main_class,
            add_launcher_args = _add_launcher_args(launchers),
            out = out.path,
        ),
        mnemonic = "JpackageAppImage",
        progress_message = "Building app image %s" % ctx.label.name,
    )

    return [DefaultInfo(files = depset([out]))]

_jpackage_attrs = {
    "jar": attr.label(
        mandatory = True,
        allow_single_file = [".jar"],
    ),
    "main_class": attr.string(
        mandatory = True,
    ),
    "extra_launchers": attr.label_list(
        allow_files = [".properties"],
        doc = "jpackage launcher properties files for additional launchers; " +
              "each launcher is named after the file (minus its .properties suffix).",
    ),
    "launcher_icon": attr.label(
        allow_single_file = [".png", ".ico", ".icns"],
        doc = "Icon for all additional launchers, added to each launcher's " +
              "properties as an icon= entry. jpackage requires .png on Linux, " +
              ".ico on Windows and .icns on macOS.",
    ),
    "package_name": attr.string(
        mandatory = True,
        doc = "The package name; also used for the output filename.",
    ),
    "app_version": attr.string(
        mandatory = True,
        doc = "Application version, e.g. '1.0.0'.",
    ),
    "package_type": attr.string(
        mandatory = True,
        values = _JPACKAGE_TYPES,
        doc = "The jpackage package type: deb/rpm (Linux), msi (Windows), dmg (macOS). " +
              "Use select() so this is only set to the matching platform.",
    ),
}

jpackage = rule(
    implementation = _jpackage_impl,
    attrs = dict(_jpackage_attrs),
    toolchains = ["@bazel_tools//tools/jdk:toolchain_type"],
)

jpackage_app_image = rule(
    implementation = _jpackage_app_image_impl,
    attrs = dict(
        {
            k: v
            for k, v in _jpackage_attrs.items()
            if k != "package_type"
        },
    ),
    toolchains = ["@bazel_tools//tools/jdk:toolchain_type"],
)
