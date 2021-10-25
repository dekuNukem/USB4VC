import select

gpio_path = "/sys/class/gpio/gpio16/value"

gpio_file = open(gpio_path, 'rb')

epoll = select.epoll()
epoll.register(gpio_file, select.EPOLLIN | select.EPOLLET)

while 1:
    events = epoll.poll()
    for df, event_type in events:
        print(df, event_type)

epoll.unregister(gpio_file)
gpio_file.close()