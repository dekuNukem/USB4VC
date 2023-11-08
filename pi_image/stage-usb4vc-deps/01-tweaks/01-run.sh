#!/bin/bash -e

install -v -m 644 "files/40-uinput.rules" "${ROOTFS_DIR}/etc/udev/rules.d/"
install -v -m 644 "files/99-udisks2.rules" "${ROOTFS_DIR}/etc/udev/rules.d/"

# tmpfiles conf to clean /media on boot if stale mounts exist
install -v -m 644 "files/tmpfiles_media.conf" "${ROOTFS_DIR}/etc/tmpfiles.d/media.conf"
install -v -m 755 "files/udev_usb_monitor.sh" "${ROOTFS_DIR}/opt/udev_usb_monitor.sh"
install -v -m 664 "files/usb-automount.service" "${ROOTFS_DIR}/etc/systemd/system/usb-automount.service"

# Disable splash, waiting for network, interactive logon serial
# Enable spi and i2c
# (raspi config 1 or 0 is opposite to what you would expect, dialogue boxes on "Yes" return 0)
on_chroot << EOF
    do_nothing() {
        echo ""
    }

    alias modprobe=do_nothing
    alias dtparam=do_nothing

    if [ -e /boot/firmware/config.txt ] ; then
        FIRMWARE=/firmware
    else
        FIRMWARE=
    fi
    CONFIG=/boot\${FIRMWARE}/config.txt

    echo "boot config location: \$CONFIG"

    echo "disabling boot wait"
	SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_boot_wait 1 || echo "disabling boot wait not supported on this raspi-config version"

    echo "disabling interactive serial but enabling UART"
    {
        SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_serial_cons 1;
        SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_serial_hw 0;
    } || {
        SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_serial 2;
    }
    
    echo "enabling spi (ignore configfs, modprobe errors)"
    SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_spi 0

    echo "enabling i2c (ignore configfs, modprobe errors)"
    SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint do_i2c 0

    echo "disabling splash"
    SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint set_config_var disable_splash 1 \$CONFIG

    echo "setting boot delay to 0"
    SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint set_config_var boot_delay 0 \$CONFIG

    echo "masking ctrl-alt-drl systemd target"
    systemctl mask ctrl-alt-del.target

    echo "masking userconf systemd service"
    systemctl mask userconfig.service

    echo "enabling the usb automount service"
    systemctl enable usb-automount.service

    if ! [ \`getconf LONG_BIT\` = "64" ]; then
        echo "disabling arm_64bit in boot config"
        SUDO_USER="${FIRST_USER_NAME}" raspi-config nonint set_config_var arm_64bit 0 \$CONFIG
    fi

    echo "enabling uhid module"
    if ! grep -q "uhid" /etc/modules; then
        echo 'uhid' | tee -a /etc/modules
    fi

    unalias modprobe
    unalias dtparam
EOF
