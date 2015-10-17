#ifndef TOUCH_JOY_GAMEPAD_WINDOW_H
#define TOUCH_JOY_GAMEPAD_WINDOW_H

#include "gamepad.h"

void RegisterGamepadWindowClass();
void InitializeGamepad(Gamepad* gamepad);
void DeinitializeGamepad(Gamepad* gamepad);

#endif