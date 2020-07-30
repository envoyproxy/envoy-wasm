load("@io_bazel_rules_rust//rust:rust.bzl", "rust_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

def _wasm_cc_transition_impl(settings, attr):
    return {
        "//command_line_option:cpu": "wasm32",
        "//command_line_option:crosstool_top": "@proxy_wasm_cpp_sdk//toolchain:emscripten",

        # Overriding copt/cxxopt/linkopt to prevent sanitizers/coverage options leak
        # into WASM build configuration
        "//command_line_option:copt": [],
        "//command_line_option:cxxopt": [],
        "//command_line_option:linkopt": [],
        "//command_line_option:collect_code_coverage": "false",
    }

def _wasm_rust_transition_impl(settings, attr):
    return {
        "//command_line_option:platforms": "@io_bazel_rules_rust//rust/platform:wasm",
    }

wasm_cc_transition = transition(
    implementation = _wasm_cc_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:cpu",
        "//command_line_option:crosstool_top",
        "//command_line_option:copt",
        "//command_line_option:cxxopt",
        "//command_line_option:linkopt",
        "//command_line_option:collect_code_coverage",
    ],
)

wasm_rust_transition = transition(
    implementation = _wasm_rust_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:platforms",
    ],
)

def _wasm_binary_impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.run_shell(
        command = 'cp "{}" "{}"'.format(ctx.files.binary[0].path, out.path),
        outputs = [out],
        inputs = ctx.files.binary,
    )

    return [DefaultInfo(runfiles = ctx.runfiles([out]))]

# WASM binary rule implementation.
# This copies the binary specified in binary attribute in WASM configuration to
# target configuration, so a binary in non-WASM configuration can depend on them.
wasm_cc_binary_rule = rule(
    implementation = _wasm_binary_impl,
    attrs = {
        "binary": attr.label(mandatory = True, cfg = wasm_cc_transition),
        "_whitelist_function_transition": attr.label(default = "@bazel_tools//tools/whitelists/function_transition_whitelist"),
    },
)

wasm_rust_binary_rule = rule(
    implementation = _wasm_binary_impl,
    attrs = {
        "binary": attr.label(mandatory = True, cfg = wasm_rust_transition),
        "_whitelist_function_transition": attr.label(default = "@bazel_tools//tools/whitelists/function_transition_whitelist"),
    },
)

def wasm_cc_binary(name, **kwargs):
    wasm_name = "_wasm_" + name
    kwargs.setdefault("additional_linker_inputs", ["@proxy_wasm_cpp_sdk//:jslib"])
    kwargs.setdefault("linkopts", ["--js-library external/proxy_wasm_cpp_sdk/proxy_wasm_intrinsics.js"])
    kwargs.setdefault("visibility", ["//visibility:public"])
    cc_binary(
        name = wasm_name,
        # Adding manual tag it won't be built in non-WASM (e.g. x86_64 config)
        # when an wildcard is specified, but it will be built in WASM configuration
        # when the wasm_binary below is built.
        tags = ["manual"],
        **kwargs
    )

    wasm_cc_binary_rule(
        name = name,
        binary = ":" + wasm_name,
    )

def wasm_rust_binary(name, **kwargs):
    wasm_name = "_wasm_" + (name if not ".wasm" in name else name.strip(".wasm"))
    kwargs.setdefault("visibility", ["//visibility:public"])
    kwargs.setdefault("rustc_flags", ["--edition=2018"])
    kwargs.setdefault("out_binary", True)

    rust_binary(
        name = wasm_name,
        **kwargs
    )

    wasm_rust_binary_rule(
        name = name,
        binary = ":" + wasm_name,
    )

    # native.genrule(
    #     name = name,
    #     srcs = [":precompile_" + name],
    #     outs = [name],
    #     tools = ["//test/tools/wee8_compile:wee8_compile_tool"],
    #     cmd = "$(location //test/tools/wee8_compile:wee8_compile_tool) $(SRCS).runfiles $(OUTS).runfiles",
    # )
