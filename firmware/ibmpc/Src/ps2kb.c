#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "ps2kb.h"
#include "delay_us.h"

#define CLKHALF 18
#define CLKFULL 36
#define BYTEWAIT 700
#define BYTEWAIT_END 200
#define PS2KB_BUS_TIMEOUT_MS 30
#define CODE_UNUSED 0xff
#define PS2KB_WRITE_DEFAULT_TIMEOUT_MS 20

#define LINUX_KEYCODE_TO_PS2_SCANCODE_SET2_SINGLE_SIZE 89
#define LINUX_KEYCODE_TO_PS2_SCANCODE_SET2_SPECIAL_SIZE 32
#define LINUX_KEYCODE_TO_PS2_SCANCODE_SET3_SIZE 195

const uint8_t linux_keycode_to_ps3_scancode_lookup_codeset3[LINUX_KEYCODE_TO_PS2_SCANCODE_SET3_SIZE] = 
{
  CODE_UNUSED, // KEY_RESERVED    0
  0x8, // KEY_ESC    1
  0x16, // KEY_1    2
  0x1e, // KEY_2    3
  0x26, // KEY_3    4
  0x25, // KEY_4    5
  0x2e, // KEY_5    6
  0x36, // KEY_6    7
  0x3d, // KEY_7    8
  0x3e, // KEY_8    9
  0x46, // KEY_9    10
  0x45, // KEY_0    11
  0x4e, // KEY_MINUS    12
  0x55, // KEY_EQUAL    13
  0x66, // KEY_BACKSPACE    14
  0xd, // KEY_TAB    15
  0x15, // KEY_Q    16
  0x1d, // KEY_W    17
  0x24, // KEY_E    18
  0x2d, // KEY_R    19
  0x2c, // KEY_T    20
  0x35, // KEY_Y    21
  0x3c, // KEY_U    22
  0x43, // KEY_I    23
  0x44, // KEY_O    24
  0x4d, // KEY_P    25
  0x54, // KEY_LEFTBRACE    26
  0x5b, // KEY_RIGHTBRACE    27
  0x5a, // KEY_ENTER    28
  0x11, // KEY_LEFTCTRL    29
  0x1c, // KEY_A    30
  0x1b, // KEY_S    31
  0x23, // KEY_D    32
  0x2b, // KEY_F    33
  0x34, // KEY_G    34
  0x33, // KEY_H    35
  0x3b, // KEY_J    36
  0x42, // KEY_K    37
  0x4b, // KEY_L    38
  0x4c, // KEY_SEMICOLON    39
  0x52, // KEY_APOSTROPHE    40
  0xe, // KEY_GRAVE    41
  0x12, // KEY_LEFTSHIFT    42
  0x5c, // KEY_BACKSLASH    43
  0x1a, // KEY_Z    44
  0x22, // KEY_X    45
  0x21, // KEY_C    46
  0x2a, // KEY_V    47
  0x32, // KEY_B    48
  0x31, // KEY_N    49
  0x3a, // KEY_M    50
  0x41, // KEY_COMMA    51
  0x49, // KEY_DOT    52
  0x4a, // KEY_SLASH    53
  0x59, // KEY_RIGHTSHIFT    54
  0x7e, // KEY_KPASTERISK    55
  0x19, // KEY_LEFTALT    56
  0x29, // KEY_SPACE    57
  0x14, // KEY_CAPSLOCK    58
  0x7, // KEY_F1    59
  0xf, // KEY_F2    60
  0x17, // KEY_F3    61
  0x1f, // KEY_F4    62
  0x27, // KEY_F5    63
  0x2f, // KEY_F6    64
  0x37, // KEY_F7    65
  0x3f, // KEY_F8    66
  0x47, // KEY_F9    67
  0x4f, // KEY_F10    68
  0x76, // KEY_NUMLOCK    69
  0x5f, // KEY_SCROLLLOCK    70
  0x6c, // KEY_KP7    71
  0x75, // KEY_KP8    72
  0x7d, // KEY_KP9    73
  0x4e, // KEY_KPMINUS    74
  0x6b, // KEY_KP4    75
  0x73, // KEY_KP5    76
  0x74, // KEY_KP6    77
  0x7c, // KEY_KPPLUS    78
  0x69, // KEY_KP1    79
  0x72, // KEY_KP2    80
  0x7a, // KEY_KP3    81
  0x70, // KEY_KP0    82
  0x71, // KEY_KPDOT    83
  CODE_UNUSED, // KEY_UNUSED    84
  CODE_UNUSED, // KEY_ZENKAKUHANKAKU    85
  0x13, // KEY_102ND    86
  0x56, // KEY_F11    87
  0x5e, // KEY_F12    88
  CODE_UNUSED, // KEY_RO    89
  CODE_UNUSED, // KEY_KATAKANA    90
  CODE_UNUSED, // KEY_HIRAGANA    91
  CODE_UNUSED, // KEY_HENKAN    92
  CODE_UNUSED, // KEY_KATAKANAHIRAGANA    93
  CODE_UNUSED, // KEY_MUHENKAN    94
  CODE_UNUSED, // KEY_KPJPCOMMA    95
  0x79, // KEY_KPENTER    96
  0x58, // KEY_RIGHTCTRL    97
  0x4a, // KEY_KPSLASH    98
  0x57, // KEY_SYSRQ    99
  0x39, // KEY_RIGHTALT    100
  CODE_UNUSED, // KEY_LINEFEED    101
  0x6e, // KEY_HOME    102
  0x63, // KEY_UP    103
  0x6f, // KEY_PAGEUP    104
  0x61, // KEY_LEFT    105
  0x6a, // KEY_RIGHT    106
  0x65, // KEY_END    107
  0x60, // KEY_DOWN    108
  0x6d, // KEY_PAGEDOWN    109
  0x67, // KEY_INSERT    110
  0x64, // KEY_DELETE    111
  CODE_UNUSED, // KEY_MACRO    112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER    116
  CODE_UNUSED, // KEY_KPEQUAL    117
  CODE_UNUSED, // KEY_KPPLUSMINUS    118
  0x62, // KEY_PAUSE    119
  CODE_UNUSED, // KEY_SCALE    120
  CODE_UNUSED, // KEY_KPCOMMA    121
  CODE_UNUSED, // KEY_HANGEUL    122
  CODE_UNUSED, // KEY_HANJA    123
  CODE_UNUSED, // KEY_YEN    124
  0x8b, // KEY_LEFTMETA    125
  0x8c, // KEY_RIGHTMETA    126
  0x8d, // KEY_COMPOSE    127
  CODE_UNUSED, // KEY_STOP    128
  CODE_UNUSED, // KEY_AGAIN    129
  CODE_UNUSED, // KEY_PROPS    130
  CODE_UNUSED, // KEY_UNDO    131
  CODE_UNUSED, // KEY_FRONT    132
  CODE_UNUSED, // KEY_COPY    133
  CODE_UNUSED, // KEY_OPEN    134
  CODE_UNUSED, // KEY_PASTE    135
  CODE_UNUSED, // KEY_FIND    136
  CODE_UNUSED, // KEY_CUT    137
  CODE_UNUSED, // KEY_HELP    138
  CODE_UNUSED, // KEY_MENU    139
  CODE_UNUSED, // KEY_CALC    140
  CODE_UNUSED, // KEY_SETUP    141
  CODE_UNUSED, // KEY_SLEEP    142
  CODE_UNUSED, // KEY_WAKEUP    143
  CODE_UNUSED, // KEY_FILE    144
  CODE_UNUSED, // KEY_SENDFILE    145
  CODE_UNUSED, // KEY_DELETEFILE    146
  CODE_UNUSED, // KEY_XFER    147
  CODE_UNUSED, // KEY_PROG1    148
  CODE_UNUSED, // KEY_PROG2    149
  CODE_UNUSED, // KEY_WWW    150
  CODE_UNUSED, // KEY_MSDOS    151
  CODE_UNUSED, // KEY_COFFEE    152
  CODE_UNUSED, // KEY_ROTATE_DISPLAY    153
  CODE_UNUSED, // KEY_CYCLEWINDOWS    154
  CODE_UNUSED, // KEY_MAIL    155
  CODE_UNUSED, // KEY_BOOKMARKS    156
  CODE_UNUSED, // KEY_COMPUTER    157
  CODE_UNUSED, // KEY_BACK    158
  CODE_UNUSED, // KEY_FORWARD    159
  CODE_UNUSED, // KEY_CLOSECD    160
  CODE_UNUSED, // KEY_EJECTCD    161
  CODE_UNUSED, // KEY_EJECTCLOSECD    162
  CODE_UNUSED, // KEY_NEXTSONG    163
  CODE_UNUSED, // KEY_PLAYPAUSE    164
  CODE_UNUSED, // KEY_PREVIOUSSONG    165
  CODE_UNUSED, // KEY_STOPCD    166
  CODE_UNUSED, // KEY_RECORD    167
  CODE_UNUSED, // KEY_REWIND    168
  CODE_UNUSED, // KEY_PHONE    169
  CODE_UNUSED, // KEY_ISO    170
  CODE_UNUSED, // KEY_CONFIG    171
  CODE_UNUSED, // KEY_HOMEPAGE    172
  CODE_UNUSED, // KEY_REFRESH    173
  CODE_UNUSED, // KEY_EXIT    174
  CODE_UNUSED, // KEY_MOVE    175
  CODE_UNUSED, // KEY_EDIT    176
  CODE_UNUSED, // KEY_SCROLLUP    177
  CODE_UNUSED, // KEY_SCROLLDOWN    178
  CODE_UNUSED, // KEY_KPLEFTPAREN    179
  CODE_UNUSED, // KEY_KPRIGHTPAREN    180
  CODE_UNUSED, // KEY_NEW    181
  CODE_UNUSED, // KEY_REDO    182
  0x08, // KEY_F13    183
  0x10, // KEY_F14    184
  0x18, // KEY_F15    185
  0x20, // KEY_F16    186
  0x28, // KEY_F17    187
  0x30, // KEY_F18    188
  0x38, // KEY_F19    189
  0x40, // KEY_F20    190
  0x48, // KEY_F21    191
  0x50, // KEY_F22    192
  0x57, // KEY_F23    193
  0x5f, // KEY_F24    194
};

