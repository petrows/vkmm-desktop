 
#include "updater.h"
#include <TlHelp32.h>

updater::updater() {} // Does not really called

void updater::run()
{
	// Default values...
	appUpdaterPath = new TCHAR[MAX_PATH];
	GetModuleFileName(0, appUpdaterPath, MAX_PATH);
	for (int x=lstrlen(appUpdaterPath); x>0; x--)
		if (appUpdaterPath[x] == L'\\' || appUpdaterPath[x] == L'/') { appUpdaterPath[x] = 0; break; }
	
	appExe = new TCHAR[MAX_PATH];
	wsprintf(appExe, L"%s\\vkmm.exe", appUpdaterPath);
	appExePath = new TCHAR[MAX_PATH];
	wsprintf(appExePath, L"%s", appUpdaterPath);

	// Get current exe path
	TCHAR ** stt;
	
	// App base path
	stt = readFileStrings(L"config.msdos.vkmmpath");	
	if (NULL != stt)
	{
		wsprintf(appExe, L"%s\\vkmm.exe", *stt);
		wsprintf(appExePath, L"%s", *stt);
	} else {
		// Error - no config file... Run app!
		execApp();
		return;
	}

	// Kill App!
	killApp();

	// Okay, get files list path
	appUpdaterFilesPath = new TCHAR[MAX_PATH];
	wsprintf(appUpdaterFilesPath, L"%s\\files", appUpdaterPath);

	// Copy files!
	copyFiles();

	// Registry?
	TCHAR updateRegFile[MAX_PATH];
	wsprintf(updateRegFile, L"%s\\regitry.reg", appUpdaterPath);
	if (INVALID_FILE_ATTRIBUTES != ::GetFileAttributes(updateRegFile))
	{
		wsprintf(updateRegFile, L"/S \"%s\\regitry.reg\"", appUpdaterPath);
		ShellExecute(NULL, L"open", L"regedit", updateRegFile, NULL, 1);
	}
	// All ok!
	execApp();
}

void updater::execApp()
{
	ShellExecute(NULL, L"open", appExe, L"", NULL, 1);
}

void updater::resError(int code)
{
	TCHAR msgTxt[1024];
	wsprintf(msgTxt, L"Ошибка обновления! Код: 0x%d", code);
	MessageBox(NULL, msgTxt, L"VKMM ошибка обновления", MB_OK | MB_ICONERROR);
	execApp();
}

bool updater::killProcess(DWORD dwProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
    if (hProcess == NULL)
        return FALSE;

    DWORD dwError = ERROR_SUCCESS;

    if (!TerminateProcess(hProcess, (DWORD)-1))
        dwError = GetLastError();

    CloseHandle(hProcess);
    SetLastError(dwError);
    return dwError == ERROR_SUCCESS;
}

void updater::killApp()
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;

	TCHAR * appExeOnlyIn = new TCHAR[MAX_PATH];
	TCHAR * appExeOnly = new TCHAR[MAX_PATH];
	wsprintf(appExeOnlyIn, L"%s", appExe);
	wsprintf(appExeOnly, L"%s", appExe);
	for (int x=0; x<lstrlen(appExeOnlyIn); x++)
		if (appExeOnlyIn[x] == L'\\' || appExeOnlyIn[x] == L'/') { appExeOnly = appExeOnlyIn + x + 1; continue; }

	DWORD my_pid = GetCurrentProcessId();

	if(!hSnapshot)
		return;

	pe.dwSize = sizeof(pe);
	for(int i = Process32First(hSnapshot, &pe); i; i=Process32Next(hSnapshot, &pe))
	{
		// Take a snapshot of all modules in the specified process		
		TCHAR * fileNameIn = new TCHAR[MAX_PATH];
		TCHAR * fileName = new TCHAR[MAX_PATH];
		wsprintf(fileNameIn, L"%s", pe.szExeFile);
		wsprintf(fileName, L"%s", pe.szExeFile);
		for (int x=0; x<lstrlen(fileNameIn); x++)
			if (fileNameIn[x] == L'\\' || fileNameIn[x] == L'/') { fileName = fileNameIn + x + 1; continue; }

		TCHAR filePath[MAX_PATH];
		wsprintf(filePath, L"%s", pe.szExeFile);
		for (int x=lstrlen(filePath); x>0; x--)
			if (filePath[x] == L'\\' || filePath[x] == L'/') { filePath[x] = 0; break; }

		bool killExe = false;

		if (!killExe && lstrcmp(fileName,appExeOnly) == 0)
		{
			killExe = true;
		} else {
			// MessageBox(0, fileName,appExeOnly, 0);
		}
		if (!killExe && lstrcmp(filePath,appExePath) == 0)
		{
			killExe = true;
		}

		delete[] fileName;

		if (killExe)
		{
			killProcess(pe.th32ProcessID);
			CloseHandle(hSnapshot);
			Sleep(100);
			killApp();
			return; // Go recurse for 100% kill!
		}
	}
	CloseHandle(hSnapshot);
	return;
}

