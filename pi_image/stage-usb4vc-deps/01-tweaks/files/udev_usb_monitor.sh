#!/usr/bin/env bash

pathtoname() {
    echo "$1" | awk -v FS== '/DEVNAME/ {print $2}'
}

pathtodevtype() {
    echo "$1" | awk -v FS== '/DEVTYPE/ {print $2}'
}

pathtodriver() {
    echo "$1" | awk -v FS== '/ID_USB_DRIVER/ {print $2}'
}

stdbuf -oL -- udevadm monitor --udev -s block | while read -r -- _ _ event devpath _; do
        if [ "$event" = add ]; then
            devinfo=$(udevadm info -p /sys/"$devpath")
            devname=$(pathtoname "$devinfo")
            devtype=$(pathtodevtype "$devinfo")
            devdriver=$(pathtodriver "$devinfo")

            if [ "$devdriver" = usb-storage -a "$devtype" = partition ]; then
                echo "Mounting $devdriver $devtype $devname"
                udisksctl mount --block-device "$devname" --no-user-interaction
            fi
        fi
done
