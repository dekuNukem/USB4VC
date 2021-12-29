"""----------bluetooth---------

https://github.com/oscaracena/pygattlib
sudo apt install pkg-config libboost-python-dev libboost-thread-dev libbluetooth-dev libglib2.0-dev python-dev
pip install gattlib


sudo apt-get install bluetooth bluez libbluetooth-dev
sudo pip3 install pybluez
https://raspberrypi.stackexchange.com/questions/114149/bluetooth-library-for-raspberry-pi
https://raspberrypi.stackexchange.com/questions/45246/bluetooth-import-for-python-raspberry-pi-3

this_device = ooopened_device_dict[key]
        print(source_code, source_type, target_info)"""

def raw_input_event_worker():
    mouse_status_dict = {}
    gamepad_status_dict = {}
    print("raw_input_event_worker started")
    while 1:
        time.sleep(1)
# ----------------- KEYBOARD PARSING -----------------
        for key in list(keyboard_opened_device_dict):
            try:
                data = keyboard_opened_device_dict[key][0].read(16)
            except OSError:
                keyboard_opened_device_dict[key][0].close()
                del keyboard_opened_device_dict[key]
                print("keyboard disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            if data[0] == EV_KEY:
                pcard_spi.xfer(make_keyboard_spi_packet(data, keyboard_opened_device_dict[key][1]))

# ----------------- MOUSE PARSING -----------------

        for key in list(mouse_opened_device_dict):
            try:
                data = mouse_opened_device_dict[key][0].read(16)
            except OSError:
                mouse_opened_device_dict[key][0].close()
                del mouse_opened_device_dict[key]
                print("mouse disappeared:", key)
                continue
            if data is None:
                continue
            """
            0 - 1 event_type
            2 - 3 key code
            4 - 7 key status
            """
            data = list(data[8:])
            # mouse movement and scrolling
            # buffer those values until a SYNC event
            if data[0] == EV_REL:
                if data[2] == REL_X:
                    rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
                if data[2] == REL_Y:
                    rawy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawy = int(rawy * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["y"] = list(rawy.to_bytes(2, byteorder='little'))
                if data[2] == REL_WHEEL:
                    mouse_status_dict["scroll"] = data[4:6]

            # mouse button pressed, send it out immediately
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                if 0x110 <= key_code <= 0x117:
                    mouse_status_dict[key_code] = data[4]
                    mouse_status_dict['x'] = [0, 0]
                    mouse_status_dict['y'] = [0, 0]
                    mouse_status_dict['scroll'] = [0, 0]
                    to_transfer = make_mouse_spi_packet(mouse_status_dict, mouse_opened_device_dict[key][1])
                    pcard_spi.xfer(to_transfer)
                """
                Logitech unifying receiver identifies itself as a mouse,
                so need to handle keyboard presses here too for compatibility
                """
                if 0x1 <= key_code <= 127:
                    pcard_spi.xfer(make_keyboard_spi_packet(data, mouse_opened_device_dict[key][1]))

            # SYNC event happened, send out an update
            if data[0] == EV_SYN and data[2] == SYN_REPORT and len(mouse_status_dict) > 0:
                to_transfer = make_mouse_spi_packet(mouse_status_dict, mouse_opened_device_dict[key][1])
                pcard_spi.xfer(to_transfer)
                mouse_status_dict['x'] = [0, 0]
                mouse_status_dict['y'] = [0, 0]
                mouse_status_dict['scroll'] = [0, 0]

# ----------------- GAMEPAD PARSING -----------------
        for key in list(gamepad_opened_device_dict):
            try:
                data = gamepad_opened_device_dict[key][0].read(16)
            except OSError:
                gamepad_opened_device_dict[key][0].close()
                del gamepad_opened_device_dict[key]
                print("gamepad disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            gamepad_id = gamepad_opened_device_dict[key][1]
            if gamepad_id not in gamepad_status_dict:
                gamepad_status_dict[gamepad_id] = {}
            # gamepad button presses, send out immediately
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                if not (GP_BTN_SOUTH <= key_code <= GP_BTN_THUMBR):
                    continue
                gamepad_status_dict[gamepad_id][key_code] = data[4]
            # joystick / analogue trigger movements, cache until next SYNC event
            if data[0] == EV_ABS:
                abs_axes = data[3] * 256 + data[2]
                abs_value = int.from_bytes(data[4:8], byteorder='little', signed=True)
                gamepad_status_dict[gamepad_id][abs_axes] = abs_value
            # SYNC report, update now
            if data[0] == EV_SYN and data[2] == SYN_REPORT:
                gp_to_transfer, kb_to_transfer, mouse_to_transfer = make_gamepad_spi_packet(gamepad_status_dict, gamepad_id, gamepad_opened_device_dict[key][2])
                pcard_spi.xfer(gp_to_transfer)
                if kb_to_transfer is not None:
                    time.sleep(0.001)
                    pcard_spi.xfer(kb_to_transfer)
                if mouse_to_transfer is not None:
                    time.sleep(0.001)
                    pcard_spi.xfer(mouse_to_transfer)

# ----------------- PBOARD INTERRUPT -----------------
        if GPIO.event_detected(SLAVE_REQ_PIN):
            slave_result = None
            for x in range(2):
                slave_result = pcard_spi.xfer(make_spi_msg_ack())
            print(int(time.time()), slave_result)
            if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_TYPE_KB_LED_REQUEST:
                try:
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                    change_kb_led(slave_result[3], slave_result[4], slave_result[5])
                except Exception as e:
                    print('change_kb_led exception:', e)



def usb_device_scan_worker():
    print("usb_device_scan_worker started")
    while 1:
        time.sleep(0.75)
        try:
            device_file_list = os.listdir(input_device_path)
        except FileNotFoundError:
            print("No input devices found")
            continue
        except Exception as e:
            print('list input device exception:', e)
            continue

        mouse_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-mouse' in x]
        keyboard_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-kbd' in x]
        gamepad_list = [os.path.join(input_device_path, x) for x in device_file_list if 'event-joystick' in x]

        for item in keyboard_list:
            if item not in keyboard_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    keyboard_opened_device_dict[item] = (this_file, device_index)
                    print("opened keyboard", keyboard_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("keyboard open exception:", e)
                    continue

        for item in mouse_list:
            if item not in mouse_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    mouse_opened_device_dict[item] = (this_file, device_index)
                    print("opened mouse", mouse_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("mouse open exception:", e)
                    continue

        for item in gamepad_list:
            if item not in gamepad_opened_device_dict:
                try:
                    this_file = open(item, "rb")
                    os.set_blocking(this_file.fileno(), False)
                    device_index = 0
                    try:
                        device_index = sum([int(x) for x in item.split(':')[1].split('.')][:2])
                    except:
                        pass
                    gamepad_info = subprocess.getoutput("timeout 0.5 evtest " + str(item))
                    gamepad_opened_device_dict[item] = (this_file, device_index, parse_evtest_info(gamepad_info))
                    print("opened gamepad", gamepad_opened_device_dict[item][1], ':' , item)
                except Exception as e:
                    print("gamepad open exception:", e)
                    continue

def parse_evtest_info(input_str):
    input_str = input_str.replace('\r', '').split('Event type 3 (EV_ABS)\n')[-1].split('Event type')[0]
    current_axis_code = None
    info_dict = {}
    for line in input_str.split('\n'):
        line = line.strip().replace('\t', '')
        if " (ABS_" in line:
            current_axis_code = int(line.split('Event code ')[1].split(' (ABS_')[0])
            info_dict[current_axis_code] = {}
            continue
        if current_axis_code is None or len(line) < 3:
            continue
        line_split = line.split(' ')
        info_dict[current_axis_code][line_split[0].lower()] = int(line_split[-1])
    return info_dict

    # print(time.time(), '-----------')

    print(gp_spi_msg)
    print(kb_spi_msg)
    print(mouse_spi_msg)
    print('-------')
            # print('=====>',from_code, from_type, to_code, to_type)
# print('kb', curr_kb_output)
    # print('prev', prev_gp_output)
    # print('curr', curr_gp_output)
    print("mouse", curr_mouse_output)
rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
    'mapping':
    {
        # buttons to buttons
        USBGP_BTN_X: IBMPC_GGP_JS1_X_NEG,
        USBGP_BTN_B: IBMPC_GGP_JS1_X_POS,
        USBGP_BTN_Y: IBMPC_GGP_JS1_Y_POS,
        USBGP_BTN_A: IBMPC_GGP_JS1_Y_NEG,
        # analog stick to analog stick
        USBGP_ABS_X: IBMPC_GGP_JS1_X,
        USBGP_ABS_Y: IBMPC_GGP_JS1_Y,
        USBGP_ABS_HAT0X: IBMPC_GGP_JS1_X,
        USBGP_ABS_HAT0Y: IBMPC_GGP_JS1_Y,
        # buttons to analog stick
        USBGP_BTN_TL: IBMPC_GGP_BTN_1,
        USBGP_BTN_TR: IBMPC_GGP_BTN_2,
        # buttons to keyboard key
        USBGP_BTN_START: KB_KEY_A,
        USBGP_BTN_SELECT: KB_KEY_B,
        USBGP_BTN_TL: IBMPC_GGP_JS1_X_NEG,
        USBGP_BTN_TR: IBMPC_GGP_JS1_X_POS,
        # analog stick to keyboard key
        USBGP_ABS_RX: {'type':'pb_kb', 'pos_key':KB_KEY_RIGHT[0], 'neg_key':KB_KEY_LEFT[0]},
        USBGP_ABS_RY: {'type':'pb_kb', 'pos_key':KB_KEY_DOWN[0], 'neg_key':KB_KEY_UP[0]},
    }
            print("HERE")
            print('=====>',from_code, from_type, to_code, to_type)
if page == 1:
                with canvas(device) as draw:
                    draw.text((0, 0), f"USB Keyboard: {len(usb4vc_usb_scan.keyboard_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 10), f"USB Mouse:    {len(usb4vc_usb_scan.mouse_opened_device_dict)}", font=font_regular, fill="white")
                    draw.text((0, 20), f"USB Gamepad:  {len(usb4vc_usb_scan.gamepad_opened_device_dict)}", font=font_regular, fill="white")
    def find_furthest_from_midpoint(this_set):
        print("vvvvvvvvvv")
        curr_best = None
        diff_max = 0
        for item in this_set:
            print(item)
            this_diff = abs(item - 127)
            if this_diff >= diff_max:
                diff_max = this_diff
                curr_best = item
        print(f"^^^^{curr_best}^^^^^")
def make_generic_gamepad_spi_packet(gp_status_dict, gp_id, axes_info, mapping_info):
    button_status = set()
    axes_status = set()
    kb_key_status = set()
    for key in mapping_info['mapping']:
        # print(key, mapping_info['mapping'][key])
        from_code = key[0]
        from_type = key[1]
        to_code = mapping_info['mapping'][key][0]
        to_type = mapping_info['mapping'][key][1]
        if from_code not in gp_status_dict[gp_id]:
            continue
        # button to button
        # add them into a set first, then sort by timestamp, so latest action will have pirority
        if from_type == 'usb_gp_btn' and to_type == 'pb_gp_btn':
            if to_code == 'ibm_ggp_btn1':
                button_status.add((IBMPC_GGP_BUTTON_1, gp_status_dict[gp_id][from_code][0], gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_btn2':
                button_status.add((IBMPC_GGP_BUTTON_2, gp_status_dict[gp_id][from_code][0], gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_btn3':
                button_status.add((IBMPC_GGP_BUTTON_3, gp_status_dict[gp_id][from_code][0], gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_btn4':
                button_status.add((IBMPC_GGP_BUTTON_4, gp_status_dict[gp_id][from_code][0], gp_status_dict[gp_id][from_code][1]))
        # analog to analog
        if from_type == 'usb_gp_axes' and to_type == 'pb_gp_axes':
            if to_code == 'ibm_ggp_js1x':
                axes_status.add((IBMPC_GGP_JS1_X, clamp_to_8bit(gp_status_dict[gp_id][from_code][0], axes_info, from_code), gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_js1y':
                axes_status.add((IBMPC_GGP_JS1_Y, clamp_to_8bit(gp_status_dict[gp_id][from_code][0], axes_info, from_code), gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_js2x':
                axes_status.add((IBMPC_GGP_JS2_X, clamp_to_8bit(gp_status_dict[gp_id][from_code][0], axes_info, from_code), gp_status_dict[gp_id][from_code][1]))
            if to_code == 'ibm_ggp_js2y':
                axes_status.add((IBMPC_GGP_JS2_Y, clamp_to_8bit(gp_status_dict[gp_id][from_code][0], axes_info, from_code), gp_status_dict[gp_id][from_code][1]))
        # button to analog
        if from_type == 'usb_gp_btn' and to_type == 'pb_gp_half_axes':
            axis_value = 127
            if gp_status_dict[gp_id][from_code][0]:
                if to_code.endswith('p'):
                    axis_value = 255
                elif to_code.endswith('n'):
                    axis_value = 0
            # button not pressed
            if gp_status_dict[gp_id][from_code][0] == 0:
                # print(key, mapping_info['mapping'][key])
                opposite_axis = None
                if to_code[-1] == 'n':
                    opposite_axis = str(to_code)[:-1] + 'p'
                elif to_code[-1] == 'p':
                    opposite_axis = str(to_code)[:-1] + 'n'
                if opposite_axis is not None:
                    for kkkk in mapping_info['mapping']:
                        if mapping_info['mapping'][kkkk][0] == opposite_axis:
                            print("FOUND IT")
                            print(kkkk, mapping_info['mapping'][kkkk])
                            if kkkk[0] in gp_status_dict and gp_status_dict[kkkk[0]]:
                                if mapping_info['mapping'][kkkk][0].endswith('p'):
                                    axis_value = 255
                                elif mapping_info['mapping'][kkkk][0].endswith('n'):
                                    axis_value = 0

            if to_code.startswith('ibm_ggp_js1x'):
                axes_status.add((IBMPC_GGP_JS1_X, axis_value, gp_status_dict[gp_id][from_code][1]))
            if to_code.startswith('ibm_ggp_js1y'):
                axes_status.add((IBMPC_GGP_JS1_Y, axis_value, gp_status_dict[gp_id][from_code][1]))
            if to_code.startswith('ibm_ggp_js2x'):
                axes_status.add((IBMPC_GGP_JS2_X, axis_value, gp_status_dict[gp_id][from_code][1]))
            if to_code.startswith('ibm_ggp_js2y'):
                axes_status.add((IBMPC_GGP_JS2_Y, axis_value, gp_status_dict[gp_id][from_code][1]))
        # button to keyboard key
        if from_type == 'usb_gp_btn' and to_type == 'pb_kb':
            kb_key_status.add((to_code, gp_status_dict[gp_id][from_code][0], gp_status_dict[gp_id][from_code][1]))
        # analog to keyboard key
        # analog to mouse axes
        # button to mouse buttons


    button_status = sorted(button_status, key=lambda x: x[2], reverse=True)
    axes_status = sorted(axes_status, key=lambda x: x[2], reverse=True)
    kb_key_status = sorted(kb_key_status, key=lambda x: x[2], reverse=True)
    gp_spi_msg = list(gamepad_event_ibm_ggp_spi_msg_template)
    gp_spi_msg[3] = gp_id;
    already_added = set()
    for item in axes_status:
        this_code = item[0]
        this_value = item[1]
        # only send event with most recent timestamp to protocol board
        if this_code in already_added:
            continue
        if this_code == IBMPC_GGP_JS1_X:
            gp_spi_msg[8] = this_value
        if this_code == IBMPC_GGP_JS1_Y:
            gp_spi_msg[9] = this_value
        if this_code == IBMPC_GGP_JS2_X:
            gp_spi_msg[10] = this_value
        if this_code == IBMPC_GGP_JS2_Y:
            gp_spi_msg[11] = this_value
        already_added.add(this_code)
    for item in button_status:
        this_code = item[0]
        this_value = item[1]
        if this_code in already_added:
            continue
        if this_code == IBMPC_GGP_BUTTON_1:
            gp_spi_msg[4] = this_value
        if this_code == IBMPC_GGP_BUTTON_2:
            gp_spi_msg[5] = this_value
        if this_code == IBMPC_GGP_BUTTON_3:
            gp_spi_msg[6] = this_value
        if this_code == IBMPC_GGP_BUTTON_4:
            gp_spi_msg[7] = this_value
        already_added.add(this_code)

    kb_result = None
    if len(kb_key_status) > 0:
        kb_result = list(keyboard_event_spi_msg_template)
        kb_result[4] = kb_key_status[0][0]
        kb_result[6] = kb_key_status[0][1]

    print(gp_spi_msg, kb_result)
    return gp_spi_msg, kb_result
# ----------------------------- 

    for item in kb_key_status:
        this_code = item[0]
        this_value = item[1]
        if this_code in already_added:
            print("pass")
            continue
        print(item, this_value)
        kb_result = list(keyboard_event_spi_msg_template)
        kb_result[4] = this_code
        kb_result[6] = this_value
        already_added.add(this_code)
        print('kbr', kb_result)
    # print(axes_status)
# ----------------- GAMEPAD PARSING -----------------
        for key in list(gamepad_opened_device_dict):
            try:
                data = gamepad_opened_device_dict[key][0].read(16)
            except OSError:
                gamepad_opened_device_dict[key][0].close()
                del gamepad_opened_device_dict[key]
                print("gamepad disappeared:", key)
                continue
            if data is None:
                continue
            data = list(data[8:])
            gamepad_id = gamepad_opened_device_dict[key][1]
            if gamepad_id not in gamepad_status_dict:
                gamepad_status_dict[gamepad_id] = {}
            # gamepad button presses, send out immediately
            if data[0] == EV_KEY:
                key_code = data[3] * 256 + data[2]
                if not (GP_BTN_SOUTH <= key_code <= GP_BTN_THUMBR):
                    continue
                gamepad_status_dict[gamepad_id][key_code] = (data[4], time.time_ns())
                gp_to_transfer, kb_to_transfer = make_gamepad_spi_packet(gamepad_status_dict, gamepad_id, gamepad_opened_device_dict[key][2])
                last_ibm_ggp_spi_message = list(gp_to_transfer)
                pcard_spi.xfer(gp_to_transfer)
                if kb_to_transfer is not None:
                    pcard_spi.xfer(kb_to_transfer)
                continue
            # joystick / analogue trigger movements, cache until next SYNC event
            if data[0] == EV_ABS:
                abs_axes = data[3] * 256 + data[2]
                abs_value = int.from_bytes(data[4:8], byteorder='little', signed=True)
                gamepad_status_dict[gamepad_id][abs_axes] = (abs_value, time.time_ns())
            # SYNC report, update now
            if data[0] == EV_SYN and data[2] == SYN_REPORT:
                gp_to_transfer, kb_to_transfer = make_gamepad_spi_packet(gamepad_status_dict, gamepad_id, gamepad_opened_device_dict[key][2])
                last_ibm_ggp_spi_message = list(gp_to_transfer)
                pcard_spi.xfer(gp_to_transfer)
def make_gamepad_spi_packet(gp_status_dict, gp_id, axes_info):
    current_gamepad_protocol = usb4vc_ui.get_gamepad_protocol()
    print(current_gamepad_protocol)
    if current_gamepad_protocol['pid'] == PID_GENERIC_GAMEPORT_GAMEPAD:
        result = list(gamepad_event_mapped_spi_msg_template)
        result[3] = gp_id;
        result[4] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_SOUTH)
        result[5] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_EAST)
        result[6] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_WEST)
        result[7] = dict_get_if_exist(gp_status_dict[gp_id], GP_BTN_NORTH)

        result[8] = dict_get_if_exist(gp_status_dict[gp_id], ABS_X)
        if ABS_X in axes_info and 'max' in axes_info[ABS_X]:
            result[8] = int(result[8] / (axes_info[ABS_X]['max'] / 127)) + 127

        result[9] = dict_get_if_exist(gp_status_dict[gp_id], ABS_Y)
        if ABS_Y in axes_info and 'max' in axes_info[ABS_Y]:
            result[9] = int(result[9] / (axes_info[ABS_Y]['max'] / 127)) + 127

        result[10] = dict_get_if_exist(gp_status_dict[gp_id], ABS_RX)
        if ABS_RX in axes_info and 'max' in axes_info[ABS_RX]:
            result[10] = int(result[10] / (axes_info[ABS_RX]['max'] / 127)) + 127

        result[11] = dict_get_if_exist(gp_status_dict[gp_id], ABS_RY)
        if ABS_RY in axes_info and 'max' in axes_info[ABS_RY]:
            result[11] = int(result[11] / (axes_info[ABS_RY]['max'] / 127)) + 127
        return result
    return list(nop_spi_msg_template)



def send_spi(self):
        protocol_bytes = []
        for index, item in enumerate(self.kb_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_keyboard_protocol_index:
                protocol_bytes[-1] |= 0x80

        for index, item in enumerate(self.mouse_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_mouse_protocol_index:
                protocol_bytes[-1] |= 0x80

        for index, item in enumerate(self.gamepad_opts):
            if item == PROTOCOL_OFF:
                continue
            protocol_bytes.append(item['pid'] & 0x7f)
            if index == self.current_gamepad_protocol_index:
                protocol_bytes[-1] |= 0x80

        # protocol_bytes = list(set(protocol_bytes))
        this_msg = list(usb4vc_shared.set_protocl_spi_msg_template)
        this_msg[3:3+len(protocol_bytes)] = protocol_bytes

        if this_msg == self.last_spi_message:
            print("SPI: no need to send")
            return
        print("set_protocol:", [hex(x) for x in this_msg])
        usb4vc_usb_scan.set_protocol(this_msg)
        print('new status:', [hex(x) for x in usb4vc_usb_scan.get_pboard_info()])
        self.last_spi_message = list(this_msg)

def load_custom_profiles(profile_list):
    for item in profile_list:
        if item['']

if data[0] == EV_REL:
                if data[2] == REL_X:
                    rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["x"] = list(rawx.to_bytes(2, byteorder='little'))
                if data[2] == REL_Y:
                    rawy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    rawy = int(rawy * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    mouse_status_dict["y"] = list(rawy.to_bytes(2, byteorder='little'))
                if data[2] == REL_WHEEL:
                    mouse_status_dict["scroll"] = data[4:6]
                # print(usb4vc_ui.get_mouse_sensitivity(), mouse_status_dict)


rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    if usb4vc_ui.get_mouse_sensitivity() != 1:
                        rawx = int(rawx * usb4vc_ui.get_mouse_sensitivity()) & 0xffff
                    print(rawx, adjusted, adjusted.to_bytes(2, byteorder='little'))
                    mouse_status_dict["x"] = data[4:6]

                    config load failed! dictionary keys changed during iteration

                    mouse_status_dict["x"] = data[4:6]
                    rawx = int.from_bytes(data[4:6], byteorder='little', signed=True)
                    adjusted = int(rawx * 1.33) & 0xffff

                    print(data)
                    print(rawx, adjusted, list(adjusted.to_bytes(2, byteorder='little')))

                this_msg = list(usb4vc_shared.set_protocl_spi_msg_template)
                start_idx = 3
                end_idx = start_idx

                for index, item in enumerate(self.kb_opts):
                    if item == PROTOCOL_OFF:
                        continue
                    this_msg[start_idx + index] = item['pid'] & 0x7f
                    if index == self.current_keyboard_protocol_index:
                        this_msg[start_idx + index] |= 0x80
                    end_idx = start_idx + index

                start_idx = end_idx

                for index, item in enumerate(self.mouse_opts):
                    if item == PROTOCOL_OFF:
                        continue
                    this_msg[start_idx + index] = item['pid'] & 0x7f
                    if index == self.current_mouse_protocol_index:
                        this_msg[start_idx + index] |= 0x80
                    end_idx = start_idx + index

                start_idx = end_idx

                for index, item in enumerate(self.gamepad_opts):
                    if item == PROTOCOL_OFF:
                        continue
                    this_msg[start_idx + index] = item['pid'] & 0x7f
                    if index == self.current_gamepad_protocol_index:
                        this_msg[start_idx + index] |= 0x80

                print(this_msg)

                if self.kb_opts[self.current_keyboard_protocol_index]['pid'] == 0:
                    
def int_to_bytes(number: int) -> bytes:
    return number.to_bytes(length=(8 + (number + (number < 0)).bit_length()) // 8, byteorder='big', signed=True)

                for index, item in enumerate(self.kb_opts):
                    print(index, item)

                # for index, item in enumerate(self.mouse_opts):
                #     print(index, item)

                # for index, item in enumerate(self.gamepad_opts):
                #     print(index, item)

                print('000000')


class usb4vc_level(object):
    def __init__(self):
        super(usb4vc_level, self).__init__()
        self.max_pages = 3
        self.current_page = 0
def print_welcome_screen(version_tuple):
    with canvas(device) as draw:
        oled_print_centered("USB4VC", font_large, 0, draw)
        oled_print_centered(f"V{version_tuple[0]}.{version_tuple[1]}.{version_tuple[2]} dekuNukem", font_regular, 20, draw)

def oled_clear():
    device.clear()

import queue
oled_display_queue = queue.PriorityQueue()
       
        
# while 1:
#   with oled_canvas as ocan:
#       ocan.rectangle((0, 0, device.width-1, device.height-1), outline=0, fill=0)
#       oled_print_centered(f"{int(time.time())}", font_large, 0, ocan)
#   time.sleep(1)
print(self.current_level, self.current_page)
    # device.contrast(1)
# persistent canvas, will need to clear areas before drawing new stuff
# oled_canvas = canvas(device)
# while 1:
#     with canvas(device) as draw:
#         # draw.rectangle(device.bounding_box, outline="white", fill="black")
        
#         # regular font test
#         draw.text((0, 0), "123456789012345678901234567890", font=font_regular, fill="white")
#         draw.text((0, 10), "ABCDEFGHIJ", font=font_regular, fill="white")
#         draw.text((0, 20), "abcdefghij", font=font_regular, fill="white")

#         # medium font test
#         # draw.text((0, 0), "123456789012345678901234567890", font=font_medium, fill="white")
#         # draw.text((0, 15), "123456789012345678901234567890", font=font_medium, fill="white")

#         # large font test
#         # draw.text((0, 0), "123456789012345678901234567890", font=font_large, fill="white")
#         # draw.text((0, 20), "123456789012345678901234567890", font=font_regular, fill="white")
#     time.sleep(1)
# ------------


    # print(gp_status_dict[gp_id], axes_info)

    # for key in gp_status_dict[gp_id]:
    #     print(key, gp_status_dict[gp_id][key], axes_info[key])
    # print('--')
                # print(data)
                # print(hex(abs_axes), abs_value)
                # print()

           # print(data)
            # if data[0] == EV_KEY or data[0] == EV_REL:
            #     to_transfer = mouse_spi_msg_template + data + [0]*20
            #     to_transfer[3] = mouse_opened_device_dict[key][1]
            #     spi.xfer(to_transfer)


                # print(time.time_ns(), 'sent')
                # print(key)
                # print(to_transfer)
                # print('----')
                # print(time.time_ns(), 'sent')
                # print(key)
                # print(to_transfer)
                # print('----')
last_spi_tx = 0
def spi_xfer_with_speedlimit(data):
    spi.xfer(data)
    # global last_spi_tx
    # while time.time_ns() - last_spi_tx <= 5000000:
    #     time.sleep(0.001)
    # spi.xfer(data)
    # last_spi_tx = time.time_ns()

while 1:
    data = list(fff.read(48))
    # mouse_movement_x = None
    # mouse_movement_y = None
    # mouse_wheel_movement_regular = None
    # mouse_wheel_movement_hi_res = None
    # mouse_button_event = None
    # packet1 = data[:16]
    # packet2 = data[16:32]
    # packet3 = data[32:]
    # print(len(packet1), len(packet2), len(packet3))

    for x in range(3):
        this_packet = data[x*16:x*16+16]
        print(this_packet[8:])
    print('----')
    # data = list(data[8:])
    # if data[0] == EV_KEY:
    #     print(time.time(), data)
    #     print('--------')
    # if data[0] == EV_REL:
    #     print(time.time(), data)
    #     print('--------')
    # spi.xfer(data)
    spi.xfer(data)

    # if data[0] == EV_KEY or data[0] == EV_REL:
    #     spi.xfer(data)
    

fff = open(sys.argv[1], "rb" )
while 1:
    data = fff.read(48)
    # data = list(data[8:])
    # if data[0] == EV_KEY:
    #     print(time.time(), data)
    #     print('--------')
    # if data[0] == EV_REL:
    #     print(time.time(), data)
    #     print('--------')
    # spi.xfer(data)
    spi.xfer(data)

    # if data[0] == EV_KEY or data[0] == EV_REL:
    #     spi.xfer(data)
    


    
    for item in scrolllock_list:
        if ps2kb_led_byte & 0x1:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

    for item in numlock_list:
        if ps2kb_led_byte & 0x2:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

    for item in capslock_list:
        if ps2kb_led_byte & 0x4:
            os.system("sudo bash -c 'echo 1 > " + os.path.join(item, 'brightness') + "'")
        else:
            os.system("sudo bash -c 'echo 0 > " + os.path.join(item, 'brightness') + "'")

if slave_result[SPI_BUF_INDEX_MAGIC] == SPI_MISO_MAGIC and slave_result[SPI_BUF_INDEX_MSG_TYPE] == SPI_MISO_MSG_KB_LED_REQ:
                    print("good!", slave_result[3])
                    for x in range(2):
                        change_kb_led(slave_result[3])
                        # time.sleep(0.1)
filler = [0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3]
                # testttt = [0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f]
                # time.sleep(0.01)

                    # change_kb_led(slave_result[3])
                    # if slave_result[3] & 0x1:
                    #     print("SCROLL LOCK ON")
                    # if slave_result[3] & 0x2:
                    #     print("NUM LOCK ON")
                    # if slave_result[3] & 0x4:
                    #     print("CAP LOCK ON")
                    # print('----')
while current_line_num < max_lines:
	this_line = file_content[current_line_num]

	print(current_line_num, len(this_line))
	print(this_line, end='')


	if this_line.startswith('I: '):
		while len(this_line) != 1:
			print(this_line)


	current_line_num += 1

FORMAT = '4IHHI'
EVENT_SIZE = struct.calcsize(FORMAT)
print(EVENT_SIZE)

def keyboard_worker():
    print("keyboard_thread started")
    while 1:
        data = fff.read(24)
        # (tv_sec, tv_usec, type, code, value) = struct.unpack('4IHHI', data)
        print(struct.unpack('4IHHI', data))

        # data = list(data)
        # spi.xfer(data)
 

 import sys
import libevdev

fd = open("/dev/input/event0", "rb" )

d = libevdev.Device(fd)
if not d.has(libevdev.EV_KEY.BTN_LEFT):
     print('This does not look like a mouse device')
     sys.exit(0)


# Loop indefinitely while pulling the currently available events off
# the file descriptor
while True:
    for e in d.events():
        if not e.matches(libevdev.EV_KEY):
            continue

        if e.matches(libevdev.EV_KEY.BTN_LEFT):
            print('Left button event')
        elif e.matches(libevdev.EV_KEY.BTN_RIGHT):
            print('Right button event')

print(struct.unpack('4IHHI', data))


data = list(fff.read(16)[8:])
        # https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/input-event-codes.h#L38
        if data[0] == 1: # Keys
            spi.xfer(data)
            # print(data)
            # print('----')

# example /dev/input/by-path/platform-3f980000.usb-usb-0:1.2.2:1.0-event-kbd
                    # kb_index = int(item.split(':')[1].split('.')[1])


while 1:
    with canvas(device) as draw:
        # draw.rectangle(device.bounding_box, outline="white", fill="black")
        
        # regular font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_regular, fill="white")
        # draw.text((0, 10), "ABCDEFGHIJ", font=font_regular, fill="white")
        # draw.text((0, 20), "abcdefghij", font=font_regular, fill="white")

        # medium font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_medium, fill="white")
        # draw.text((0, 15), "123456789012345678901234567890", font=font_medium, fill="white")

        # large font test
        # draw.text((0, 0), "123456789012345678901234567890", font=font_large, fill="white")
        # draw.text((0, 20), "123456789012345678901234567890", font=font_regular, fill="white")
    time.sleep(1)