#define SET3_KEY_STATE_UNKNOWN 0
#define SET3_KEY_STATE_ALL 1
#define SET3_KEY_STATE_MAKE_BREAK 2
#define SET3_KEY_STATE_MAKE_ONLY 3
#define SET3_KEY_STATE_TYPEMATIC_ONLY 4

#define SET3_STATUS_LOOKUP_SIZE 142

const uint8_t scancode_set3_default_status[SET3_STATUS_LOOKUP_SIZE] = 
{
  SET3_KEY_STATE_ALL, // (0, 0x0), UNUSED
  SET3_KEY_STATE_ALL, // (1, 0x1), UNUSED
  SET3_KEY_STATE_ALL, // (2, 0x2), UNUSED
  SET3_KEY_STATE_ALL, // (3, 0x3), UNUSED
  SET3_KEY_STATE_ALL, // (4, 0x4), UNUSED
  SET3_KEY_STATE_ALL, // (5, 0x5), UNUSED
  SET3_KEY_STATE_ALL, // (6, 0x6), UNUSED
  SET3_KEY_STATE_MAKE_ONLY, // (7, 0x7), KEY_F1, (59, 0x3b)
  SET3_KEY_STATE_MAKE_ONLY, // (8, 0x8), KEY_F13, (183, 0xb7)
  SET3_KEY_STATE_ALL, // (9, 0x9), UNUSED
  SET3_KEY_STATE_ALL, // (10, 0xa), UNUSED
  SET3_KEY_STATE_ALL, // (11, 0xb), UNUSED
  SET3_KEY_STATE_ALL, // (12, 0xc), UNUSED
  SET3_KEY_STATE_ALL, // (13, 0xd), KEY_TAB, (15, 0xf)
  SET3_KEY_STATE_ALL, // (14, 0xe), KEY_GRAVE, (41, 0x29)
  SET3_KEY_STATE_MAKE_ONLY, // (15, 0xf), KEY_F2, (60, 0x3c)
  SET3_KEY_STATE_ALL, // (16, 0x10), KEY_F14, (184, 0xb8)
  SET3_KEY_STATE_MAKE_BREAK, // (17, 0x11), KEY_LEFTCTRL, (29, 0x1d)
  SET3_KEY_STATE_MAKE_BREAK, // (18, 0x12), KEY_LEFTSHIFT, (42, 0x2a)
  SET3_KEY_STATE_ALL, // (19, 0x13), KEY_102ND, (86, 0x56)
  SET3_KEY_STATE_MAKE_BREAK, // (20, 0x14), KEY_CAPSLOCK, (58, 0x3a)
  SET3_KEY_STATE_ALL, // (21, 0x15), KEY_Q, (16, 0x10)
  SET3_KEY_STATE_ALL, // (22, 0x16), KEY_1, (2, 0x2)
  SET3_KEY_STATE_MAKE_ONLY, // (23, 0x17), KEY_F3, (61, 0x3d)
  SET3_KEY_STATE_ALL, // (24, 0x18), KEY_F15, (185, 0xb9)
  SET3_KEY_STATE_MAKE_BREAK, // (25, 0x19), KEY_LEFTALT, (56, 0x38)
  SET3_KEY_STATE_ALL, // (26, 0x1a), KEY_Z, (44, 0x2c)
  SET3_KEY_STATE_ALL, // (27, 0x1b), KEY_S, (31, 0x1f)
  SET3_KEY_STATE_ALL, // (28, 0x1c), KEY_A, (30, 0x1e)
  SET3_KEY_STATE_ALL, // (29, 0x1d), KEY_W, (17, 0x11)
  SET3_KEY_STATE_ALL, // (30, 0x1e), KEY_2, (3, 0x3)
  SET3_KEY_STATE_MAKE_ONLY, // (31, 0x1f), KEY_F4, (62, 0x3e)
  SET3_KEY_STATE_ALL, // (32, 0x20), KEY_F16, (186, 0xba)
  SET3_KEY_STATE_ALL, // (33, 0x21), KEY_C, (46, 0x2e)
  SET3_KEY_STATE_ALL, // (34, 0x22), KEY_X, (45, 0x2d)
  SET3_KEY_STATE_ALL, // (35, 0x23), KEY_D, (32, 0x20)
  SET3_KEY_STATE_ALL, // (36, 0x24), KEY_E, (18, 0x12)
  SET3_KEY_STATE_ALL, // (37, 0x25), KEY_4, (5, 0x5)
  SET3_KEY_STATE_ALL, // (38, 0x26), KEY_3, (4, 0x4)
  SET3_KEY_STATE_MAKE_ONLY, // (39, 0x27), KEY_F5, (63, 0x3f)
  SET3_KEY_STATE_ALL, // (40, 0x28), KEY_F17, (187, 0xbb)
  SET3_KEY_STATE_ALL, // (41, 0x29), KEY_SPACE, (57, 0x39)
  SET3_KEY_STATE_ALL, // (42, 0x2a), KEY_V, (47, 0x2f)
  SET3_KEY_STATE_ALL, // (43, 0x2b), KEY_F, (33, 0x21)
  SET3_KEY_STATE_ALL, // (44, 0x2c), KEY_T, (20, 0x14)
  SET3_KEY_STATE_ALL, // (45, 0x2d), KEY_R, (19, 0x13)
  SET3_KEY_STATE_ALL, // (46, 0x2e), KEY_5, (6, 0x6)
  SET3_KEY_STATE_MAKE_ONLY, // (47, 0x2f), KEY_F6, (64, 0x40)
  SET3_KEY_STATE_ALL, // (48, 0x30), KEY_F18, (188, 0xbc)
  SET3_KEY_STATE_ALL, // (49, 0x31), KEY_N, (49, 0x31)
  SET3_KEY_STATE_ALL, // (50, 0x32), KEY_B, (48, 0x30)
  SET3_KEY_STATE_ALL, // (51, 0x33), KEY_H, (35, 0x23)
  SET3_KEY_STATE_ALL, // (52, 0x34), KEY_G, (34, 0x22)
  SET3_KEY_STATE_ALL, // (53, 0x35), KEY_Y, (21, 0x15)
  SET3_KEY_STATE_ALL, // (54, 0x36), KEY_6, (7, 0x7)
  SET3_KEY_STATE_MAKE_ONLY, // (55, 0x37), KEY_F7, (65, 0x41)
  SET3_KEY_STATE_ALL, // (56, 0x38), KEY_F19, (189, 0xbd)
  SET3_KEY_STATE_MAKE_ONLY, // (57, 0x39), KEY_RIGHTALT, (100, 0x64)
  SET3_KEY_STATE_ALL, // (58, 0x3a), KEY_M, (50, 0x32)
  SET3_KEY_STATE_ALL, // (59, 0x3b), KEY_J, (36, 0x24)
  SET3_KEY_STATE_ALL, // (60, 0x3c), KEY_U, (22, 0x16)
  SET3_KEY_STATE_ALL, // (61, 0x3d), KEY_7, (8, 0x8)
  SET3_KEY_STATE_ALL, // (62, 0x3e), KEY_8, (9, 0x9)
  SET3_KEY_STATE_MAKE_ONLY, // (63, 0x3f), KEY_F8, (66, 0x42)
  SET3_KEY_STATE_ALL, // (64, 0x40), KEY_F20, (190, 0xbe)
  SET3_KEY_STATE_ALL, // (65, 0x41), KEY_COMMA, (51, 0x33)
  SET3_KEY_STATE_ALL, // (66, 0x42), KEY_K, (37, 0x25)
  SET3_KEY_STATE_ALL, // (67, 0x43), KEY_I, (23, 0x17)
  SET3_KEY_STATE_ALL, // (68, 0x44), KEY_O, (24, 0x18)
  SET3_KEY_STATE_ALL, // (69, 0x45), KEY_0, (11, 0xb)
  SET3_KEY_STATE_ALL, // (70, 0x46), KEY_9, (10, 0xa)
  SET3_KEY_STATE_MAKE_ONLY, // (71, 0x47), KEY_F9, (67, 0x43)
  SET3_KEY_STATE_ALL, // (72, 0x48), KEY_F21, (191, 0xbf)
  SET3_KEY_STATE_ALL, // (73, 0x49), KEY_DOT, (52, 0x34)
  SET3_KEY_STATE_ALL, // (74, 0x4a), KEY_KPSLASH, (98, 0x62)
  SET3_KEY_STATE_ALL, // (75, 0x4b), KEY_L, (38, 0x26)
  SET3_KEY_STATE_ALL, // (76, 0x4c), KEY_SEMICOLON, (39, 0x27)
  SET3_KEY_STATE_ALL, // (77, 0x4d), KEY_P, (25, 0x19)
  SET3_KEY_STATE_ALL, // (78, 0x4e), KEY_KPMINUS, (74, 0x4a)
  SET3_KEY_STATE_MAKE_ONLY, // (79, 0x4f), KEY_F10, (68, 0x44)
  SET3_KEY_STATE_ALL, // (80, 0x50), KEY_F22, (192, 0xc0)
  SET3_KEY_STATE_ALL, // (81, 0x51), UNUSED
  SET3_KEY_STATE_ALL, // (82, 0x52), KEY_APOSTROPHE, (40, 0x28)
  SET3_KEY_STATE_ALL, // (83, 0x53), UNUSED
  SET3_KEY_STATE_ALL, // (84, 0x54), KEY_LEFTBRACE, (26, 0x1a)
  SET3_KEY_STATE_ALL, // (85, 0x55), KEY_EQUAL, (13, 0xd)
  SET3_KEY_STATE_MAKE_ONLY, // (86, 0x56), KEY_F11, (87, 0x57)
  SET3_KEY_STATE_MAKE_ONLY, // (87, 0x57), KEY_F23, (193, 0xc1)
  SET3_KEY_STATE_MAKE_ONLY, // (88, 0x58), KEY_RIGHTCTRL, (97, 0x61)
  SET3_KEY_STATE_MAKE_BREAK, // (89, 0x59), KEY_RIGHTSHIFT, (54, 0x36)
  SET3_KEY_STATE_ALL, // (90, 0x5a), KEY_ENTER, (28, 0x1c)
  SET3_KEY_STATE_ALL, // (91, 0x5b), KEY_RIGHTBRACE, (27, 0x1b)
  SET3_KEY_STATE_ALL, // (92, 0x5c), KEY_BACKSLASH, (43, 0x2b)
  SET3_KEY_STATE_ALL, // (93, 0x5d), UNUSED
  SET3_KEY_STATE_MAKE_ONLY, // (94, 0x5e), KEY_F12, (88, 0x58)
  SET3_KEY_STATE_MAKE_ONLY, // (95, 0x5f), KEY_F24, (194, 0xc2)
  SET3_KEY_STATE_ALL, // (96, 0x60), KEY_DOWN, (108, 0x6c)
  SET3_KEY_STATE_ALL, // (97, 0x61), KEY_LEFT, (105, 0x69)
  SET3_KEY_STATE_MAKE_ONLY, // (98, 0x62), KEY_PAUSE, (119, 0x77)
  SET3_KEY_STATE_ALL, // (99, 0x63), KEY_UP, (103, 0x67)
  SET3_KEY_STATE_ALL, // (100, 0x64), KEY_DELETE, (111, 0x6f)
  SET3_KEY_STATE_MAKE_ONLY, // (101, 0x65), KEY_END, (107, 0x6b)
  SET3_KEY_STATE_ALL, // (102, 0x66), KEY_BACKSPACE, (14, 0xe)
  SET3_KEY_STATE_MAKE_ONLY, // (103, 0x67), KEY_INSERT, (110, 0x6e)
  SET3_KEY_STATE_ALL, // (104, 0x68), UNUSED
  SET3_KEY_STATE_MAKE_ONLY, // (105, 0x69), KEY_KP1, (79, 0x4f)
  SET3_KEY_STATE_ALL, // (106, 0x6a), KEY_RIGHT, (106, 0x6a)
  SET3_KEY_STATE_MAKE_ONLY, // (107, 0x6b), KEY_KP4, (75, 0x4b)
  SET3_KEY_STATE_MAKE_ONLY, // (108, 0x6c), KEY_KP7, (71, 0x47)
  SET3_KEY_STATE_MAKE_ONLY, // (109, 0x6d), KEY_PAGEDOWN, (109, 0x6d)
  SET3_KEY_STATE_MAKE_ONLY, // (110, 0x6e), KEY_HOME, (102, 0x66)
  SET3_KEY_STATE_MAKE_ONLY, // (111, 0x6f), KEY_PAGEUP, (104, 0x68)
  SET3_KEY_STATE_MAKE_ONLY, // (112, 0x70), KEY_KP0, (82, 0x52)
  SET3_KEY_STATE_MAKE_ONLY, // (113, 0x71), KEY_KPDOT, (83, 0x53)
  SET3_KEY_STATE_MAKE_ONLY, // (114, 0x72), KEY_KP2, (80, 0x50)
  SET3_KEY_STATE_MAKE_ONLY, // (115, 0x73), KEY_KP5, (76, 0x4c)
  SET3_KEY_STATE_MAKE_ONLY, // (116, 0x74), KEY_KP6, (77, 0x4d)
  SET3_KEY_STATE_MAKE_ONLY, // (117, 0x75), KEY_KP8, (72, 0x48)
  SET3_KEY_STATE_MAKE_ONLY, // (118, 0x76), KEY_NUMLOCK, (69, 0x45)
  SET3_KEY_STATE_MAKE_ONLY, // (119, 0x77), UNUSED
  SET3_KEY_STATE_ALL, // (120, 0x78), UNUSED
  SET3_KEY_STATE_MAKE_ONLY, // (121, 0x79), KEY_KPENTER, (96, 0x60)
  SET3_KEY_STATE_MAKE_ONLY, // (122, 0x7a), KEY_KP3, (81, 0x51)
  SET3_KEY_STATE_ALL, // (123, 0x7b), UNUSED
  SET3_KEY_STATE_ALL, // (124, 0x7c), KEY_KPPLUS, (78, 0x4e)
  SET3_KEY_STATE_MAKE_ONLY, // (125, 0x7d), KEY_KP9, (73, 0x49)
  SET3_KEY_STATE_MAKE_ONLY, // (126, 0x7e), KEY_KPASTERISK, (55, 0x37)
  SET3_KEY_STATE_ALL, // (127, 0x7f), UNUSED
  SET3_KEY_STATE_ALL, // (128, 0x80), UNUSED
  SET3_KEY_STATE_ALL, // (129, 0x81), UNUSED
  SET3_KEY_STATE_ALL, // (130, 0x82), UNUSED
  SET3_KEY_STATE_ALL, // (131, 0x83), UNUSED
  SET3_KEY_STATE_MAKE_ONLY, // (132, 0x84), UNUSED
  SET3_KEY_STATE_ALL, // (133, 0x85), UNUSED
  SET3_KEY_STATE_ALL, // (134, 0x86), UNUSED
  SET3_KEY_STATE_ALL, // (135, 0x87), UNUSED
  SET3_KEY_STATE_ALL, // (136, 0x88), UNUSED
  SET3_KEY_STATE_ALL, // (137, 0x89), UNUSED
  SET3_KEY_STATE_ALL, // (138, 0x8a), UNUSED
  SET3_KEY_STATE_ALL, // (139, 0x8b), KEY_LEFTMETA, (125, 0x7d)
  SET3_KEY_STATE_ALL, // (140, 0x8c), KEY_RIGHTMETA, (126, 0x7e)
  SET3_KEY_STATE_ALL, // (141, 0x8d), KEY_COMPOSE, (127, 0x7f)
};

