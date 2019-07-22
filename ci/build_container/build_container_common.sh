#!/bin/bash -e

# buildifier
VERSION=0.25.0
SHA256=6e6aea35b2ea2b4951163f686dfbfe47b49c840c56b873b3a7afe60939772fc1
curl --location --output /usr/local/bin/buildifier https://github.com/bazelbuild/buildtools/releases/download/"$VERSION"/buildifier \
  && echo "$SHA256" '/usr/local/bin/buildifier' | sha256sum --check \
  && chmod +x /usr/local/bin/buildifier

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
