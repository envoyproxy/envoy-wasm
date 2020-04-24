def _bazel_wasm_repository_impl(ctx):
    root = ctx.path(ctx.attr.envoy_root).dirname
    ctx.symlink(root.get_child(".bazelversion"), ".bazelversion")

    path = root
    for child in ctx.attr.reldir.split("/"):
        path = path.get_child(child)

    for p in path.readdir():
        if not p.basename.startswith("bazel-"):
            ctx.symlink(p, p.basename)

    targets = ctx.execute(["bazel", "query", "--output=label", "filter('.wasm$', //:all)"]).stdout.splitlines()

    ctx.file("BUILD.bazel", """
load("@envoy//bazel:bazel_repository.bzl", "bazel_build")

bazel_build(
    name = "build",
    srcs = [":all"],
    outs = {},
    log = "log.txt",
    script = "build.sh",
)

filegroup(
    name = "all",
    srcs = glob(
        ["**"],
        exclude = ["BUILD.bazel"],
    ),
    visibility = ["//visibility:public"],
)
""".format(str([t.replace(":", "/").replace("///", "") for t in targets])))

    ctx.file("build.sh", """
#!/bin/bash

set -ex

export HOME="$(mktemp -d)"
export TEST_TMPDIR="$(mktemp -d)"
export USE_BAZEL_VERSION="{}"
SCRIPT_DIR="$(realpath "$(dirname "$0")")"

unset BAZEL_DO_NOT_DETECT_CPP_TOOLCHAIN

pushd $SCRIPT_DIR

BAZEL_STARTUP_OPTIONS=""
bazel $BAZEL_STARTUP_OPTIONS build --announce_rc --symlink_prefix=/ {}

BIN_DIR=$(bazel $BAZEL_STARTUP_OPTIONS info bazel-bin)
popd

for out in "$@"; do
    cp $BIN_DIR/$(basename $out) $out
done

""".format(ctx.os.environ.get("USE_BAZEL_VERSION", ""), " ".join(targets)))

bazel_wasm_repository = repository_rule(
    implementation = _bazel_wasm_repository_impl,
    configure = True,
    attrs = {
        "envoy_root": attr.label(default = "@envoy//:BUILD"),
        "reldir": attr.string(),
    },
    environ = ["USE_BAZEL_VERSION"],
)

def _bazel_build_impl(ctx):
    sources = depset([], transitive = [t.files for t in ctx.attr.srcs])

    command = """bash {} {}""".format(
        ctx.file.script.path,
        " ".join([f.path for f in ctx.outputs.outs]),
    )

    outs = [] + ctx.outputs.outs
    if ctx.outputs.log:
        command += " &> " + ctx.outputs.log.path
        outs.append(ctx.outputs.log)

    ctx.actions.run_shell(
        command = command,
        inputs = sources,
        mnemonic = "BazelBuild",
        outputs = outs,
        use_default_shell_env = True,
    )

bazel_build = rule(
    implementation = _bazel_build_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "script": attr.label(allow_single_file = True),
        "log": attr.output(),
        "outs": attr.output_list(),
    },
)
