echo 16 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio16/direction
echo rising > /sys/class/gpio/gpio16/edge