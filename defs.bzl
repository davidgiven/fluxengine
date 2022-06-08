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
    native.proto_library(
        name = name + "_proto",
        deps = deps
    )

    native.cc_proto_library(
        name = name + "_cc_proto",
        deps = [ name + "_proto" ]
    )

    exe = name + "_encoder"
    native.cc_binary(
        name = exe,
        srcs = [ "//scripts:proto_encode.cc" ],
        defines = [ "PROTO=" + message ],
        deps = [ name + "_cc_proto" ]
    )

    native.genrule(
        name = name,
        tools = [ exe ],
        srcs = [ src ],
        outs = [ name + ".binpb" ],
        cmd = "$(location :{}) < $< > $@".format(exe)
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

