import usb4vc_shared

default_map_dict = {}

for item in usb4vc_shared.gamepad_event_code_name_list:
	this_name = item
	this_value = usb4vc_shared.code_name_to_value_lookup.get(item)
	if this_value is None:
		continue
	default_map_dict[this_name] = {'code':f'IBM_GGP_BTN_{(this_value[0]%4) + 1}'}

print(default_map_dict)