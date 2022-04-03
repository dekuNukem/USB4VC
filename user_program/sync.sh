# scp ./* pi@169.254.194.124:~/usb4vc/rpi_app

# sh sync.sh; ssh -t pi@169.254.194.124 "pkill python3;cd ~/usb4vc/rpi_app;python3 usb4vc_main.py"

scp ./* pi@169.254.194.124:~/usb4vc/rpi_app
# ssh -t pi@169.254.194.124 "pkill python3;cd ~/usb4vc/rpi_app;python3 usb4vc_main.py"
ssh -t pi@169.254.194.124 "pkill python3;cd ~/usb4vc/rpi_app;python3 usb4vc_check_update.py"

# ssh -t pi@169.254.194.124 "pkill python3;cd ~/usb4vc/rpi_app;python3 firmware_flasher.py /home/pi/usb4vc/firmware/PBFW_IBMPC_PBID1_V0_1_5.hex"
# ssh -t pi@169.254.194.124 "pkill python3;cd ~/usb4vc/rpi_app;python3 bb_tester.py"