import select

gpio_path = "/sys/class/gpio/gpio131/value"

gpio_file = open(gpio_path, 'rb')

epoll = select.epoll()
epoll.register(gpio_file, select.EPOLLET)

while 1:
    events = epoll.poll(timeout=0.2)
    for df, event_type in events:
        print(df, event_type)
    print('here')

epoll.unregister(gpio_file)
gpio_file.close()