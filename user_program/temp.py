while current_line_num < max_lines:
	this_line = file_content[current_line_num]

	print(current_line_num, len(this_line))
	print(this_line, end='')


	if this_line.startswith('I: '):
		while len(this_line) != 1:
			print(this_line)


	current_line_num += 1
