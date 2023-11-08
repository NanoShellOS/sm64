#ifdef TARGET_NANOSHELL

#include <stdbool.h>
#include <ultra64.h>

#include <stdio.h>

#include "controller_api.h"
#include "../configfile.h"
#include "../common.h"

#include <nanoshell/nanoshell.h>

#include "controller_nanoshell_keyboard.h"

static int mapping_length = 13;
static int keyboard_mapping[13][2];

static void set_keyboard_mapping(int index, int mask, int scancode) {
    keyboard_mapping[index][0] = scancode;
    keyboard_mapping[index][1] = mask;
}

static int keyboard_keycode_buf[1024];
static int keyboard_keycode_buf_head, keyboard_keycode_buf_tail;

int advance(int * x) {
	int old_x = *x;
	*x = (old_x + 1) % 1024;
	return old_x;
}

void keyboard_nanoshell_feed(int keycode) {
	keyboard_keycode_buf[advance(&keyboard_keycode_buf_head)] = keycode;
}

int keyboard_nanoshell_eat() {
	return keyboard_keycode_buf[advance(&keyboard_keycode_buf_tail)];
}

bool keys[128];

static void keyboard_eat() {
	while (keyboard_keycode_buf_head != keyboard_keycode_buf_tail) {
		int keycode = keyboard_nanoshell_eat();
		
		bool sta = true;
		if (keycode & SCANCODE_RELEASE) {
			sta = false;
			keycode &= ~SCANCODE_RELEASE;
		}
		
		keys[keycode] = sta;
	}
}

static void keyboard_init(void) {
    //install_keyboard();

    int i = 0;
	// TODO: Configurable.
	// Using Project64 1.6 default mapping because that's what iProgram's used to
    set_keyboard_mapping(i++, 0x80000,      KEY_UP);//configKeyStickUp);
    set_keyboard_mapping(i++, 0x10000,      KEY_LEFT);//configKeyStickLeft);
    set_keyboard_mapping(i++, 0x40000,      KEY_DOWN);//configKeyStickDown);
    set_keyboard_mapping(i++, 0x20000,      KEY_RIGHT);//configKeyStickRight);
    set_keyboard_mapping(i++, A_BUTTON,     KEY_X);//configKeyA);
    set_keyboard_mapping(i++, B_BUTTON,     KEY_C);//configKeyB);
    set_keyboard_mapping(i++, Z_TRIG,       KEY_SPACE);//configKeyZ);
    set_keyboard_mapping(i++, U_CBUTTONS,   KEY_W);//configKeyCUp);
    set_keyboard_mapping(i++, L_CBUTTONS,   KEY_A);//configKeyCLeft);
    set_keyboard_mapping(i++, D_CBUTTONS,   KEY_S);//configKeyCDown);
    set_keyboard_mapping(i++, R_CBUTTONS,   KEY_D);//configKeyCRight);
    set_keyboard_mapping(i++, R_TRIG,       KEY_E);//configKeyR);
    set_keyboard_mapping(i++, START_BUTTON, KEY_ENTER);//configKeyStart);
}

static void keyboard_read(OSContPad *pad) {
    //if (keyboard_needs_poll())
    //    poll_keyboard();
	keyboard_eat();

    for (int i = 0; i < mapping_length; i++) {
        int scan_code = keyboard_mapping[i][0];
        int mapping = keyboard_mapping[i][1];

        if (keys[scan_code & 0x7F]) {
            if (mapping == 0x10000)
                pad->stick_x = -128;
            else if (mapping  == 0x20000)
                pad->stick_x = 127;
            else if (mapping  == 0x40000)
                pad->stick_y = -128;
            else if (mapping == 0x80000)
                pad->stick_y = 127;
            else
                pad-> button |= mapping;
        }
    }

    // check for ALT+F4
    //if ((key[119] || key[120]) && key[50])
    if (keys[KEY_ALT] && keys[KEY_F4])
        game_exit();
}


struct ControllerAPI controller_nanoshell_keyboard = {
    keyboard_init,
    keyboard_read
};

#endif
