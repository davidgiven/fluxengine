_JPACKAGE_TYPES = ["deb", "rpm", "msi", "dmg", "unsupported"]

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
        inputs = [jar],
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
                --name "{name}" \
                --app-version "{app_version}" \
                --input "$(pwd)/workdir/input" \
                --main-jar "{main_jar}" \
                --main-class "{main_class}" \
                --dest "$(pwd)/workdir/dest"
            cp workdir/dest/*.{extension} "{out}"
            rm -rf workdir
        """.format(
            jpackage = jpackage_path,
            package_type = package_type,
            extension = extension,
            name = ctx.attr.name,
            app_version = ctx.attr.app_version,
            jar = jar.path,
            main_jar = jar.basename,
            main_class = ctx.attr.main_class,
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
    out = ctx.actions.declare_file(ctx.attr.package_name + "_" + ctx.attr.app_version + ".tar")

    # jpackage --type app-image writes a directory (with a jlink runtime image
    # and the app launcher) and chmods files in it. Do the scratch work in a
    # plain directory under the execroot (which is writable in the sandbox),
    # then tar the result up. The sandbox input jar is a symlink to a read-only
    # file, so dereference it (cp -L) and make the copy writable.
    ctx.actions.run_shell(
        outputs = [out],
        inputs = [jar],
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
                --name "{name}" \
                --app-version "{app_version}" \
                --input "$(pwd)/workdir/input" \
                --main-jar "{main_jar}" \
                --main-class "{main_class}" \
                --dest "$(pwd)/workdir/dest"
            tar cf "{out}" -C "$(pwd)/workdir/dest" "{name}"
            rm -rf workdir
        """.format(
            jpackage = jpackage_path,
            name = ctx.attr.name,
            app_version = ctx.attr.app_version,
            jar = jar.path,
            main_jar = jar.basename,
            main_class = ctx.attr.main_class,
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