uint8_t scancode_set3_current_status[SET3_STATUS_LOOKUP_SIZE];

const uint8_t linux_keycode_to_ps2_scancode_lookup_single_byte_codeset2[LINUX_KEYCODE_TO_PS2_SCANCODE_SET2_SINGLE_SIZE] = 
{
    CODE_UNUSED, // KEY_RESERVED       0
    0x76, // KEY_ESC            1
    0x16, // KEY_1          2
    0x1E, // KEY_2          3
    0x26, // KEY_3          4
    0x25, // KEY_4          5
    0x2E, // KEY_5          6
    0x36, // KEY_6          7
    0x3D, // KEY_7          8
    0x3E, // KEY_8          9
    0x46, // KEY_9          10
    0x45, // KEY_0          11
    0x4E, // KEY_MINUS      12
    0x55, // KEY_EQUAL      13
    0x66, // KEY_BACKSPACE      14
    0x0D, // KEY_TAB            15
    0x15, // KEY_Q          16
    0x1D, // KEY_W          17
    0x24, // KEY_E          18
    0x2D, // KEY_R          19
    0x2C, // KEY_T          20
    0x35, // KEY_Y          21
    0x3C, // KEY_U          22
    0x43, // KEY_I          23
    0x44, // KEY_O          24
    0x4D, // KEY_P          25
    0x54, // KEY_LEFTBRACE      26
    0x5B, // KEY_RIGHTBRACE     27
    0x5A, // KEY_ENTER      28
    0x14, // KEY_LEFTCTRL       29
    0x1C, // KEY_A          30
    0x1B, // KEY_S          31
    0x23, // KEY_D          32
    0x2B, // KEY_F          33
    0x34, // KEY_G          34
    0x33, // KEY_H          35
    0x3B, // KEY_J          36
    0x42, // KEY_K          37
    0x4B, // KEY_L          38
    0x4C, // KEY_SEMICOLON      39
    0x52, // KEY_APOSTROPHE     40
    0x0E, // KEY_GRAVE      41
    0x12, // KEY_LEFTSHIFT      42
    0x5D, // KEY_BACKSLASH      43
    0x1A, // KEY_Z          44
    0x22, // KEY_X          45
    0x21, // KEY_C          46
    0x2A, // KEY_V          47
    0x32, // KEY_B          48
    0x31, // KEY_N          49
    0x3A, // KEY_M          50
    0x41, // KEY_COMMA      51
    0x49, // KEY_DOT            52
    0x4A, // KEY_SLASH      53
    0x59, // KEY_RIGHTSHIFT     54
    0x7C, // KEY_KPASTERISK     55
    0x11, // KEY_LEFTALT        56
    0x29, // KEY_SPACE      57
    0x58, // KEY_CAPSLOCK       58
    0x05, // KEY_F1         59
    0x06, // KEY_F2         60
    0x04, // KEY_F3         61
    0x0C, // KEY_F4         62
    0x03, // KEY_F5         63
    0x0B, // KEY_F6         64
    0x83, // KEY_F7         65
    0x0A, // KEY_F8         66
    0x01, // KEY_F9         67
    0x09, // KEY_F10            68
    0x77, // KEY_NUMLOCK        69
    0x7E, // KEY_SCROLLLOCK     70
    0x6C, // KEY_KP7            71
    0x75, // KEY_KP8            72
    0x7D, // KEY_KP9            73
    0x7B, // KEY_KPMINUS        74
    0x6B, // KEY_KP4            75
    0x73, // KEY_KP5            76
    0x74, // KEY_KP6            77
    0x79, // KEY_KPPLUS     78
    0x69, // KEY_KP1            79
    0x72, // KEY_KP2            80
    0x7A, // KEY_KP3            81
    0x70, // KEY_KP0            82
    0x71, // KEY_KPDOT      83
    CODE_UNUSED, // KEY_UNUSED     84
    CODE_UNUSED, // KEY_ZENKAKUHANKAKU 85
    0x61, // KEY_102ND      86
    0x78, // KEY_F11            87
    0x07, // KEY_F12            88
};

