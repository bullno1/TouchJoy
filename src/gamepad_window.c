#include "gamepad_window.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <windowsx.h>

#include "utils.h"

#define MOUSEEVENTF_FROMTOUCH 0xFF515700
#define BUTTON(HWND, VAR) \
	Button* VAR = (Button*)GetWindowLongPtr(HWND, GWLP_USERDATA);

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
		return OnMouse(hWnd, uMsg, wParam, lParam);
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
		SetLayeredWindowAttributes(hwnd, button->colorKey, 255, LWA_ALPHA | LWA_COLORKEY);
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