def _compilation_mode_transition(settings, attr):
    return [
        {"//command_line_option:compilation_mode": attr.mode},
    ]

compilation_mode_transition = transition(
    implementation = _compilation_mode_transition,
    inputs = [],
    outputs = [
        "//command_line_option:compilation_mode",
    ],
)

def _nop_impl(ctx):
    return [
        DefaultInfo(files = depset(ctx.files.deps))
    ]

compilation_mode = rule(
        implementation = _nop_impl,
        cfg = compilation_mode_transition,
        attrs = {
            "deps": attr.label_list(),
            "mode": attr.string(),
            "_whitelist_function_transition": attr.label(
                default = "@bazel_tools//tools/whitelists/function_transition_whitelist"),
        },
    )

def cc_binary_both(name, srcs, linkopts=[], deps=[]):
    native.cc_binary(
        name = name,
        srcs = srcs,
        linkstatic = True,
        linkopts = linkopts,
        deps = deps
    )

    compilation_mode(
        name = name + "_opt",
        deps = [ ":" + name ],
        mode = "opt",
    )

    compilation_mode(
        name = name + "_dbg",
        deps = [ ":" + name ],
        mode = "dbg",
    )

def proto_encode(name, src, message, proto, deps=[]):
    native.genrule(
        name = name,
        srcs = [ src ] + deps,
        outs = [ name + ".binpb" ],
        cmd = "protoc --encode={} --descriptor_set_in={} {} < $(location {}) > $@".format(
            message, ":".join(["$(location {})".format(n) for n in deps]), proto, src)
    )

def objectify(name, src, identifier=None):
    if not identifier:
        identifier = name

    native.genrule(
        name = name,
        srcs = [ src ],
        outs = [ name + ".cc" ],
        cmd = "(" + " && ".join([
            "echo '#include <string>'",
            "echo 'static const unsigned char data[] = {'",
            "xxd -i < $<",
            "echo '};'",
            "echo 'extern std::string {}();'".format(identifier),
            "echo 'std::string {}() {{'".format(identifier),
            "echo '  return std::string((const char*)data, sizeof(data));'",
            "echo '}'"
        ]) + ")> $@"
    )

# vim: ts=4 sw=4 et

