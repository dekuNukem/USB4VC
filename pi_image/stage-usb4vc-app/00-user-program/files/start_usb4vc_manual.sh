#!/bin/sh -e

if [ $(id -u) -ne 0 ]; then
  printf "usb4vc must be run as root. Try 'sudo start_usb4vc_manual.sh'\n"
  exit 1
fi

pushd /opt/usb4vc/rpi_app
export USB4VC_IS_SYSTEMD=0
export USB4VC_INSTALL_PATH=/opt/usb4vc
export XPADNEO_SOURCE_PATH=/opt/xpadneo

/opt/usb4vc/venv/bin/python3 -u usb4vc_main.py || echo "usb4vc did not exit cleanly: $?"
popd