void updater::copyFiles()
{
	TCHAR ** filesList = readFileStrings(L"config.msdos.files");
	if (NULL == filesList) return; // WTF?

	TCHAR * fileNamePtr;
	for (; NULL != *filesList; filesList++)
	{
		fileNamePtr = *filesList;
		// MessageBox(0, fileNamePtr, L"File", 0);
		createDir(fileNamePtr);
		TCHAR pathFileOut[MAX_PATH];
		wsprintf(pathFileOut, L"%s\\%s", appExePath, fileNamePtr);
		TCHAR pathFileIn[MAX_PATH];
		wsprintf(pathFileIn, L"%s\\files\\%s", appUpdaterPath, fileNamePtr);
		// Copy The File
		if (!CopyFile(pathFileIn, pathFileOut, false))
		{
			resError(8800000+GetLastError()); return;
		}
	}
}

void updater::createDir(TCHAR * path)
{
	int pathSize = 0;
	TCHAR pathCurrent[MAX_PATH];
	TCHAR pathCreate[MAX_PATH];
	TCHAR pathCreateFull[MAX_PATH];
	lstrcpy(pathCurrent, path);
	for (int x=0; x<lstrlen(pathCurrent); x++)
	{
		int slashFound = -1;
		for (int y=x; y<lstrlen(pathCurrent); y++)
		{
			if (L'\\' == pathCurrent[y] || L'/' == pathCurrent[y]) { slashFound = y; x = y+1; break; }
		}
		if (-1 == slashFound) break;
		lstrcpyn(pathCreate, pathCurrent, slashFound+1);
		wsprintf(pathCreateFull, L"%s\\%s", appExePath, pathCreate);
		CreateDirectory(pathCreateFull, NULL);		
	}
}

TCHAR ** updater::readFileStrings(TCHAR * path)
{	
	TCHAR outPath[MAX_PATH];
	wsprintf(outPath, L"%s\\%s", appUpdaterPath, path);
	HANDLE hFile = CreateFile (outPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (!hFile || INVALID_HANDLE_VALUE == hFile) return NULL;

	DWORD fileSize = SetFilePointer(hFile, 0, 0, FILE_END);
	if (0 == fileSize || INVALID_FILE_SIZE == fileSize) { CloseHandle(hFile); return NULL; }

	TCHAR * fileBuf = new TCHAR [fileSize];
	
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);	
	DWORD dwRead;
	::ReadFile(hFile, fileBuf, fileSize, &dwRead, NULL);
	CloseHandle(hFile);

	// Okay, we have done reading...
	// Now do file-split - count num of lines
	fileSize = fileSize/sizeof(TCHAR); // UTF-16 - fuck it off
	int numOfLines = 1;
	for (unsigned int x=0; x<fileSize; x++)
	{
		if (L'\n' == fileBuf[x]) numOfLines++;
	}
	TCHAR ** out = new TCHAR*[numOfLines+1];
	for (int x=0; x<numOfLines+1; x++) out[x] = NULL;
	int lineIndex = 0;
	int lineCharIndex = 0;
	for (unsigned int x=0; x<fileSize; x++)
	{
		if (NULL == out[lineIndex]) 
		{
			// New line - create buffer
			// Detect size of line
			int lineSize = fileSize-x;
			for (unsigned int y=x,n=0; y<fileSize-x; y++,n++) if (L'\n' == fileBuf[y]) { lineSize = y; break; }
			out[lineIndex] = new TCHAR[lineSize+1];
			for (int y=0; y<=lineSize; y++) out[lineIndex][y] = 0;
		}
		if (L'\n' == fileBuf[x]) { lineIndex++; lineCharIndex = 0; continue; }
		if (L'\r' == fileBuf[x]) continue; // Piss-off
		out[lineIndex][lineCharIndex] = fileBuf[x];
		out[lineIndex][lineCharIndex+1] = 0;
		lineCharIndex++;
	}
	return out;
}