import os
import sys
import time
import spidev
import RPi.GPIO as GPIO

PBOARD_RESET_PIN = 25
PBOARD_BOOT0_PIN = 12
SLAVE_REQ_PIN = 16
GPIO.setmode(GPIO.BCM)
GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
GPIO.setup(PBOARD_BOOT0_PIN, GPIO.IN)
GPIO.setup(SLAVE_REQ_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def enter_dfu():
    # RESET LOW: Enter reset
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    # BOOT0 HIGH: Boot into DFU mode
    GPIO.setup(PBOARD_BOOT0_PIN, GPIO.OUT)
    GPIO.output(PBOARD_BOOT0_PIN, GPIO.HIGH)
    time.sleep(0.05)
    # Release RESET, BOOT0 still HIGH, STM32 now in DFU mode
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(1)

def exit_dfu():
    # Release BOOT0
    GPIO.setup(PBOARD_BOOT0_PIN, GPIO.IN)
    # Activate RESET
    GPIO.setup(PBOARD_RESET_PIN, GPIO.OUT)
    GPIO.output(PBOARD_RESET_PIN, GPIO.LOW)
    time.sleep(0.05)
    # Release RESET, BOOT0 is LOW, STM32 boots in normal mode
    GPIO.setup(PBOARD_RESET_PIN, GPIO.IN)
    time.sleep(0.2)

def flash_firmware(fw_path):
    print(f"----------------- Flashing: {fw_path.split('/')[-1]} -----------------")
    enter_dfu()
    exit_code = os.system(f'sudo stm32flash -w {fw_path} -a 0x3b /dev/i2c-1') >> 8
    exit_dfu()
    if exit_code != 0:
        for x in range(5):
            print("!!!!!!!!!!!!!!!!! TEST FLASH FAILED !!!!!!!!!!!!!!!!!")
        exit()

if(len(sys.argv) < 2):
    print(sys.argv[0] + ' hex_file')
    exit()

os.system("clear")

pcard_spi = spidev.SpiDev(0, 0)
pcard_spi.max_speed_hz = 2000000

payload_fw_path = sys.argv[1]

flash_firmware(payload_fw_path)

SPI_MOSI_MAGIC = 0xde
SPI_MOSI_MSG_TYPE_INFO_REQUEST = 1

nop_spi_msg_template = [SPI_MOSI_MAGIC] + [0]*31
info_request_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_INFO_REQUEST] + [0]*29

this_msg = list(info_request_spi_msg_template)
pcard_spi.xfer(this_msg)
time.sleep(0.1)
response = pcard_spi.xfer(list(nop_spi_msg_template))
time.sleep(0.1)
print(response)

if response[0] != 205:
    for x in range(5):
        print("!!!!!!!!!!!!!!!!! WRONG RESPONSE !!!!!!!!!!!!!!!!!")
else:
    print("----------------- TEST OK! -----------------")
