from PIL import Image

im = Image.open('ccc.png', 'r').convert('RGB')
width, height = im.size

if width < 128 or height < 128:
	print("Wrong size, should be exactly 128x128")
	exit()

# x: left 0, right 128
# y: top 0, bottom 128
pix = im.load()

curve_dict = {}

for horizontal_location in range(128):
	col_dict = {}
	for y in range(128):
		col_dict[sum(pix[horizontal_location,y])] = y
	# print(horizontal_location, 128 - col_dict[min(col_dict.keys())])
	curve_dict[horizontal_location] = 128 - col_dict[min(col_dict.keys())]
	if curve_dict[horizontal_location] < 0:
		curve_dict[horizontal_location] = 0
	if curve_dict[horizontal_location] > 127:
		curve_dict[horizontal_location] = 127

print(curve_dict)