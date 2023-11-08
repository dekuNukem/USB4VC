#!/bin/bash -e

install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/usb4vc"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/xpadneo"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/dkms-hid-nintendo"
install -v -o 1000 -g 1000 -d "${ROOTFS_DIR}/opt/joycond"

rsync -lr --exclude=.git --chown=1000:1000 "files/xpadneo/" "${ROOTFS_DIR}/opt/xpadneo/"
rsync -lr --exclude=.git --chown=1000:1000 "files/dkms-hid-nintendo/" "${ROOTFS_DIR}/opt/dkms-hid-nintendo/"
rsync -lr --exclude=.git --chown=1000:1000 "files/joycond/" "${ROOTFS_DIR}/opt/joycond/"

# create venv for usb4vc and install luma.oled
on_chroot << EOF
    python3 -m venv --system-site-packages /opt/usb4vc/venv

    source /opt/usb4vc/venv/bin/activate
        pip install evdev spidev serial luma.oled
        pip cache purge || echo "pip cache purge did nothing"
    deactivate

    chown -R 1000:1000 /opt/usb4vc/venv
EOF

# build and install nintendo hid, joycond
on_chroot << EOF
    pushd /opt/dkms-hid-nintendo
        dkms remove -m nintendo -v 3.2 --all || echo ""
        dkms add .

        # find the kernel version in the rootfs for dkms to target
        KERNEL_VERSIONS=(\$(ls -1 "/lib/modules/"))

        for kernel in "\${KERNEL_VERSIONS[@]}"; do
            if [ -d "/usr/src/linux-headers-\$kernel" ]; then
                dkms build nintendo -v 3.2 -k \$kernel
                dkms install nintendo -v 3.2 -k \$kernel
            fi
        done
    popd

    pushd /opt/joycond
        cmake .
        make install
        systemctl enable joycond
    popd
EOF
