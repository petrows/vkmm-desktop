 
#include <windows.h>
#include "../resource.h"

class updater
{
public:
	updater();

	void run();

	void execApp();
	void resError(int code);

	TCHAR ** readFileStrings(TCHAR * path);
	void createDir(TCHAR * fileName);

	bool killProcess(DWORD id);
	void killApp();
	void copyFiles();

	TCHAR * appExe;
	TCHAR * appExePath;
	TCHAR * appUpdaterPath;
	TCHAR * appUpdaterFilesPath;
};