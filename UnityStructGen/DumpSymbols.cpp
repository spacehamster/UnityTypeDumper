#include "Log.h"
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <DbgHelp.h>
FILE* DumpFile;
bool FileExists(const char* path) {
	DWORD dwAttrib = GetFileAttributesA(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
BOOL CALLBACK EnumSymProc(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	UNREFERENCED_PARAMETER(UserContext);
	char szUndName[MAX_SYM_NAME];
	if (UnDecorateSymbolName(pSymInfo->Name, szUndName, sizeof(szUndName), UNDNAME_COMPLETE))
	{
		// UnDecorateSymbolName returned success
		fprintf(DumpFile, "%llX %4lu %s\t%s\n", pSymInfo->Address, SymbolSize, pSymInfo->Name, szUndName);
		fflush(DumpFile);
	}
	else
	{
		// UnDecorateSymbolName failed
		DWORD error = GetLastError();
		fprintf(DumpFile, "%llX %4lu %s\t", pSymInfo->Address, SymbolSize, pSymInfo->Name);
		fprintf(DumpFile, "UnDecorateSymbolName returned error %d\n", error);
		fflush(DumpFile);
	}
	return TRUE;
}
BOOL CALLBACK EnumModules(
	PCTSTR  ModuleName,
	DWORD64 BaseOfDll,
	PVOID   UserContext)
{
	UNREFERENCED_PARAMETER(UserContext);
	fprintf(DumpFile, "%llX %s\n", BaseOfDll, ModuleName);
	return TRUE;
}
void DumpSymbols(const char * targetPath, const char * outputPath) {
	fopen_s(&DumpFile, outputPath, "w");
	DWORD  error;
	DWORD64 BaseOfDll;
	BOOL status;
	const char* Mask = "*";
	SymSetOptions(SYMOPT_DEFERRED_LOADS);
	HANDLE hProcess = GetCurrentProcess();
	status = SymInitialize(hProcess, NULL, FALSE);
	if (status == FALSE)
	{
		error = GetLastError();
		LOG("SymInitialize returned error : %d\n", error);
		return;
	}
	BaseOfDll = SymLoadModuleEx(hProcess,
		NULL,
		targetPath,
		NULL,
		0,
		0,
		NULL,
		0);
	if (BaseOfDll == 0)
	{
		error = GetLastError();
		LOG("SymLoadModuleEx returned error : %d\n", error);
		SymCleanup(hProcess);
		return;
	}
	if (SymEnumerateModules64(hProcess, EnumModules, NULL))
	{
		// SymEnumerateModules64 returned success
	}
	else
	{
		error = GetLastError();
		fprintf(DumpFile, "SymEnumerateModules64 returned error : %d\n", error);
	}
	if (SymEnumSymbols(hProcess,     // Process handle from SymInitialize.
		BaseOfDll,   // Base address of module.
		Mask,        // Name of symbols to match.
		EnumSymProc, // Symbol handler procedure.
		NULL))       // User context.
	{
		// SymEnumSymbols succeeded
	}
	else
	{
		// SymEnumSymbols failed
		fprintf(DumpFile, "SymEnumSymbols failed: %d\n", GetLastError());
	}
}
void DumpSymbols() {
	if (FileExists("UnityPlayer.dll") && !FileExists("UnityPlayerSymbols.txt")) {
		DumpSymbols("UnityPlayer.dll", "UnityPlayerSymbols.txt");
	}
	else
	{
		LOG("Could not find UnityPlayer.dll");
	}
	if (FileExists("Types.exe") && !FileExists("ExeSymbols.txt")) {
		DumpSymbols("Types.exe", "ExeSymbols.txt");
	}
	else
	{
		LOG("Could not find Types.exe");
	}
}