const uint8_t linux_keycode_to_ps2_scancode_lookup_special_codeset2[LINUX_KEYCODE_TO_PS2_SCANCODE_SET2_SPECIAL_SIZE] = 
{
  0x5A, // KPENTER    96
  0x14, // KEY_RIGHTCTRL    97
  0x4A, // KEY_KPSLASH    98
  CODE_UNUSED, // 99
  0x11, // KEY_RIGHTALT   100
  CODE_UNUSED, // 101
  0x6C, // KEY_HOME   102
  0x75, // KEY_UP     103
  0x7D, // KEY_PAGEUP   104
  0x6B, // KEY_LEFT   105
  0x74, // KEY_RIGHT    106
  0x69, // KEY_END      107
  0x72, // KEY_DOWN   108
  0x7A, // KEY_PAGEDOWN   109
  0x70, // KEY_INSERT   110
  0x71, // KEY_DELETE   111
  CODE_UNUSED, // KEY_MACRO   112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER   116 
  CODE_UNUSED, // KEY_KPEQUAL   117
  CODE_UNUSED, // KEY_KPPLUSMINUS   118
  CODE_UNUSED, // KEY_PAUSE   119
  CODE_UNUSED, // KEY_SCALE   120 
  CODE_UNUSED, // KEY_KPCOMMA   121
  CODE_UNUSED, // KEY_HANGEUL   122
  CODE_UNUSED, // KEY_HANJA   123
  CODE_UNUSED, // KEY_YEN     124
  0x1F, // KEY_LEFTMETA   125
  0x27, // KEY_RIGHTMETA    126
  0x2F, // KEY_COMPOSE    127
};

