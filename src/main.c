#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "gb_ini.h"
#include "stb_image.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <windowsx.h>

#define UNUSED(x) ((void)x)
#define STR_EQUAL(lhs, rhs) (strcmp(lhs, rhs) == 0)
#define TO_NUM(x) strtol(x, 0, 0)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MAX_BUTTONS 64
#define MAX_ERROR_LENGTH 128

#define MOUSEEVENTF_FROMTOUCH 0xFF515700

#define BUTTON(HWND, VAR) Button* VAR = (Button*)GetWindowLongPtr(HWND, GWLP_USERDATA);

void DebugPrint(const char* format, ...)
{
	char msgBuff[MAX_ERROR_LENGTH];
	va_list args;
	va_start(args, format);
	vsnprintf(msgBuff, MAX_ERROR_LENGTH, format, args);
	va_end(args);
	OutputDebugString(msgBuff);
}

typedef enum
{
	BTN_KEY,
	BTN_WHEEL,
	BTN_QUIT
} ButtonType;

typedef struct
{
	ButtonType type;
	int x;
	int y;
	int width;
	int height;
	HBITMAP image;
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

gb_Ini_HRT GamepadIniHandler(void* data, const char* section, const char* name, const char* value)
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

	if (STR_EQUAL(name, "x"))
	{
		button->x = TO_NUM(value);
	}
	else if (STR_EQUAL(name, "y"))
	{
		button->y = TO_NUM(value);
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

bool ParseKeyMap(const char* path, Gamepad* gamepad, ParseError* error)
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

	return parseError.type == GB_INI_ERROR_NONE;
}

typedef struct
{
	Gamepad gamepad;
} ProgramState;

LRESULT CALLBACK Paint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(uMsg);
	UNUSED(wParam);
	UNUSED(lParam);

	BUTTON(hWnd, button);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	HDC buttonDC = CreateCompatibleDC(hdc);
	SelectObject(buttonDC, button->image);
	BitBlt(hdc, 0, 0, button->width, button->height, buttonDC, 0, 0, SRCPAINT);
	DeleteDC(buttonDC);
	EndPaint(hWnd, &ps);

	return 0;
}

void HandleKeyButton(Button* button, bool down)
{
	KEYBDINPUT kbInput;
	kbInput.wVk = button->extras.key.code;
	kbInput.wScan = 0;
	kbInput.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	kbInput.time = 0;
	kbInput.dwExtraInfo = 0;

	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki = kbInput;

	SendInput(1, &input, sizeof(INPUT));
}

void HandleQuitButton(Button* button, bool down)
{
	UNUSED(button);

	if (!down) { PostQuitMessage(0); }
}

void HandleWheelButton(Button* button, bool down)
{
	if (!down) { return; }

	MOUSEINPUT mouseInput;
	POINT mousePos;
	GetCursorPos(&mousePos);
	mouseInput.dx = mousePos.x;
	mouseInput.dy = mousePos.y;
	mouseInput.mouseData = WHEEL_DELTA * button->extras.wheel.direction * button->extras.wheel.amount;
	mouseInput.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL;
	mouseInput.time = 0;
	mouseInput.dwExtraInfo = 0;

	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = mouseInput;

	SendInput(1, &input, sizeof(INPUT));
}

void HandleUpDown(Button* button, bool down)
{
	switch (button->type)
	{
	case BTN_KEY:
		HandleKeyButton(button, down);
		break;
	case BTN_WHEEL:
		HandleWheelButton(button, down);
		break;
	case BTN_QUIT:
		HandleQuitButton(button, down);
		break;
	}
}

