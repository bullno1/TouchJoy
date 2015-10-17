#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <ShlObj.h>

#include "gamepad.h"
#include "gamepad_window.h"
#include "utils.h"

#define WM_CONFIGCHANGED (WM_USER + 1)

typedef struct
{
	Gamepad gamepad;
	char configFile[MAX_PATH];
} ProgramState;

void ShowParseError(ParseError err)
{
	char msgBuff[MAX_ERROR_LENGTH];
	_snprintf(
		msgBuff,
		MAX_ERROR_LENGTH,
		"%s\nLine: %d",
		err.message, err.line
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
	{
		char dirName[MAX_PATH];
		memcpy(dirName, state.configFile, namePart - state.configFile);
		dirName[dirLength] = 0;
		SetCurrentDirectory(dirName);
	}

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

	HWND notifyWindow = CreateWindow(
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

	SHChangeNotifyEntry configEntry;
	{
		wchar_t wcFullPath[MAX_PATH];
		MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED,
			state.configFile, -1,
			wcFullPath, MAX_PATH
		);
		PIDLIST_ABSOLUTE list;
		if (SUCCEEDED(SHParseDisplayName(wcFullPath, NULL, &list, 0, NULL)))
		{
			configEntry.fRecursive = false;
			configEntry.pidl = list;
			SHChangeNotifyRegister(
				notifyWindow,
				SHCNRF_ShellLevel | SHCNRF_NewDelivery,
				SHCNE_UPDATEITEM,
				WM_CONFIGCHANGED,
				1,
				&configEntry
				);
		}
	}

	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

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