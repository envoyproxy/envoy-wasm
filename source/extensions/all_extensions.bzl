load("@envoy_build_config//:extensions_build_config.bzl", "EXTENSIONS", "WINDOWS_EXTENSIONS")

# Return all extensions to be compiled into Envoy.
def envoy_all_extensions(blacklist = dict()):
    # These extensions are registered using the extension system but are required for the core
    # Envoy build.
    all_extensions = [
        "//source/extensions/transport_sockets/raw_buffer:config",
    ]

    # These extensions can be removed on a site specific basis.
    for name, path in EXTENSIONS.items():
        if not name in blacklist.values():
            all_extensions.append(path)

    return all_extensions

def envoy_windows_extensions():
    # These extensions are registered using the extension system but are required for the core
    # Envoy build.
    windows_extensions = [
        "//source/extensions/transport_sockets/raw_buffer:config",
        "//source/extensions/transport_sockets/tls:config",
    ]

    # These extensions can be removed on a site specific basis.
    for path in WINDOWS_EXTENSIONS.values():
        windows_extensions.append(path)

    return windows_extensions
