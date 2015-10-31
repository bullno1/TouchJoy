#include "gamepad_window.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <windowsx.h>

#include "utils.h"

#define MOUSEEVENTF_FROMTOUCH 0xFF515700
#define BUTTON(HWND, VAR) \
	Button* VAR = (Button*)GetWindowLongPtr(HWND, GWLP_USERDATA);

typedef enum
{
	TOUCH_DOWN,
	TOUCH_UP,
	TOUCH_MOVE
} TouchEvent;

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
	BitBlt(hdc, 0, 0, button->width, button->height, buttonDC, 0, 0, SRCCOPY);
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

	// Scrolling is a bit tricky.
	// First we need to move the mouse to the target area.
	// Then we simulate a scroll event.
	INPUT inputs[2];

	// Move the mouse to a point slightly above and to the left of the button's
	// top left corner
	inputs[0].type = INPUT_MOUSE;
	int x = GetButtonX(button) - 5;
	int y = GetButtonY(button) - 5;
	// Windows uses a weird coordinate system for mouse: [0, 65535]
	int absX = (int)((float)x / (float)GetSystemMetrics(SM_CXSCREEN) * 65535.f);
	int absY = (int)((float)y / (float)GetSystemMetrics(SM_CYSCREEN) * 65535.f);
	inputs[0].mi.dx = absX;
	inputs[0].mi.dy = absY;
	inputs[0].mi.mouseData = 0;
	inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	inputs[0].mi.time = 0;
	inputs[0].mi.dwExtraInfo = 0;

	// Scroll
	inputs[1].type = INPUT_MOUSE;
	inputs[1].mi.dx = 0;
	inputs[1].mi.dy = 0;
	inputs[1].mi.mouseData = WHEEL_DELTA * button->extras.wheel.direction * button->extras.wheel.amount;
	inputs[1].mi.dwFlags = MOUSEEVENTF_WHEEL;
	inputs[1].mi.time = 0;
	inputs[1].mi.dwExtraInfo = 0;

	SendInput(2, inputs, sizeof(INPUT));
}

void HandleStickButton(Button* button, TouchEvent event, int touchX, int touchY)
{
	float joyX, joyY;

	if (event == TOUCH_UP)
	{
		// If the touch is released, the stick moves to its center position
		joyX = 0.f;
		joyY = 0.f;
	}
	else
	{
		// In other cases, use the real touch position to calculate stick
		// position

		joyX = (float)touchX / (float)button->width * 2.f - 1.f;
		joyY = (float)touchY / (float)button->height * 2.f - 1.f;
	}

	bool newStates[4];
	float threshold = button->extras.stick.threshold;
	newStates[STICK_UP]    = joyY < -threshold;
	newStates[STICK_DOWN]  = joyY >  threshold;
	newStates[STICK_LEFT]  = joyX < -threshold;
	newStates[STICK_RIGHT] = joyX >  threshold;

	INPUT inputs[4];
	int numInputs = 0;

	for (int i = 0; i < 4; ++i)
	{
		if (newStates[i] != button->extras.stick.states[i])
		{
			INPUT* input = &inputs[numInputs++];
			input->type = INPUT_KEYBOARD;
			input->ki.wVk = button->extras.stick.codes[i];
			input->ki.dwFlags = newStates[i] ? 0 : KEYEVENTF_KEYUP;
			input->ki.wScan = 0;
			input->ki.time = 0;
			input->ki.dwExtraInfo = 0;
		}

		button->extras.stick.states[i] = newStates[i];
	}

	SendInput(numInputs, inputs, sizeof(INPUT));
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
		if (button->type == BTN_STICK)
		{
			TouchEvent event;
			if (touch.dwFlags & TOUCHEVENTF_DOWN)
			{
				event = TOUCH_DOWN;
			}
			else if (touch.dwFlags & TOUCHEVENTF_UP)
			{
				event = TOUCH_UP;
			}
			else
			{
				event = TOUCH_MOVE;
			}

			int clientX = touch.x / 100 - GetButtonX(button);
			int clientY = touch.y / 100 - GetButtonY(button);
			HandleStickButton(button, event, clientX, clientY);
		}
		else if (touch.dwFlags & (TOUCHEVENTF_DOWN | TOUCHEVENTF_UP))
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

bool IsFakeMouseEvent()
{
	return (GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH;
}

LRESULT CALLBACK OnMouseButton(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(wParam);

	if (IsFakeMouseEvent()) { return 0; }

	BUTTON(hWnd, button);
	if (button->type == BTN_STICK)
	{
		TouchEvent event = uMsg == WM_LBUTTONDOWN ? TOUCH_DOWN : TOUCH_UP;
		HandleStickButton(
			button, event, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)
		);
	}
	else
	{
		HandleUpDown(button, uMsg == WM_LBUTTONDOWN);
	}

	return 0;
}

LRESULT CALLBACK OnMouseMove(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(uMsg);

	if (IsFakeMouseEvent()) { return 0; }

	BUTTON(hWnd, button);
	if ((button->type == BTN_STICK) && (wParam & MK_LBUTTON))
	{
		HandleStickButton(
			button, TOUCH_MOVE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)
		);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		SetWindowLongPtr(
			hWnd,
			GWLP_USERDATA,
			(LONG_PTR)(((LPCREATESTRUCT)lParam)->lpCreateParams)
		);
		return 0;
	case WM_PAINT:
		return Paint(hWnd, uMsg, wParam, lParam);
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_TOUCH:
		return OnTouch(hWnd, uMsg, wParam, lParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		return OnMouseButton(hWnd, uMsg, wParam, lParam);
	case WM_MOUSEMOVE:
		return OnMouseMove(hWnd, uMsg, wParam, lParam);
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

void RegisterGamepadWindowClass()
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = &WindowProc;
	wc.lpszClassName = "TouchJoy";
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_HAND);
	RegisterClass(&wc);
}

void InitializeGamepad(Gamepad* gamepad)
{
	for (int i = 0; i < gamepad->numButtons; ++i)
	{
		Button* button = &gamepad->buttons[i];

		HWND hwnd = CreateWindowEx(
			WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE, // Ex styles
			"TouchJoy", // Class name
			button->name, // Title
			WS_VISIBLE | WS_POPUP, // Styles
			GetButtonX(button), GetButtonY(button), // Position
			button->width, button->height, // Size
			NULL, // Parent
			NULL, // Menu
			GetModuleHandle(NULL),
			button // Extra param
		);
		ShowWindow(hwnd, SW_SHOW);
		SetLayeredWindowAttributes(
			hwnd, button->colorKey, 180, LWA_ALPHA | LWA_COLORKEY
		);
		RegisterTouchWindow(hwnd, TWF_FINETOUCH | TWF_WANTPALM);

		button->window = hwnd;
	}
}

void DeinitializeGamepad(Gamepad* gamepad)
{
	for (int i = 0; i < gamepad->numButtons; ++i)
	{
		Button* button = &gamepad->buttons[i];

		if (button->window)
		{
			DestroyWindow(button->window);
			button->window = 0;
		}
	}
}