GPIO_TypeDef* ps2kb_clk_port;
uint16_t ps2kb_clk_pin;

GPIO_TypeDef* ps2kb_data_port;
uint16_t ps2kb_data_pin;
uint32_t ps2kb_wait_start;

uint8_t ps2kb_current_scancode_set = 2;
uint8_t ps2kb_data_reporting_enabled = 1;

#define PS2KB_CLK_HI() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_SET)
#define PS2KB_CLK_LOW() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_RESET)

#define PS2KB_DATA_HI() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_SET)
#define PS2KB_DATA_LOW() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_RESET)

#define PS2KB_READ_DATA_PIN() HAL_GPIO_ReadPin(ps2kb_data_port, ps2kb_data_pin)
#define PS2KB_READ_CLK_PIN() HAL_GPIO_ReadPin(ps2kb_clk_port, ps2kb_clk_pin)

#define PS2KB_SENDACK() ps2kb_write(0xFA, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS)

void ps2kb_release_lines(void)
{
  PS2KB_CLK_HI();
  PS2KB_DATA_HI();
}

void ps2kb_reset(void)
{
  ps2kb_current_scancode_set = 2;
  ps2kb_data_reporting_enabled = 1;
  memcpy(scancode_set3_current_status, scancode_set3_default_status, SET3_STATUS_LOOKUP_SIZE);
}

