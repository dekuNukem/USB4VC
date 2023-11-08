#!/bin/bash -eu
# see: https://github.com/RPi-Distro/pi-gen for details on this process

DEPS_STAGE="stage-usb4vc-deps"
APP_STAGE="stage-usb4vc-app"

BUILD_32BIT=${BUILD_32BIT:-1}
BUILD_64BIT=${BUILD_64BIT:-0}
SKIP_PI_STAGES=${SKIP_PI_STAGES:-0}
SKIP_DEPS_STAGE=${SKIP_DEPS_STAGE:-0}

setup_stages() {
    # prevent "lite" image output
    touch ./$1/stage2/SKIP_IMAGES

    # clean up skips
    rm -f ./$1/stage0/SKIP ./$1/stage1/SKIP ./$1/stage2/SKIP ./$DEPS_STAGE/SKIP

    if [ $SKIP_PI_STAGES = 1 ]; then
        touch ./$1/stage0/SKIP ./$1/stage1/SKIP ./$1/stage2/SKIP
    fi

    if [ $SKIP_DEPS_STAGE = 1 ]; then
        touch ./$DEPS_STAGE/SKIP
    fi
}

# prevent "lite" image output
setup_stages pi-gen
setup_stages pi-gen-arm64

# setup for the pi-gen build-docker script
CONTAINER_NAME=${CONTAINER_NAME:-pigen_usb4vc_work}

DEPS_STAGE_PATH=$(realpath -s "./${DEPS_STAGE}" || realpath "./${DEPS_STAGE}")
APP_STAGE_PATH=$(realpath -s "./${APP_STAGE}" || realpath "./${APP_STAGE}")
USERPROGRAM_PATH=$(realpath -s "../user_program" || realpath "../user_program")

# paths relative to inside pi-gen
CONFIG_FILE=${CONFIG_FILE:-"../config"}
PIGEN_DOCKER_OPTS=${PIGEN_DOCKER_OPTS:-""}
PIGEN_DOCKER_OPTS="${PIGEN_DOCKER_OPTS} --volume ${DEPS_STAGE_PATH}:/pi-gen/${DEPS_STAGE}"
PIGEN_DOCKER_OPTS="${PIGEN_DOCKER_OPTS} --volume ${APP_STAGE_PATH}:/pi-gen/${APP_STAGE}"
PIGEN_DOCKER_OPTS="${PIGEN_DOCKER_OPTS} --volume ${USERPROGRAM_PATH}:/pi-gen/${APP_STAGE}/00-user-program/files/rpi_app:ro"

export CONTAINER_NAME
export PIGEN_DOCKER_OPTS

if [ $BUILD_32BIT = 1 ]; then
    echo "Starting pi-gen..."
    pushd ./pi-gen && ./build-docker.sh -c $CONFIG_FILE && popd || popd && exit
    echo "pi-gen complete"
fi

if [ $BUILD_64BIT = 1 ]; then
    echo "Starting pi-gen for arm64..."
    export CONTAINER_NAME="arm64_${CONTAINER_NAME}"
    export PIGEN_DOCKER_OPTS=${PIGEN_DOCKER_OPTS:-" -e IS_PIGEN_ARM64=1"}
    pushd ./pi-gen-arm64 && ./build-docker.sh -c $CONFIG_FILE && popd || popd && exit
    echo "pi-gen arm64 complete"
fi

echo "All images should be created, check the deploy folder of each pi-gen."