LRESULT CALLBACK OnTouch(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BUTTON(hWnd, button);
	TOUCHINPUT touch; //Only handle the first touch on any button

	if (GetTouchInputInfo((HTOUCHINPUT)lParam, 1, &touch, sizeof(TOUCHINPUT)))
	{
		if (touch.dwFlags & (TOUCHEVENTF_DOWN | TOUCHEVENTF_UP))
		{
			HandleUpDown(button, touch.dwFlags & TOUCHEVENTF_DOWN);
		}

		CloseTouchInputHandle((HTOUCHINPUT)lParam);
		return 0;
	}
	else
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

LRESULT CALLBACK OnMouse(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(wParam);
	UNUSED(lParam);

	if ((GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH)
	{
		// This is a fake mouse event
		return 0;
	}

	BUTTON(hWnd, button);
	HandleUpDown(button, uMsg == WM_LBUTTONDOWN);

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(((LPCREATESTRUCT)lParam)->lpCreateParams));
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		return Paint(hWnd, uMsg, wParam, lParam);
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_TOUCH:
		return OnTouch(hWnd, uMsg, wParam, lParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		return OnMouse(hWnd, uMsg, wParam, lParam);
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNUSED(hPrevInstance);
	UNUSED(lpCmdLine);
	UNUSED(nCmdShow);

	ProgramState state;
	ParseError err;
	if (!ParseKeyMap(__argc > 1 ? __argv[1] : "config.ini", &state.gamepad, &err))
	{
		char msgBuff[MAX_ERROR_LENGTH];
		_snprintf(msgBuff, MAX_ERROR_LENGTH, "%s\nLine: %d", err.message, err.line);
		MessageBox(NULL, msgBuff, "Error while loading config", MB_OK);
		return -1;
	}

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = &WindowProc;
	wc.lpszClassName = "TouchJoy";
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_HAND);
	RegisterClass(&wc);

	for (int i = 0; i < state.gamepad.numButtons; ++i)
	{
		Button* button = &state.gamepad.buttons[i];

		HWND hwnd = CreateWindowEx(
			WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE, // Ex styles
			"TouchJoy", // Class name
			button->name, // Title
			WS_VISIBLE | WS_POPUP, // Styles
			button->x, button->y, // Position
			button->width, button->height, // Size
			NULL, // Parent
			NULL, // Menu
			hInstance,
			button // Extra param
		);
		ShowWindow(hwnd, SW_SHOW);
		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA | LWA_COLORKEY);
		RegisterTouchWindow(hwnd, TWF_FINETOUCH | TWF_WANTPALM);

		button->window = hwnd;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

#ifdef _TEST

#include "utest.h"

TEST(parse_ini)
{
	Gamepad gamepad;
	ParseError err;
	int hit = 0;

	TEST_ASSERT(ParseKeyMap("simple.ini", &gamepad, &err));
	TEST_ASSERT_EQUAL_INT(2, gamepad.numButtons);

	for (int i = 0; i < gamepad.numButtons; ++i)
	{
		Button button = gamepad.buttons[i];
		if (STR_EQUAL(button.name, "up"))
		{
			++hit;
			TEST_ASSERT_EQUAL_INT(30, button.x);
			TEST_ASSERT_EQUAL_INT(60, button.y);
		}

		if (STR_EQUAL(button.name, "down"))
		{
			++hit;
			TEST_ASSERT_EQUAL_INT(40, button.x);
			TEST_ASSERT_EQUAL_INT(30, button.y);
		}
	}

	TEST_ASSERT_EQUAL_INT(2, hit);
}

TEST(parse_ini_fail)
{
	Gamepad gamepad;
	ParseError err;

	TEST_ASSERT(!ParseKeyMap("notfound.ini", &gamepad, &err));
	TEST_ASSERT_EQUAL_INT(0, err.line);

	TEST_ASSERT(!ParseKeyMap("fail.ini", &gamepad, &err));
	TEST_ASSERT_EQUAL_INT(2, err.line);
}

TEST_FIXTURE_BEGIN(all)
	TEST_FIXTURE_TEST(parse_ini)
	TEST_FIXTURE_TEST(parse_ini_fail)
TEST_FIXTURE_END()

int main()
{
	return utest_run_fixture(all);
}

#endif