void ps2kb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
	ps2kb_clk_port = clk_port;
	ps2kb_clk_pin = clk_pin;
	ps2kb_data_port = data_port;
	ps2kb_data_pin = data_pin;
	ps2kb_release_lines();
  ps2kb_reset();
}

uint8_t ps2kb_get_bus_status(void)
{
  uint8_t clk_stat = PS2KB_READ_CLK_PIN();
  uint8_t data_stat = PS2KB_READ_DATA_PIN();
	if(data_stat == GPIO_PIN_SET && clk_stat == GPIO_PIN_SET)
		return PS2_BUS_IDLE;
	if(data_stat == GPIO_PIN_SET && clk_stat == GPIO_PIN_RESET)
		return PS2_BUS_INHIBIT;
	if(data_stat == GPIO_PIN_RESET && clk_stat == GPIO_PIN_SET)
		return PS2_BUS_REQ_TO_SEND;
	return PS2_BUS_UNKNOWN;
}

uint8_t ps2kb_read(uint8_t* result, uint8_t timeout_ms)
{
  uint16_t data = 0x00;
  uint16_t bit = 0x01;

  ps2kb_wait_start = HAL_GetTick();
  while(ps2kb_get_bus_status() != PS2_BUS_REQ_TO_SEND)
  {
  	if(HAL_GetTick() - ps2kb_wait_start >= timeout_ms)
  		return PS2_ERROR_TIMEOUT;
  }

  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  while (bit < 0x0100)
  {
    if (PS2KB_READ_DATA_PIN() == GPIO_PIN_SET)
	    data = data | bit;
    bit = bit << 1;
    delay_us(CLKHALF);
    PS2KB_CLK_LOW();
    delay_us(CLKFULL);
    PS2KB_CLK_HI();
    delay_us(CLKHALF);
  }

  // stop bit
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(CLKHALF);
  PS2KB_DATA_LOW();
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  PS2KB_DATA_HI();

  *result = data & 0x00FF;
  return PS2_OK;
}

