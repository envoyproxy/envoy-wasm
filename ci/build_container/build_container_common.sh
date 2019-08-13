#!/bin/bash -e

if [[ "$(uname -m)" == "x86_64" ]]; then
  # buildifier
  VERSION=0.28.0
  SHA256=3d474be62f8e18190546881daf3c6337d857bf371faf23f508e9b456b0244267
  curl --location --output /usr/local/bin/buildifier https://github.com/bazelbuild/buildtools/releases/download/"$VERSION"/buildifier \
    && echo "$SHA256  /usr/local/bin/buildifier" | sha256sum --check \
    && chmod +x /usr/local/bin/buildifier

  # bazelisk
  VERSION=0.0.8
  SHA256=5fced4fec06bf24beb631837fa9497b6698f34041463d9188610dfa7b91f4f8d
  curl --location --output /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/v${VERSION}/bazelisk-linux-amd64 \
    && echo "$SHA256  /usr/local/bin/bazel" | sha256sum --check \
    && chmod +x /usr/local/bin/bazel

  # CMake
  VERSION=3.14.5
  SHA256=757e7ddd29d11422334a60d7cd9c19b7af20349a03cb15aa3c879f1b54193c95
  curl -sLo cmake-"$VERSION".tar.gz https://github.com/Kitware/CMake/releases/download/v"$VERSION"/cmake-"$VERSION"-Linux-x86_64.tar.gz \
    && echo "$SHA256" cmake-"$VERSION".tar.gz | sha256sum --check \
    && tar -zxf cmake-"$VERSION".tar.gz -C /usr --strip-components=1

  # ninja
  VERSION=1.8.2
  SHA256=d2fea9ff33b3ef353161ed906f260d565ca55b8ca0568fa07b1d2cab90a84a07
  curl -sLo ninja-"$VERSION".zip https://github.com/ninja-build/ninja/releases/download/v"$VERSION"/ninja-linux.zip \
    && echo "$SHA256" ninja-"$VERSION".zip | sha256sum --check \
    && unzip ninja-"$VERSION".zip \
    && mv ninja /usr/bin
fi
