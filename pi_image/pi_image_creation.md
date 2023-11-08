# Raspberry Pi Disk Image
#### Building a Raspberry Pi disk image containing usb4vc using pi-gen.

This process currently relies on Docker and pi-gen. Docker is the easiest method for using pi-gen as it requires very specific versions of ubuntu or debian to run standalone.

Please read through the pi-gen documentation to have a good understanding of how to setup the build environment and what stages / config mean:
- https://github.com/RPi-Distro/pi-gen

## Setup
- First the submodules for pi-gen and the usb4vc-deps must be cloned:
    ```bash
    git submodule update --init --recursive
    ```
    These are shallow clones by default, so to switch branch (e.g. to bullseye from bookworm) you will need to perform a fetch.
- Ensure docker is installed and accessible from the command line. Linux hosts / WSL should work fine.
    https://docs.docker.com/engine/install/

## Overview
- `pi-gen`: A clone of pi-gen for armhf / armv7 (32bit ARM)
- `pi-gen-arm64`: A clone of pi-gen for arm64 (64bit ARM, newer Pi's)
- `stage-usb4vc-deps`: A pi-gen stage containing all software dependencies for usb4vc.
- `stage-usb4vc-app`: A pi-gen stage containing the scripts to install usb4vc inside the image.
- `config`: The pi-gen config used for the build.
- `build-docker.sh`: Helper script to automatically call pi-gen(-arm64)'s build-docker.sh with the correct arguments.

## Quick Usage
Once docker is installed and the submodules have been updated / cloned, you can run the build script.
- To build just the 32bit image (the default):
    ```bash
    ./build-docker.sh
    ```
- To build 32bit and 64bit images:
    ```bash
    BUILD_32BIT=1 BUILD_64BIT=1 ./build-docker.sh
    ```
- To build just the 64bit image:
    ```bash
    BUILD_32BIT=0 BUILD_64BIT=1 ./build-docker.sh
    ```
- To preserve the container to speed up iteration:
    ```bash
    BUILD_32BIT=1 BUILD_64BIT=1 PRESERVE_CONTAINER=1 ./build-docker.sh
    ```
- To continue and retry a failed build:
    ```bash
    BUILD_32BIT=1 BUILD_64BIT=1 PRESERVE_CONTAINER=1 CONTINUE=1 ./build-docker.sh
    ```

Outputs can be found in:
- `pi-gen/deploy`
- `pi-gen-arm64/deploy`

### APT Proxy
The APT proxy can be used to cache packages locally to speed up image building from scratch. See `APT_PROXY` under `Config` in pi-gen: https://github.com/RPi-Distro/pi-gen#config

You can then uncomment / modify the line in the `config` file.

## Script Configuration
All configuration for `build-docker.sh` is controlled via environment variables.
- `BUILD_32BIT` (Default: `1`)
 
    Toggle the pi-gen build for a 32bit image.
- `BUILD_64BIT` (Default: `0`)

    Toggle the pi-gen-arm64 build for a 64bit image.
- `SKIP_PI_STAGES` (Default: `0`)

    Toggle skipping the base pi image stages (stage0, stage1, stage2). It will automatically create the `SKIP` files in each stage directory. This can help speed up image build time when combines with `PRESERVE_CONTAINER=1` and `CONTINUE=1` when something goes wrong in the `stage-usb4vc-deps` or `stage-usb4vc-app` stage.
- `SKIP_DEPS_STAGE` (Default: `0`)

    Toggle skipping the `stage-usb4vc-deps` stage. Similar to `SKIP_PI_STAGES` but will skip the dependencies stage specifically (much faster if you already have a preserved base of stage 0-2 with deps you just want to update the app inside).

All environment variables are passed through to the underlying pi-gen build-docker.sh, but some are manipulated by the script here:
- `CONFIG_FILE` (Default `"../config"`)

    Path to the config file (relative from inside pi-gen as it mounts it to the container itself).
- `CONTAINER_NAME` (Default `"pigen_usb4vc_work"`)

    The name of the container used for the pi-gen work. When `BUILD_64BIT` is `1`, this container name will have `arm64_` prepended to the name as the pi-gen build-docker checks if any container matches the container name (meaning it throws an error if you try to preserve both the 32bit and 64bit containers).
- `PIGEN_DOCKER_OPTS` (Default `""`)

    This passes extra options to the `docker run` call for pi-gen. By default this is empty, but the script always adds the volumes for the usb4vc stages and the user_program. You can use this to specify any additonal docker options.

The script mounts some volumes in docker for easy iteration outside the container environment:
- `./stage-usb4vc-deps:/pi-gen/stage-usb4vc-deps`
- `./stage-usb4vc-app:/pi-gen/stage-usb4vc-app`
- `../user_program:/pi-gen/stage-usb4vc-app/00-user-program/files/rpi_app`

## Stages
The build stages run in this order by default (specified in `config`):
- stage0
- stage1
- stage2 (this is a "lite" rpi-image)
- stage-usb4vc-deps
- stage-usb4vc-app (this is where the final image is exported)

### `stage-usb4vc-deps`
This stage installs all software dependencies for usb4vc. See `00-packages` for exact list, but in general:
- python3
- stm32flash
- i2c-tools
- dfutil
- etc..

A virtualenv is created for usb4vc in `/opt/usb4vc/venv`, this is a requirement in newer raspios distributions for installing non api provided python packages.
- Creates python3 venv in `/opt/usb4vc/venv`.
- Installs `evdev`, `spidev`, `serial` and `luma.oled`.
- Purges pip cache to save disk space.

It also compiles and installs (see `00-run.sh`):
- xpadneo
    - Only the source code for usb4vc to compile on startup in `/opt/xpadneo`.
- joycond
- dkms-hid-nintendo
    - For this module, the script must find all kernels inside the image and install dkms for each (to ensure the module works on every pi the image supports).

After the dependencies are installed, this stage performs some system tweaks specific to usb4vc. See `01-tweaks` and `01-run.sh`:
- Installs uniput and udisks udev rules with a script started by systemd (`usb-automount.service` and `udev_usb_monitor.sh`) to monitor for newly attached USB storage devices and automount them to `/media`. This replaces `usbmount` from old Debian systems which is no longer maintained.
- Attempts to disable boot wait in `config.txt`.
- Disables the interactive serial console and enables hardware serial.
- Enables SPI and I2C.
- Disables the boot splash.
- Sets the boot delay to 0.
- Disables the ctrl-alt-del systemd target.
- Enables the uhid kernel module (for xpadneo).
- Masks the userconf systemd service.
    - This may not be necessary but in some cases when connecting a keyboard it was constantly running a background process to configure the key map.
- If the image being built is 32bit:
    - Sets arm_64bit to 0 in `config.txt`. This is to prevent the 64bit kernel from loading in the 32bit image, this is the default behaviour with modern rpi images and it breaks compiling xpadneo as there are no cross compilation kernel headers available for the system.

### `stage-usb4vc-app`
This stage copies `user_program` into `/opt/usb4vc/rpi_app` and is also the stage where `EXPORT_IMAGE` is specified.
- Creates the `rpi_app`, `config`, `firmware` and `temp` directories under `/opt/usb4vc`.
- Installs the `usb4vc.service` systemd service to run usb4vc in the background on boot.
    - This service configures the environment variables to override the compatability for `/home/pi` in usb4vc, and starts usb4vc using the virtualenv.
    - Logs for usb4vc are available via `journalctl -u usb4vc.service`.
- Copies across some helper scripts and a note about the debug log.
- Copies `*.py` and `*.ttf` from `user_program` to `rpi_app`.
- Enables the `usb4vc.service` systemd service.

## Outputs
All outputs can be found in their respective `pi-gen`'s `deploy` folder. Currently 64bit and 32bit images have the same output name, but they output to different folders. The 64bit one can be renamed for upload.