#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <direct.h>
#include <vector>
using namespace std;
#define MAX_SYM_NAME          260
FILE* DumpFile;
BOOL CALLBACK EnumSymProc(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	UNREFERENCED_PARAMETER(UserContext);
	//printf("%08X %4u %s\n", pSymInfo->Address, SymbolSize, pSymInfo->Name);
	char szUndName[MAX_SYM_NAME];
	if (UnDecorateSymbolName(pSymInfo->Name, szUndName, sizeof(szUndName), UNDNAME_COMPLETE))
	{
		// UnDecorateSymbolName returned success
		fprintf(DumpFile, "%08X %4u %s\t%s\n", pSymInfo->Address, SymbolSize, pSymInfo->Name, szUndName);
	}
	else
	{
		// UnDecorateSymbolName failed
		DWORD error = GetLastError();
		fprintf(DumpFile, "%08X %4u %s\t", pSymInfo->Address, SymbolSize, pSymInfo->Name);
		fprintf(DumpFile, "UnDecorateSymbolName returned error %d\n", error);
	}

	return TRUE;
}
BOOL CALLBACK EnumModules(
	PCTSTR  ModuleName,
	DWORD64 BaseOfDll,
	PVOID   UserContext)
{
	UNREFERENCED_PARAMETER(UserContext);
	printf("%08X %s\n", BaseOfDll, ModuleName);
	fprintf(DumpFile, "%08X %s\n", BaseOfDll, ModuleName);

	return TRUE;
}

void Parse(const char* filePath, const char* outPath)
{
	//Refer 
	fopen_s(&DumpFile, outPath, "w");

	DWORD  error;

	DWORD64 BaseOfDll;
	const char* Mask = "*";
	BOOL status;
	SymSetOptions(SYMOPT_DEFERRED_LOADS);
	HANDLE hProcess = GetCurrentProcess();
	status = SymInitialize(hProcess, NULL, FALSE);
	if (status == FALSE)
	{
		error = GetLastError();
		printf("SymInitialize returned error : %d\n", error);
		return;
	}

	BaseOfDll = SymLoadModuleEx(hProcess,
		NULL,
		filePath,
		NULL,
		0,
		0,
		NULL,
		0);

	if (BaseOfDll == 0)
	{
		SymCleanup(hProcess);
		return;
	}
	printf("Enumerating Modules\n");
	fprintf(DumpFile, "Enumerating Modules\n");
	if (SymEnumerateModules64(hProcess, EnumModules, NULL))
	{
		// SymEnumerateModules64 returned success
	}
	else
	{
		// SymEnumerateModules64 failed
		error = GetLastError();
		printf("SymEnumerateModules64 returned error : %d\n"), error);
	}

	ULONG64 buffer[(sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;
	vector<string> symbolNames;

	symbolNames.push_back("?BufferBegin@CommonString@Unity@@3QEBDEB"); //rttiData
	symbolNames.push_back("?GenerateTypeTree@@YAXAEBVObject@@AEAVTypeTree@@W4TransferInstructionFlags@@@Z");
	symbolNames.push_back("?Produce@Object@@CAPEAV1@PEBVType@Unity@@0HUMemLabelId@@W4ObjectCreationMode@@@Z"); //Object__Produce
	symbolNames.push_back("??0TypeTree@@QEAA@AEBUMemLabelId@@@Z"); //TypeTree__TypeTree
	symbolNames.push_back("??0TypeTree@@QEAA@UMemLabelId@@@Z"); //unityVersion
	symbolNames.push_back("?GameEngineVersion@PlatformWrapper@UnityEngine@@SAPEBDXZ"); //ptr to globalBuff?
	symbolNames.push_back("?Application_Get_Custom_PropUnityVersion@@YAPEAUMonoString@@XZ"); //ptr to globalBuff?
	symbolNames.push_back("?ms_runtimeTypes@RTTI@@0URuntimeTypeArray@1@A"); //rttiData
	symbolNames.push_back("FindOrCreateSerializedTypeForUnityType"); //rttiData
	symbolNames.push_back("Debug_Get_Custom_PropIsDebugBuild"); //rttiData
	symbolNames.push_back("?Debug_Get_Custom_PropIsDebugBuild@@YAEXZ"); //rttiData
	fprintf(DumpFile, "\nSymbols by name\n");
	for (auto& symbolName : symbolNames) {
		if (SymFromName(hProcess, symbolName.c_str(), pSymbol))
		{

			fprintf(DumpFile, "%08X %4u %s\n",
				pSymbol->Address, pSymbol->Size, pSymbol->Name);
		}
		else
		{
			// SymFromName failed
			DWORD error = GetLastError();
			fprintf(DumpFile, "SymFromName returned error for symbol %s: %d\n",
				symbolName.c_str(), error);
		}
	}

	printf("Enumerating Symbols\n");
	fprintf(DumpFile, "\nEnumerating Symbols\n");
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
		printf("SymEnumSymbols failed: %d\n", GetLastError());
	}

	SymCleanup(hProcess);
	fclose(DumpFile);
	return;
}
void main(int argc, char** argv) {
	if (argc < 2) {
		printf("SymbolReader SourceFile [OutputFile]");
	}
	else if (argc == 2) {
		Parse(argv[1], "Symbols.txt");
	}
	else {
		Parse(argv[1], argv[2]);
	}
}