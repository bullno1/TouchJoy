#include <stdlib.h>
#include "gamepad.h"
#include "gb_ini.h"
#include "stb_image.h"

#define STR_EQUAL(lhs, rhs) (strcmp(lhs, rhs) == 0)
#define TO_NUM(x) strtol(x, 0, 0)

typedef struct
{
	Gamepad* gamepad;
	ParseError* error;
} ParseState;

Button* findOrCreateButton(Gamepad* gamepad, const char* buttonName)
{
	for (int i = 0; i < gamepad->numButtons; ++i)
	{
		Button* button = &gamepad->buttons[i];
		if (STR_EQUAL(buttonName, button->name))
		{
			return button;
		}
	}

	if (gamepad->numButtons == MAX_BUTTONS) { return NULL; }

	Button* button = &gamepad->buttons[gamepad->numButtons];
	++gamepad->numButtons;
	memset(button, 0, sizeof(Button));
	strcpy(button->name, buttonName);

	return button;
}

bool LoadButtonImage(const char* path, Button* button)
{
	int width, height, comp;
	stbi_uc* image = stbi_load(path, &width, &height, &comp, 4);
	if (image == NULL) { return false; }
	button->image = CreateBitmap(width, height, 1, 32, image);
	button->width = width;
	button->height = height;
	stbi_image_free(image);

	return button->image != NULL;
}

gb_Ini_HRT GamepadIniHandler(
	void* data,
	const char* section,
	const char* name,
	const char* value
)
{
#	define RETURN_ERROR(MSG) \
	do { \
		state->error->message = MSG; \
		return false; \
	} while (0, 0)

#	define ENSURE(COND, FAIL_MSG) if(!(COND)) { RETURN_ERROR(FAIL_MSG); }

	ParseState* state = (ParseState*)data;
	Gamepad* gamepad = state->gamepad;

	Button* button = findOrCreateButton(gamepad, section);

	ENSURE(button, "Too many buttons");

	if (STR_EQUAL(name, "x") || STR_EQUAL(name, "left"))
	{
		button->hMargin = TO_NUM(value);
		button->hAnchor = ANCHOR_LEFT;
	}
	else if (STR_EQUAL(name, "y") || STR_EQUAL(name, "top"))
	{
		button->vMargin = TO_NUM(value);
		button->vAnchor = ANCHOR_TOP;
	}
	else if (STR_EQUAL(name, "right"))
	{
		button->hMargin = TO_NUM(value);
		button->hAnchor = ANCHOR_RIGHT;
	}
	else if (STR_EQUAL(name, "bottom"))
	{
		button->vMargin = TO_NUM(value);
		button->vAnchor = ANCHOR_BOTTOM;
	}
	else if (STR_EQUAL(name, "keycode"))
	{
		ENSURE(button->type == BTN_KEY, "Invalid button property");
		button->extras.key.code = (WORD)TO_NUM(value);
	}
	else if (STR_EQUAL(name, "direction"))
	{
		ENSURE(button->type == BTN_WHEEL, "Invalid button property");

		if (STR_EQUAL(value, "up"))
		{
			button->extras.wheel.direction = 1;
		}
		else if (STR_EQUAL(value, "down"))
		{
			button->extras.wheel.direction = -1;
		}
		else
		{
			RETURN_ERROR("Invalid wheel direction");
		}
	}
	else if (STR_EQUAL(name, "amount"))
	{
		ENSURE(button->type == BTN_WHEEL, "Invalid button property");

		int amount = strtol(value, 0, 0);
		ENSURE(amount > 0, "Invalid scroll amount");

		button->extras.wheel.amount = amount;
	}
	else if (STR_EQUAL(name, "image"))
	{
		ENSURE(LoadButtonImage(value, button), "Could not load image");
	}
	else if (STR_EQUAL(name, "type"))
	{
		if (STR_EQUAL(value, "quit"))
		{
			button->type = BTN_QUIT;
		}
		else if (STR_EQUAL(value, "key"))
		{
			button->type = BTN_KEY;
		}
		else if (STR_EQUAL(value, "wheel"))
		{
			button->type = BTN_WHEEL;
			button->extras.wheel.amount = 1;
		}
		else
		{
			RETURN_ERROR("Invalid button type");
		}
	}
	else
	{
		RETURN_ERROR("Invalid button property");
	}

#	undef RETURN_ERROR
#	undef ENSURE

	return true;
}

bool LoadGamepad(const char* path, Gamepad* gamepad, ParseError* error)
{
	gamepad->numButtons = 0;

	ParseState state;
	state.gamepad = gamepad;
	state.error = error;
	gb_Ini_Error parseError = gb_ini_parse(path, GamepadIniHandler, &state);

	error->line = parseError.line_num;
	if (parseError.type != GB_INI_ERROR_HANDLER_ERROR)
	{
		error->message = gb_ini_error_string(parseError);
	}

	bool success = parseError.type == GB_INI_ERROR_NONE;
	// Release all resources created before
	if (!success) { FreeGamepad(gamepad); }

	return success;
}

void FreeGamepad(Gamepad* gamepad)
{
	for (int i = 0; i < gamepad->numButtons; ++i)
	{
		Button* button = &gamepad->buttons[i];
		if (button->image) { DeleteObject(button->image); }
	}

	gamepad->numButtons = 0;
}

int GetButtonX(Button* button)
{
	switch (button->hAnchor)
	{
	case ANCHOR_LEFT:
		return button->hMargin;
	case ANCHOR_RIGHT:
		return GetSystemMetrics(SM_CXSCREEN) - (button->hMargin + button->width);
	default:
		return 0;
	}
}

int GetButtonY(Button* button)
{
	switch (button->vAnchor)
	{
	case ANCHOR_TOP:
		return button->vMargin;
	case ANCHOR_BOTTOM:
		return GetSystemMetrics(SM_CYSCREEN) - (button->vMargin + button->height);
	default:
		return 0;
	}
}