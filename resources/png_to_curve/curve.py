from PIL import Image

im = Image.open('bitmap.png', 'r').convert('RGB')
width, height = im.size

# x: left 0, right width
# y: top 0, bottom height
pix = im.load()

curve_list = []

for x in range(width):
	col_list = []
	for y in range(height):
		col_list.append((y, sum(pix[x,y])))
	col_list = sorted(col_list, key=lambda x: x[1])
	curve_list.append((x, height - col_list[0][0]))


adjusted_list = [(int(x[0]/4), int((x[1]/255)*12000) + 500) for x in curve_list[::4]]
print(adjusted_list)
print([x[1] for x in adjusted_list])