#!/bin/bash -e

install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/usb4vc/rpi_app"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/usb4vc/config"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/usb4vc/firmware"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/usb4vc/temp"

install -v -m 664 "files/usb4vc.service" "${ROOTFS_DIR}/etc/systemd/system/usb4vc.service"
install -v -o 1000 -g 1000 -m 755 "files/start_usb4vc_manual.sh" "${ROOTFS_DIR}/opt/usb4vc/start_usb4vc_manual.sh"
install -v -o 1000 -g 1000 -m 644 "files/usb4vc_debug_log.txt" "${ROOTFS_DIR}/home/${FIRST_USER_NAME}/usb4vc_debug_log.txt"

rsync -r --include="*.py" --include="*.ttf" --exclude="*" \
 "files/rpi_app/" "${ROOTFS_DIR}/opt/usb4vc/rpi_app/"

chown -R 1000:1000 "${ROOTFS_DIR}/opt/usb4vc/rpi_app"

on_chroot << EOF
systemctl enable usb4vc.service
EOF