SPI_MOSI_MAGIC = 0xde
SPI_MOSI_MSG_TYPE_SET_PROTOCOL = 2

RPI_APP_VERSION_TUPLE = (0, 0, 1)
set_protocl_spi_msg_template = [SPI_MOSI_MAGIC, 0, SPI_MOSI_MSG_TYPE_SET_PROTOCOL] + [0]*29

BTN_LEFT = 0x110
BTN_RIGHT = 0x111
BTN_MIDDLE = 0x112
BTN_SIDE = 0x113
BTN_EXTRA = 0x114
BTN_FORWARD = 0x115
BTN_BACK = 0x116
BTN_TASK = 0x117


REL_X = 0x00
REL_Y = 0x01
REL_Z = 0x02
REL_RX = 0x03
REL_RY = 0x04
REL_RZ = 0x05
REL_HWHEEL = 0x06
REL_DIAL = 0x07
REL_WHEEL = 0x08
REL_MISC = 0x09

KB_KEY_A = (30, 'pb_kb')
KB_KEY_B = (48, 'pb_kb')
KB_KEY_C = (46, 'pb_kb')
KB_KEY_D = (32, 'pb_kb')
KB_KEY_UP = (103, 'pb_kb')
KB_KEY_DOWN = (108, 'pb_kb')
KB_KEY_LEFT = (105, 'pb_kb')
KB_KEY_RIGHT = (106, 'pb_kb')
KB_KEY_LCTRL = (29, 'pb_kb')
MOUSE_BTN_LEFT = (BTN_LEFT, 'mouse_btn')
MOUSE_BTN_MIDDLE = (BTN_MIDDLE, 'mouse_btn')
MOUSE_BTN_RIGHT = (BTN_RIGHT, 'mouse_btn')
MOUSE_BTN_SIDE = (BTN_SIDE, 'mouse_btn')
MOUSE_BTN_EXTRA = (BTN_EXTRA, 'mouse_btn')
MOUSE_X = (REL_X, 'mouse_axes')
MOUSE_Y = (REL_Y, 'mouse_axes')
MOUSE_SCROLL = (REL_WHEEL, 'mouse_axes')
IBMPC_GGP_BTN_1 = ('IBMGGP_BTN_1', 'pb_gp_btn')
IBMPC_GGP_BTN_2 = ('IBMGGP_BTN_2', 'pb_gp_btn')
IBMPC_GGP_BTN_3 = ('IBMGGP_BTN_3', 'pb_gp_btn')
IBMPC_GGP_BTN_4 = ('IBMGGP_BTN_4', 'pb_gp_btn')
IBMPC_GGP_JS1_X = ('IBMGGP_JS1_X', 'pb_gp_axes')
IBMPC_GGP_JS1_Y = ('IBMGGP_JS1_Y', 'pb_gp_axes')
IBMPC_GGP_JS2_X = ('IBMGGP_JS2_X', 'pb_gp_axes')
IBMPC_GGP_JS2_Y = ('IBMGGP_JS2_Y', 'pb_gp_axes')

IBMPC_GGP_JS1_X_POS = ('IBMGGP_JS1_XP', 'pb_gp_half_axes')
IBMPC_GGP_JS1_X_NEG = ('IBMGGP_JS1_XN', 'pb_gp_half_axes')
IBMPC_GGP_JS1_Y_POS = ('IBMGGP_JS1_YP', 'pb_gp_half_axes')
IBMPC_GGP_JS1_Y_NEG = ('IBMGGP_JS1_YN', 'pb_gp_half_axes')

IBMPC_GGP_JS2_X_POS = ('IBMGGP_JS2_XP', 'pb_gp_half_axes')
IBMPC_GGP_JS2_X_NEG = ('IBMGGP_JS2_XN', 'pb_gp_half_axes')
IBMPC_GGP_JS2_Y_POS = ('IBMGGP_JS2_YP', 'pb_gp_half_axes')
IBMPC_GGP_JS2_Y_NEG = ('IBMGGP_JS2_YN', 'pb_gp_half_axes')
