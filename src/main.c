#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

#include "gamepad.h"
#include "gamepad_window.h"
#include "utils.h"

#define WM_CONFIGCHANGED (WM_USER + 1)

typedef struct
{
	Gamepad gamepad;
	char configFile[MAX_PATH];
	char configDir[MAX_PATH];
	HANDLE shutdownEvent;
	volatile bool running;
	HWND msgWindow;
} ProgramState;

void ShowParseError(ParseError err)
{
	char msgBuff[MAX_ERROR_LENGTH];
	_snprintf(
		msgBuff,
		MAX_ERROR_LENGTH,
		"%s\nLine: %d",
		err.message,(int)err.line
	);
	MessageBox(NULL, msgBuff, "Error while loading config", MB_OK);
}

void ReloadConfig(ProgramState* state)
{
	Gamepad tempGamepad;
	ParseError parseError;
	if (LoadGamepad(state->configFile, &tempGamepad, &parseError))
	{
		DeinitializeGamepad(&state->gamepad);
		FreeGamepad(&state->gamepad);
		state->gamepad = tempGamepad;
		InitializeGamepad(&state->gamepad);
	}
	else
	{
		ShowParseError(parseError);
	}
}

LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED(hWnd);
	UNUSED(wParam);
	UNUSED(lParam);

	switch (uMsg)
	{
	case WM_CREATE:
		SetWindowLongPtr(
			hWnd,
			GWLP_USERDATA,
			(LONG_PTR)(((LPCREATESTRUCT)lParam)->lpCreateParams)
			);
		return 0;
	case WM_CONFIGCHANGED:
		ReloadConfig((ProgramState*)(GetWindowLongPtr(hWnd, GWLP_USERDATA)));
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

DWORD WINAPI ConfigMonitorProc(LPVOID lpParameter)
{
	ProgramState* state = (ProgramState*)lpParameter;

	HANDLE changeHandle = FindFirstChangeNotification(
		state->configDir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE
	);
	while (state->running)
	{
		HANDLE waitObjs[2];
		waitObjs[0] = changeHandle;
		waitObjs[1] = state->shutdownEvent;
		WaitForMultipleObjects(2, waitObjs, FALSE, INFINITE);

		FindNextChangeNotification(changeHandle);

		PostMessage(state->msgWindow, WM_CONFIGCHANGED, 0, 0);
	}
	FindCloseChangeNotification(changeHandle);

	return 0;
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
)
{
	UNUSED(hPrevInstance);
	UNUSED(lpCmdLine);
	UNUSED(nCmdShow);

	ProgramState state;

	// Calculate full path to config file and its containing directory
	const char* configFile = __argc > 1 ? __argv[1] : "config.ini";
	char* namePart;
	GetFullPathName(configFile, MAX_PATH, state.configFile, &namePart);
	size_t dirLength = namePart - state.configFile;
	memcpy(state.configDir, state.configFile, namePart - state.configFile);
	state.configDir[dirLength] = 0;
	SetCurrentDirectory(state.configDir);

	// Try to load the config file
	ParseError err;
	if (!LoadGamepad(state.configFile, &state.gamepad, &err))
	{
		ShowParseError(err);
		return -1;
	}

	// Display gamepad
	RegisterGamepadWindowClass();
	InitializeGamepad(&state.gamepad);

	// Create a window to receive change notifications
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = &MsgWindowProc;
	wc.lpszClassName = "TouchJoyMessage";
	wc.hInstance = hInstance;
	RegisterClass(&wc);

	state.msgWindow = CreateWindow(
		"TouchJoyMessage", // Class name
		"TouchJoyMessage", // Title
		0, // Styles
		0, 0, // Position
		0, 0, // Size
		HWND_MESSAGE, // Parent
		NULL, // Menu
		hInstance,
		&state // Extra
	);

	// Create a thread to monitor changes to config file
	state.shutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	HANDLE threadHandle = CreateThread(
		NULL, 0, &ConfigMonitorProc, &state, 0, NULL
	);

	// Message loop
	state.running = true;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	state.running = false;

	SetEvent(state.shutdownEvent);
	WaitForSingleObject(threadHandle, INFINITE);
	DeinitializeGamepad(&state.gamepad);
	FreeGamepad(&state.gamepad);

	return (int)msg.wParam;
}

#ifdef _TEST

#include "utest.h"

TEST(parse_ini)
{
	Gamepad gamepad;
	ParseError err;
	int hit = 0;

	TEST_ASSERT(LoadGamepad("simple.ini", &gamepad, &err));
	TEST_ASSERT_EQUAL_INT(2, gamepad.numButtons);

	for (int i = 0; i < gamepad.numButtons; ++i)
	{
		Button button = gamepad.buttons[i];
		if (STR_EQUAL(button.name, "up"))
		{
			++hit;
			TEST_ASSERT_EQUAL_INT(30, GetButtonX(&button));
			TEST_ASSERT_EQUAL_INT(60, GetButtonY(&button));
		}

		if (STR_EQUAL(button.name, "down"))
		{
			++hit;
			TEST_ASSERT_EQUAL_INT(40, GetButtonX(&button));
			TEST_ASSERT_EQUAL_INT(30, GetButtonY(&button));
		}
	}

	TEST_ASSERT_EQUAL_INT(2, hit);
}

TEST(parse_ini_fail)
{
	Gamepad gamepad;
	ParseError err;

	TEST_ASSERT(!LoadGamepad("notfound.ini", &gamepad, &err));
	TEST_ASSERT_EQUAL_INT(0, err.line);

	TEST_ASSERT(!LoadGamepad("fail.ini", &gamepad, &err));
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