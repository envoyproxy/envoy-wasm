#!/bin/bash

set -e

# Setup basic requirements and install them.
apt-get update
export DEBIAN_FRONTEND=noninteractive
apt-get install -y curl wget software-properties-common make git python python-pip \
  bc libtool ninja-build automake zip time golang g++ gdb strace wireshark tshark tcpdump
# Install cmake 3.12.
curl -sLO https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.tar.gz
echo "0210f500c71af0ee7e8c42da76954298144d5f72f725ea381ae5db7b766b000e cmake-3.12.3-Linux-x86_64.tar.gz" | sha256sum --check
tar -zxf cmake-3.12.3-Linux-x86_64.tar.gz -C /usr --strip-components=1
# clang 7.
curl http://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-7 main"
apt-get update
apt-get install -y clang-7 clang-format-7 clang-tidy-7 lld-7
# Bazel and related dependencies.
apt-get install -y openjdk-8-jdk
echo "deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
curl https://bazel.build/bazel-release.pub.gpg | apt-key add -
apt-get update
apt-get install -y bazel
rm -rf /var/lib/apt/lists/*

# Setup tcpdump for non-root.
groupadd pcap
chgrp pcap /usr/sbin/tcpdump
chmod 750 /usr/sbin/tcpdump
setcap cap_net_raw,cap_net_admin=eip /usr/sbin/tcpdump

# virtualenv
pip install virtualenv

EXPECTED_CXX_VERSION="g++ (Ubuntu 5.4.0-6ubuntu1~16.04.10) 5.4.0 20160609" ./build_container_common.sh

