#pragma once

#include <raylib.h>

void input_init();
void input_default_update();

Vector2 get_mouse_position();

#include <raylib.h>
#include <stdbool.h>

void    input_init(void);
void    input_update(void);

Vector2 input_mouse_position(void);
bool    input_mouse_down(int button);
bool    input_mouse_pressed(int button);
bool    input_mouse_released(int button);

bool    input_key_down(int key);
bool    input_key_pressed(int key);
bool    input_key_released(int key);
bool    input_key_repeat(int k);
int input_char_pressed(void);

bool input_keyboard_active(void);
