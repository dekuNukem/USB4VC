import os

dev_path = '/proc/bus/input/devices'

if 'nt' in os.name:
	dev_path = 'devices.txt'

dev_file = open(dev_path)
file_content = dev_file.readlines()
dev_file.close()

current_line_num = 0
max_lines = len(file_content)

while current_line_num < max_lines:
	this_line = file_content[current_line_num]

	if this_line.startswith('I: '):
		print("-------- New block! --------")
		while len(this_line) != 1:
			current_line_num += 1
			if current_line_num >= max_lines:
				print("-------- EOF --------")
				break
			this_line = file_content[current_line_num]
			print(this_line)
		print("-------- Block end --------")
	
	current_line_num += 1
