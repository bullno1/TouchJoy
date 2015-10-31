#ifndef TOUCH_JOY_GAMEPAD_H
#define TOUCH_JOY_GAMEPAD_H

#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

#define MAX_BUTTONS 32
#define MAX_ERROR_LENGTH 128

typedef enum
{
	BTN_KEY,
	BTN_WHEEL,
	BTN_STICK,
	BTN_QUIT
} ButtonType;

typedef enum
{
	ANCHOR_LEFT,
	ANCHOR_RIGHT
} HAnchorType;

typedef enum
{
	ANCHOR_TOP,
	ANCHOR_BOTTOM
} VAnchorType;

typedef enum
{
	STICK_UP,
	STICK_DOWN,
	STICK_LEFT,
	STICK_RIGHT
} StickDirection;

typedef struct
{
	ButtonType type;
	HAnchorType hAnchor;
	VAnchorType vAnchor;
	int hMargin;
	int vMargin;
	int width;
	int height;
	HBITMAP image;
	COLORREF colorKey;
	HWND window;
	char name[GB_INI_MAX_SECTION_LENGTH];
	union
	{
		struct
		{
			WORD code;
			bool sticky;
		} key;

		struct
		{
			int direction;
			DWORD amount;
		} wheel;

		struct
		{
			float threshold;
			WORD codes[4];
			bool states[4];
		} stick;
	} extras;
} Button;

typedef struct
{
	int numButtons;
	Button buttons[MAX_BUTTONS];
} Gamepad;

typedef struct
{
	size_t line;
	const char* message;
} ParseError;

bool LoadGamepad(const char* path, Gamepad* gamepad, ParseError* error);
void FreeGamepad(Gamepad* gamepad);
int GetButtonX(Button* button);
int GetButtonY(Button* button);

#endif