uint8_t ps2kb_write_nowait(uint8_t data)
{
  uint8_t parity = 1;

  PS2KB_DATA_LOW();
  delay_us(CLKHALF);
  // device sends on falling clock
  PS2KB_CLK_LOW(); // start bit
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2kb_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  for (int i=0; i < 8; i++)
  {
    if (data & 0x01)
      PS2KB_DATA_HI();
    else
      PS2KB_DATA_LOW();

    delay_us(CLKHALF);
    PS2KB_CLK_LOW();
    delay_us(CLKFULL);
    PS2KB_CLK_HI();
    delay_us(CLKHALF);
    if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
    {
      ps2kb_release_lines();
      return PS2_ERROR_HOST_INHIBIT;
    }

    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }

  // parity bit
  if (parity)
    PS2KB_DATA_HI();
  else
    PS2KB_DATA_LOW();

  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2kb_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  // stop bit
  PS2KB_DATA_HI();
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(BYTEWAIT_END);

  return PS2_OK;
}

uint8_t ps2kb_write(uint8_t data, uint8_t delay_start, uint8_t timeout_ms)
{
  ps2kb_write_idle_check:
  ps2kb_wait_start = HAL_GetTick();
  while(ps2kb_get_bus_status() != PS2_BUS_IDLE)
  {
  	if(HAL_GetTick() - ps2kb_wait_start >= timeout_ms)
  		return PS2_ERROR_TIMEOUT;
  }

  ps2kb_wait_start = micros();
  // make sure idle is more than 50us, some PC will actually spike clock line briefly during inhibition in certain DOS games
  while(micros() - ps2kb_wait_start < 60)
  {
    if(ps2kb_get_bus_status() != PS2_BUS_IDLE)
      goto ps2kb_write_idle_check;
  }

  // if responding to host, wait a little while for it to get ready
  if(delay_start)
    delay_us(BYTEWAIT);

  return ps2kb_write_nowait(data);
}

#define PS2_RECEIVE_MODE_NORMAL 0
#define PS2_RECEIVE_MODE_TYPEMATIC_ONLY 1
#define PS2_RECEIVE_MODE_MAKE_BREAK 2
#define PS2_RECEIVE_MODE_MAKE_ONLY 3

uint8_t ps2_receive_mode;

#define SET3_CMD_BACK_TO_NORMAL_MODE 0
#define SET3_CMD_CODE_UPDATED 1
#define SET3_CMD_ERROR 2

uint8_t handle_set3_commands(uint8_t current_mode, uint8_t cmd)
{
  // printf("%x %x", current_mode, cmd);
  if(current_mode == PS2_RECEIVE_MODE_NORMAL)
    return SET3_CMD_BACK_TO_NORMAL_MODE;
  if(current_mode != PS2_RECEIVE_MODE_NORMAL && cmd >= 0xED)
    return SET3_CMD_BACK_TO_NORMAL_MODE;
  if(ps2_receive_mode == PS2_RECEIVE_MODE_TYPEMATIC_ONLY && cmd < SET3_STATUS_LOOKUP_SIZE)
    scancode_set3_current_status[cmd] = SET3_KEY_STATE_TYPEMATIC_ONLY;
  else if(ps2_receive_mode == PS2_RECEIVE_MODE_MAKE_BREAK && cmd < SET3_STATUS_LOOKUP_SIZE)
    scancode_set3_current_status[cmd] = SET3_KEY_STATE_MAKE_BREAK;
  else if(ps2_receive_mode == PS2_RECEIVE_MODE_MAKE_ONLY && cmd < SET3_STATUS_LOOKUP_SIZE)
    scancode_set3_current_status[cmd] = SET3_KEY_STATE_MAKE_ONLY;
  return SET3_CMD_CODE_UPDATED;
}

