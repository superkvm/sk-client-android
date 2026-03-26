

#include "missing.h"

#include <config.h>
#include <libusbi.h>

#include <windows.h>


#define ENVIRONMENT_REG_PATH _T("Software\\libusb\\environment")


char *getenv(const char *name)
{
	static char value[MAX_PATH];
	TCHAR wValue[MAX_PATH];
	WCHAR wName[MAX_PATH];
	DWORD dwType, dwData;
	HKEY hkey;
	LONG rc;

	if (!name)
		return NULL;

	if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wName, MAX_PATH) <= 0) {
		usbi_dbg("Failed to convert environment variable name to wide string");
		return NULL;
	}
	wName[MAX_PATH - 1] = 0; 

	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ENVIRONMENT_REG_PATH, 0, KEY_QUERY_VALUE, &hkey);
	if (rc != ERROR_SUCCESS) {
		usbi_dbg("Failed to open registry key for getenv with error %d", rc);
		return NULL;
	}

	
	dwData = sizeof(wValue);
	rc = RegQueryValueEx(hkey, wName, NULL, &dwType,
		(LPBYTE)&wValue, &dwData);
	RegCloseKey(hkey);
	if (rc != ERROR_SUCCESS) {
		usbi_dbg("Failed to read registry key value for getenv with error %d", rc);
		return NULL;
	}
	if (dwType != REG_SZ) {
		usbi_dbg("Registry value was of type %d instead of REG_SZ", dwType);
		return NULL;
	}

	
	if (WideCharToMultiByte(CP_UTF8, 0,
			wValue, dwData / sizeof(*wValue),
			value, MAX_PATH,
			NULL, NULL) <= 0) {
		usbi_dbg("Failed to convert environment variable value to narrow string");
		return NULL;
	}
	value[MAX_PATH - 1] = 0; 
	return value;
}
