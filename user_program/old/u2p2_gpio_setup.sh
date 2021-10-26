echo 131 > /sys/class/gpio/unexport
cat /proc/interrupts
echo 131 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio131/direction
echo rising > /sys/class/gpio/gpio131/edge
cat /proc/interrupts