void keyboard_reply(uint8_t cmd, uint8_t *leds)
{
  uint8_t received = 255;
  if(handle_set3_commands(ps2_receive_mode, cmd) == SET3_CMD_BACK_TO_NORMAL_MODE)
  {
    ps2_receive_mode = PS2_RECEIVE_MODE_NORMAL;
  }
  else
  {
    PS2KB_SENDACK();
    return;
  }

  switch (cmd)
  {
	  case 0xFF: //reset
	    PS2KB_SENDACK();
      ps2kb_reset();
      HAL_Delay(333); // probably unnecessary, but that's what most keyboards do
	    ps2kb_write(0xAA, 0, 250);
      // IBM battlecruiser 1394324 sends two extra keyboard ID bytes after reset
      // ps2kb_write(0xBF, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
      // ps2kb_write(0xAC, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xFE: //resend
	    PS2KB_SENDACK();
	    break;
    case 0xFD:
      ps2_receive_mode = PS2_RECEIVE_MODE_MAKE_ONLY;
      PS2KB_SENDACK();
      break;
    case 0xFC:
      ps2_receive_mode = PS2_RECEIVE_MODE_MAKE_BREAK;
      PS2KB_SENDACK();
      break;
    case 0xFB:
      ps2_receive_mode = PS2_RECEIVE_MODE_TYPEMATIC_ONLY;
      PS2KB_SENDACK();
      break;
    case 0xFA: // set all keys to all, set 3 only
      for (int i = 0; i < SET3_STATUS_LOOKUP_SIZE; ++i)
        scancode_set3_current_status[i] = SET3_KEY_STATE_ALL;
      PS2KB_SENDACK();
      break;
    case 0xF9: // set all keys to make only, set 3 only
      for (int i = 0; i < SET3_STATUS_LOOKUP_SIZE; ++i)
        scancode_set3_current_status[i] = SET3_KEY_STATE_MAKE_ONLY;
      PS2KB_SENDACK();
      break;
    case 0xF8: // set all keys to make/brake, set 3 only
      for (int i = 0; i < SET3_STATUS_LOOKUP_SIZE; ++i)
        scancode_set3_current_status[i] = SET3_KEY_STATE_MAKE_BREAK;
      PS2KB_SENDACK();
      break;
    case 0xF7: // set all keys to typematic only, set 3 only
      for (int i = 0; i < SET3_STATUS_LOOKUP_SIZE; ++i)
        scancode_set3_current_status[i] = SET3_KEY_STATE_TYPEMATIC_ONLY;
      PS2KB_SENDACK();
      break;
	  case 0xF6: //set defaults
      ps2kb_reset();
	    PS2KB_SENDACK();
	    break;
	  case 0xF5: //disable data reporting, restore default
      ps2kb_reset();
	    PS2KB_SENDACK();
      ps2kb_data_reporting_enabled = 0;
	    break;
	  case 0xF4: //enable data reporting
	    PS2KB_SENDACK();
      ps2kb_data_reporting_enabled = 1;
	    break;
	  case 0xF3: //set typematic rate
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == PS2_OK) 
	    	PS2KB_SENDACK(); //do nothing with the rate
	    break;
	  case 0xF2: //get device id
	    PS2KB_SENDACK();
	    ps2kb_write(0xAB, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    ps2kb_write(0x83, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
      // ps2kb_write(0xBF, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS); // ID for IBM battlecruiser 1394324
      // ps2kb_write(0xAC, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xF0: //get/change scan code set
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == PS2_OK)
      {
	    	PS2KB_SENDACK();
        if(received == 0)
          ps2kb_write(ps2kb_current_scancode_set, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
        else if(received <= 3)
          ps2kb_current_scancode_set = received;
      }
	    break;
	  case 0xEE: //echo
	    ps2kb_write(0xEE, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xED: // set/reset LEDs
	    PS2KB_SENDACK();
	    if(ps2kb_read(leds, 30) == PS2_OK)
	    	PS2KB_SENDACK();
	    break;
    default:
      PS2KB_SENDACK();
      break;
  }
}

#define LINUX_KEYCODE_F11 87
#define LINUX_KEYCODE_F12 88
uint8_t ps2kb_press_key_scancode_1(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  // XT codes
  if(linux_keycode <= 83 || linux_keycode == LINUX_KEYCODE_F11 || linux_keycode == LINUX_KEYCODE_F12)
  {
    if(linux_keyvalue)
    {
      if(ps2kb_write(linux_keycode, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(linux_keycode | 0x80, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  return PS2_ERROR_UNKNOWN;
}

uint8_t ps2kb_press_key_scancode_2(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  // linux_keyvalue: release 0 press 1 autorepeat 2
  uint8_t lookup_result;
  if(linux_keycode < LINUX_KEYCODE_TO_PS2_SCANCODE_SET2_SINGLE_SIZE)
  {
    lookup_result = linux_keycode_to_ps2_scancode_lookup_single_byte_codeset2[linux_keycode];
    if(lookup_result == CODE_UNUSED)
      return PS2_ERROR_UNUSED_CODE;
    if(linux_keyvalue)
    {
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }

  if(linux_keycode == LINUX_KEYCODE_SYSRQ)
  {
    if(linux_keyvalue)
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x12, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x7c, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x7c, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x12, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  else if(linux_keycode == LINUX_KEYCODE_PAUSE && linux_keyvalue)
  {
    if(ps2kb_write(0xe1, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x14, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x77, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xe1, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x14, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x77, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    return PS2_OK;
  }
  else if(linux_keycode >= 96 && linux_keycode <= 127)
  {
    lookup_result = linux_keycode_to_ps2_scancode_lookup_special_codeset2[linux_keycode-96];
    if(lookup_result == CODE_UNUSED)
      return PS2_ERROR_UNUSED_CODE;
    if(linux_keyvalue)
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  return PS2_ERROR_UNKNOWN;
}

uint8_t ps2kb_press_key_scancode_3(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  // printf("%d %d", linux_keycode, linux_keyvalue);
  // linux_keyvalue: release 0 press 1 autorepeat 2
  if(linux_keycode >= LINUX_KEYCODE_TO_PS2_SCANCODE_SET3_SIZE)
    return PS2_ERROR_UNKNOWN_EV;

  uint8_t set3_scancode = linux_keycode_to_ps3_scancode_lookup_codeset3[linux_keycode];
  if(set3_scancode == CODE_UNUSED)
    return PS2_ERROR_UNUSED_CODE;

  if(set3_scancode >= SET3_STATUS_LOOKUP_SIZE)
    return PS2_ERROR_UNKNOWN_SCANCODE;

  uint8_t key_status = scancode_set3_current_status[set3_scancode];
  // make
  if(linux_keyvalue == 1 && (key_status == SET3_KEY_STATE_MAKE_BREAK || key_status == SET3_KEY_STATE_ALL || key_status == SET3_KEY_STATE_MAKE_ONLY))
  {
    if(ps2kb_write(set3_scancode, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
  }
  // break
  if(linux_keyvalue == 0 && (key_status == SET3_KEY_STATE_MAKE_BREAK || key_status == SET3_KEY_STATE_ALL))
  {
    if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(set3_scancode, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
  }
  // typematic
  if(linux_keyvalue == 2 && (key_status == SET3_KEY_STATE_ALL || key_status == SET3_KEY_STATE_TYPEMATIC_ONLY))
  {
    if(ps2kb_write(set3_scancode, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
  }
  return PS2_OK;
}

uint8_t ps2kb_press_key(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  if(ps2kb_data_reporting_enabled == 0)
    return PS2_ERROR_SCAN_DISABLED;
  switch(ps2kb_current_scancode_set)
  {
    case 1:
      return ps2kb_press_key_scancode_1(linux_keycode, linux_keyvalue);
    case 2:
      return ps2kb_press_key_scancode_2(linux_keycode, linux_keyvalue);
    case 3:
      return ps2kb_press_key_scancode_3(linux_keycode, linux_keyvalue);
    default:
      return PS2_ERROR_UNKNOWN_CODE_SET;
  }
}
