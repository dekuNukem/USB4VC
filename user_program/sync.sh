# scp ./* pi@192.168.1.65:~/usb4vc/rpi_app

# sh sync.sh; ssh -t pi@192.168.1.67 "pkill python3;cd ~/usb4vc/rpi_app;python3 usb4vc_main.py"

scp ./* pi@192.168.1.59:~/usb4vc/rpi_app
ssh -t pi@192.168.1.59 "pkill python3;cd ~/usb4vc/rpi_app;python3 usb4vc_main.py"