#!/bin/bash

# Do not ever set -x here, it is a security hazard as it will place the credentials below in the
# CI logs.
set -e

ENVOY_DOCKER_IMAGE_DIRECTORY="${ENVOY_DOCKER_IMAGE_DIRECTORY:-${BUILD_STAGINGDIRECTORY:-.}/build_images}"

# Setting environments for buildx tools
config_env() {
  # Qemu configurations
  docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

  # Remove older build instance
  docker buildx rm multi-builder | true
  docker buildx create --use --name multi-builder --platform linux/arm64,linux/amd64
}

build_platforms() {
  TYPE=$1
  FILE_SUFFIX="${TYPE/-debug/}"

  if [[ -z "${FILE_SUFFIX}" ]]; then
    echo "linux/arm64,linux/amd64"
  else
    echo "linux/amd64"
  fi
}

build_args() {
  TYPE=$1
  FILE_SUFFIX="${TYPE/-debug/}"

  echo "-f ci/Dockerfile-envoy${FILE_SUFFIX}"
  [[ "${TYPE}" == *-debug ]] && echo "--build-arg ENVOY_BINARY_SUFFIX="
  if [[ "${TYPE}" == "-google-vrp" ]]; then
    echo "--build-arg ENVOY_VRP_BASE_IMAGE=${VRP_BASE_IMAGE}"
  fi
}

use_builder() {
  TYPE=$1
  if [[ "${TYPE}" == "-google-vrp" ]]; then
    docker buildx use default
  else
    docker buildx use multi-builder
  fi
}

IMAGES_TO_SAVE=()

build_images() {
  TYPE=$1
  BUILD_TAG=$2

  use_builder "${TYPE}"
  ARGS="$(build_args ${TYPE})"
  PLATFORM="$(build_platforms ${TYPE})"

  docker buildx build --platform "${PLATFORM}" ${ARGS} -t "${BUILD_TAG}" .

  PLATFORM="$(build_platforms ${TYPE} | tr ',' ' ')"
  # docker buildx load cannot have multiple platform, load individually
  for ARCH in ${PLATFORM}; do
    if [[ "${ARCH}" == "linux/amd64" ]]; then
      IMAGE_TAG="${BUILD_TAG}"
    else
      IMAGE_TAG="${BUILD_TAG}-${ARCH/linux\//}"
    fi
    docker buildx build --platform "${ARCH}" ${ARGS} -t "${IMAGE_TAG}" . --load
    IMAGES_TO_SAVE+=("${IMAGE_TAG}")
  done
}

push_images() {
  TYPE=$1
  BUILD_TAG=$2

  use_builder "${TYPE}"
  ARGS="$(build_args ${TYPE})"
  PLATFORM="$(build_platforms ${TYPE})"
  # docker buildx doesn't do push with default builder
  docker buildx build --platform "${PLATFORM}" ${ARGS} -t ${BUILD_TAG} . --push || \
    docker push "${BUILD_TAG}"
}

MASTER_BRANCH="refs/heads/master"
RELEASE_BRANCH_REGEX="^refs/heads/release/v.*"
RELEASE_TAG_REGEX="^refs/tags/v.*"

# For master builds and release branch builds use the dev repo. Otherwise we assume it's a tag and
# we push to the primary repo.
if [[ "${AZP_BRANCH}" =~ "${RELEASE_TAG_REGEX}" ]]; then
  IMAGE_POSTFIX=""
  IMAGE_NAME="${AZP_BRANCH/refs\/tags\//}"
else
  IMAGE_POSTFIX="-dev"
  IMAGE_NAME="${AZP_SHA1}"
fi

# This prefix is altered for the private security images on setec builds.
DOCKER_IMAGE_PREFIX="${DOCKER_IMAGE_PREFIX:-envoyproxy/envoy}"

# "-google-vrp" must come afer "" to ensure we rebuild the local base image dependency.
BUILD_TYPES=("" "-debug" "-alpine" "-alpine-debug" "-google-vrp")

# Configure docker-buildx tools
config_env

# VRP base image is only for amd64
VRP_BASE_IMAGE="${DOCKER_IMAGE_PREFIX}${IMAGE_POSTFIX}:${IMAGE_NAME}"

# Test the docker build in all cases, but use a local tag that we will overwrite before push in the
# cases where we do push.
for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
  build_images "${BUILD_TYPE}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${IMAGE_NAME}"
done

mkdir -p "${ENVOY_DOCKER_IMAGE_DIRECTORY}"
ENVOY_DOCKER_TAR="${ENVOY_DOCKER_IMAGE_DIRECTORY}/envoy-docker-images.tar.xz"
echo "Saving built images to ${ENVOY_DOCKER_TAR}."
docker save "${IMAGES_TO_SAVE[@]}" | xz -T0 -2 >"${ENVOY_DOCKER_TAR}"

# Only push images for master builds, release branch builds, and tag builds.
if [[ "${AZP_BRANCH}" != "${MASTER_BRANCH}" ]] &&
  ! [[ "${AZP_BRANCH}" =~ ${RELEASE_BRANCH_REGEX} ]] &&
  ! [[ "${AZP_BRANCH}" =~ ${RELEASE_TAG_REGEX} ]]; then
  echo 'Ignoring non-master branch or tag for docker push.'
  exit 0
fi

docker login -u "$DOCKERHUB_USERNAME" -p "$DOCKERHUB_PASSWORD"

for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
  push_images "${BUILD_TYPE}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${IMAGE_NAME}"

  # Only push latest on master builds.
  if [[ "${AZP_BRANCH}" == "${MASTER_BRANCH}" ]]; then
    docker tag "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${IMAGE_NAME}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:latest"
    push_images "${BUILD_TYPE}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:latest"
  fi

  # Push vX.Y-latest to tag the latest image in a release line
  if [[ "${AZP_BRANCH}" =~ ${RELEASE_TAG_REGEX} ]]; then
    RELEASE_LINE=$(echo "$IMAGE_NAME" | sed -E 's/(v[0-9]+\.[0-9]+)\.[0-9]+/\1-latest/')
    docker tag "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${IMAGE_NAME}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${RELEASE_LINE}"
    push_images "${BUILD_TYPE}" "${DOCKER_IMAGE_PREFIX}${BUILD_TYPE}${IMAGE_POSTFIX}:${RELEASE_LINE}"
  fi
done
