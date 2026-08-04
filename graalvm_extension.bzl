load("//:graalvm_repository.bzl", "graalvm_repository")

def _graalvm_ext_impl(mctx):
    graalvm_repository(name = "graalvm")

graalvm_ext = module_extension(
    implementation = _graalvm_ext_impl,
)
