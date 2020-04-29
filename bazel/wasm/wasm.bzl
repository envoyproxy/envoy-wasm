def _wasm_transition_impl(settings, attr):
    return {
        "//command_line_option:cpu": "wasm",
        "//command_line_option:crosstool_top": "@proxy_wasm_cpp_sdk//toolchain:emscripten",
    }

wasm_transition = transition(
    implementation = _wasm_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:cpu",
        "//command_line_option:crosstool_top",
    ],
)

def _wasm_binary_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.run_shell(
        command = 'cp "{}" "{}"'.format(ctx.files.binary[0].path, out.path),
        outputs = [out],
        inputs = ctx.files.binary,
    )
    print(ctx.files.binary[0].path)
    print(out.path)

    return [DefaultInfo(files = depset([out]))]

wasm_binary = rule(
    implementation = _wasm_binary_impl,
    attrs = {
        "binary": attr.label(mandatory = True, cfg = wasm_transition),
        "_whitelist_function_transition": attr.label(default = "@bazel_tools//tools/whitelists/function_transition_whitelist"),
    },
)

def wasm_cc_binary(name, **kwargs):
    wasm_name = "_wasm_" + name
    native.cc_binary(
        name = wasm_name,
        tags = ["manual"],
        **kwargs
    )
    wasm_binary(
        name = name,
        binary = wasm_name,
    )
