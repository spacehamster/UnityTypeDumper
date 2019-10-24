// UnityStructGen - Dump the Unity Runtime base type info
// Compile with /MT(d), NOT /MD(d) (i.e. use static-linked runtime)
// Tested with VS2015 x64.
//
// To use:
// Compile Inject.cpp:
//   $ cl /O2 /Zi /MT Inject.cpp
// Compile UnityStructGen.cpp:
//   $ cl /O2 /Zi /MT /LD UnityStructGen.cpp
//
// 64-bit player now.  Clear relocatable flag, use the debugger env variable.
// ...
//   $ Inject.exe Player.exe UnityStructGen.dll
// This should write some files and crash the player.

// --- Types ---


/*

Variable Name	IDA Name	Offset
globalBuf					aAabb_0																										0x141137870
GenerateTypeTree			GenerateTypeTree(Objectconst &,TypeTree &,TransferInstructionFlags)											0x1406F99F0
Object__Produce				Object::Produce(Unity::Type const *,Unity::Type const *,int,MemLabelIdentifier,ObjectCreationMode)			0x1401F9B60
TypeTree__TypeTree			TypeTree::TypeTree(MemLabelIdentifier)																		0x1406F6D60
unityVersion				a2017_1_0p5																									0x141189490
rttiData					RTTI::RuntimeTypeArray RTTI::ms_runtimeTypes																0x1414C0AA0

*/


// This allows compiling the in Debug mode, since Debug and Release normally
// affect the STL container layout:
#define _ITERATOR_DEBUG_LEVEL 0

#include <Windows.h>
#include <DbgHelp.h>

#include <cstdlib>
#include <string>
#include <list>
#include <memory>
#include <vector>
#include <stddef.h>
#include <stdint.h>

#include "Log.h"
#include "DumpSymbols.h"

// Fake allocator to allow us to interop with UnityPlayer's STL objects.
template <class T = char>
class stl_allocator : public std::allocator<T>
{
public:
	template<class _Other>
	struct rebind
	{
		typedef stl_allocator<_Other> other;
	};

	stl_allocator()
	{
	}
	stl_allocator(const std::allocator<T>&)
	{
	}
private:
	int rootref;
};

typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char>> TypeTreeString;

struct Object__RTTI
{
	Object__RTTI* base;
	void* factory;
	int classId;
	TypeTreeString className;
	int size;
	bool isAbstract;
	bool unk0;
	bool unk1;
};

struct TypeTreeNode
{
	int16_t m_Version;
	int8_t m_Depth;
	int8_t m_IsArray;
	int32_t m_Type; // offset; &(1<<31) => uses common buffer; otherwise local buffer
	int32_t m_Name; // same as Type
	int32_t m_ByteSize;
	int32_t m_Index;
	int32_t m_MetaFlag;
};

struct MemLabelId
{
	int id;
	// int *rootref; <-- only in debug builds
};

template <typename T>
struct dynamic_array
{
	T* data;
	MemLabelId label;
	size_t size;
	size_t cap; // < 0 => data is in union{T*, T[N]}
}; // 0x14

const char* globalBuf;
const char* unityVersion;
const char* stringsBuf;
bool isDebug;

struct TypeTree
{
	// +0
	dynamic_array<TypeTreeNode> m_Nodes;
	// +20
	dynamic_array<char> m_StringData;
	// +40
	dynamic_array<void*> m_ByteOffsets;
	// +60

	// Recursive dumper
	std::string Dump() const
	{
		std::string result{};
		char debug[512];
		memset(debug, 0, 512);
		for (int i = 0; i < m_Nodes.size; i++) {
			auto& node = m_Nodes.data[i];
			const char* type;
			const char* name;
			if (node.m_Type < 0) {
				type = globalBuf + (0x7fffffff & node.m_Type);
			}
			else {
				type = m_StringData.data + node.m_Type;
			}
			if (node.m_Name < 0) {
				name = globalBuf + (0x7fffffff & node.m_Name);
			}
			else {
				name = m_StringData.data + node.m_Name;
			}
			sprintf_s(debug, "%s %s // ByteSize{%x}, Index{%x}, IsArray{%d}, MetaFlag{%x}",
				type, name, node.m_ByteSize, node.m_Index, node.m_IsArray, node.m_MetaFlag);
			for (int j = 0; j < node.m_Depth; j++) {
				result += "  ";
			}
			result += std::string(debug);
			result += "\n";
		}
		return result;
	}

	// Binary serializer
	void Write(FILE* file) const
	{
		fwrite(&m_Nodes.size, 4, 1, file);
		fwrite(&m_StringData.size, 4, 1, file);
		fwrite(m_Nodes.data, m_Nodes.size * sizeof(TypeTreeNode), 1, file);
		fwrite(m_StringData.data, m_StringData.size, 1, file);
	}
};

class ProxyTransfer;
class Object;
typedef int ObjectCreationMode;

#if 0
typedef Object * (__cdecl * Object__Produce_t)(int classID, int instanceID, MemLabelId memLabel, ObjectCreationMode mode);
typedef Object__RTTI* (__cdecl* Object__ClassIDToRTTI_t)(int classID);
typedef void(__thiscall* TypeTree__TypeTree_t)(TypeTree* self);
#endif

typedef void(__cdecl* GenerateTypeTree_t)(Object* object, TypeTree* typeTree, int options);
typedef Object* (__cdecl* Object__Produce_t)(struct RTTIClass* classInfo, struct RTTIClass* classInfo2, int instanceID, int memLabel, ObjectCreationMode mode);
typedef void(__thiscall* TypeTree__TypeTree_Release_t)(TypeTree* self, int memLabel);
typedef void(__thiscall* TypeTree__TypeTree_Development_t)(TypeTree* self, int * memLabel);
typedef const char*(__cdecl* GameEngineVersion_t)();
typedef void* (__cdecl* Custom_PropUnityVersion_t)();
typedef char* (__cdecl* mono_string_to_utf8_t)(void*);
//struct MonoString* __ptr64 __cdecl Application_Get_Custom_PropUnityVersion(void)

// --- Offsets ---
// 5.1.3p2, x86 Windows; pdbs are nice
#if 0
GenerateTypeTree_t GenerateTypeTree = (GenerateTypeTree_t)(void*)0x6346B0;
Object__Produce_t Object__Produce = (Object__Produce_t)(void*)0x40C5E0;
TypeTree__TypeTree_t TypeTree__TypeTree = (TypeTree__TypeTree_t)(void*)0x634D80;
Object__ClassIDToRTTI_t Object__ClassIDToRTTI = (Object__ClassIDToRTTI_t)(void*)0x4093D0;

MemLabelId* kMemBaseObject = (MemLabelId*)0x12540DC;
#endif
// 5.5.2f1, x64 Windows; pdbs are now less nice because RTTI system changed, but hey it's cool Unity you do you fam
GenerateTypeTree_t GenerateTypeTree;
Object__Produce_t Object__Produce;
TypeTree__TypeTree_Release_t TypeTree__TypeTree_Release;
TypeTree__TypeTree_Development_t TypeTree__TypeTree_Development;
GameEngineVersion_t GameEngineVersion;
Custom_PropUnityVersion_t Custom_PropUnityVersion;
mono_string_to_utf8_t mono_string_to_utf8;

struct RTTIClass {
	RTTIClass* base;
	void* unk1; // probably constructor
	const char* name;
	const char* unk3;
	const char* unk4;
	int classID;
	int objectSize;
	int typeIndex;
	int unk5;
	bool isAbstract;
	bool unk6;
	bool unk7;
};
struct RTTIData {
	size_t length;
	RTTIClass* data[1];
};
RTTIData* rttiData;

RTTIClass* classFromClassID(int classID) {
	for (size_t i = 0; i < rttiData->length; i++) {
		RTTIClass* curr = rttiData->data[i];
		if (!curr) continue;
		if (classID == curr->classID) return curr;
	}
	return nullptr;
}

// --- Dumper ---
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

FILE* dumpFile, * treeFile, * stringsFile, * jsonFile;

bool GenStruct(int classID)
{
	RTTIClass* rtti = classFromClassID(classID);
	if (rtti == nullptr)
	{
		LOG("rtti for %d is null\n", classID);
		return false;
	}
	// Dump the base class chain:
	RTTIClass* base = rtti;
	std::string inheritance{};
	while (true)
	{
		inheritance += base->name;
		base = base->base;
		if (base != NULL)
		{
			inheritance += " <- ";
		}
		else
		{
			break;
		}
	}
	LOG("Inheritence for for %d is %s\n", classID, inheritance.c_str());
	char buf[256];
	sprintf_s(buf, "\n// classID{%d}: %s\n", classID, inheritance.c_str());
	LOG("Buf for for %d is %s\n", classID, buf);
	fputs(buf, dumpFile);
	// Go up the inheritance chain until we find some data to dump:
	int newClassID = classID;
	while (rtti->isAbstract)
	{
		sprintf_s(buf, "// %s is abstract\n", rtti->name);
		LOG("Buf for for %d is %s\n", newClassID, buf);
		fputs(buf, dumpFile);
		rtti = rtti->base;
		if (rtti == NULL)
		{
			return false;
		}
		newClassID = rtti->classID;
	}
	LOG("Calling Object::Produce for %s\n", rtti->name);
	Object* value = Object__Produce(rtti, rtti, 0x32, 0, 0);
	LOG("Produced object %p for %d\n", value, classID);
	// We can't call our TypeTree dtor since we haven't properly implemented
	// stl_allocator and ~std::list() will crash.
	// If this leak is concerning, link the dtor func (::~TypeTree()) and
	// call it explicitly.
	TypeTree* typeTree = (TypeTree*)malloc(sizeof(TypeTree));
	LOG("Calling TypeTree::TypeTree for %d\n", classID);
	if (isDebug) {
		//TODO: this is probably broken
		int label = 0x32;
		TypeTree__TypeTree_Development(typeTree, &label);
	}
	else {
		TypeTree__TypeTree_Release(typeTree, 0x32);
	}
	LOG("Calling GenerateTypeTree for %d\n", classID);
	GenerateTypeTree(value, typeTree, 0x2000);
	fputs(typeTree->Dump().c_str(), dumpFile);
	fflush(dumpFile);
	fwrite(&newClassID, 4, 1, treeFile);
	char fakeGuid[0x20];
	memset(fakeGuid, 0, 0x20);
	if (newClassID < 0) {
		fwrite(fakeGuid, 0x20, 1, treeFile);
	}
	else {
		fwrite(fakeGuid, 0x10, 1, treeFile);
	}
	typeTree->Write(treeFile);
	if (ftell(jsonFile) > 1) {
		fprintf(jsonFile, ", ");
	}
	fprintf(jsonFile, "\"%d\": \"%s\"", classID, rtti->name);
	return true;
}
ULONG LoadPointer(HANDLE hProcess, const char* symbolName) {

	ULONG64 buffer[(sizeof(SYMBOL_INFO) +
		MAX_SYM_NAME * sizeof(TCHAR) +
		sizeof(ULONG64) - 1) /
		sizeof(ULONG64)];
	PSYMBOL_INFO _pSymbol = (PSYMBOL_INFO)buffer;
	_pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	_pSymbol->MaxNameLen = MAX_SYM_NAME;
	if (SymFromName(hProcess, symbolName, _pSymbol)) {
		return _pSymbol->Address;
	}
	else {
		DWORD error = GetLastError();
		LOG("Error finding symbol %s, error code %d, hProcess %p\n", symbolName, error, hProcess);
		return 0;
	}
}

void LoadPointers() 
{
	HANDLE hProcess = GetCurrentProcess();
	DWORD64 BaseOfDll;
	SymSetOptions(SYMOPT_DEFERRED_LOADS);
	BOOL status = SymInitialize(hProcess, NULL, FALSE);
	if (!status) {
		LOG("Error initializing DbgHelp\n");
	}
	BaseOfDll = SymLoadModuleEx(hProcess,
		NULL,
		"UnityPlayer.dll",
		NULL,
		0,
		0,
		NULL,
		0);
	HMODULE baseAddress = GetModuleHandleA("UnityPlayer.dll");
	LOG("Loaded DbgHelper - BaseOfDll %llX BaseAddress %p\n", BaseOfDll, baseAddress);
	ULONG offset;

	offset = LoadPointer(hProcess, "?Application_Get_Custom_PropUnityVersion@@YAPEAUMonoString@@XZ");
	Custom_PropUnityVersion = (Custom_PropUnityVersion_t)(void*)(baseAddress + (offset - 0x80000000) / 4);
	LOG("LoadedPointer: Custom_PropUnityVersion %08X, %p\n", offset, Custom_PropUnityVersion);

	//This gets passed in by the loader
	if (unityVersion == NULL) {

		//This method is not supported on older versions of unity
		//public: static char const * __ptr64 __cdecl UnityEngine::PlatformWrapper::GameEngineVersion(void)
		//offset = LoadPointer(hProcess, "?GameEngineVersion@PlatformWrapper@UnityEngine@@SAPEBDXZ");
		//GameEngineVersion = (GameEngineVersion_t)(const char*)(baseAddress + (offset - 0x80000000) / 4);
		//LOG("LoadedPointer: GameEngineVersion %08X, %p\n", offset, GameEngineVersion);
		//unityVersion = GameEngineVersion();
		//LOG("LoadedPointer: unityVersion %08X, %p\n", offset, unityVersion);

		offset = LoadPointer(hProcess, "mono_string_to_utf8");
		mono_string_to_utf8 = (mono_string_to_utf8_t)(char*)(baseAddress + (offset - 0x80000000) / 4);
		LOG("LoadedPointer: mono_string_to_utf8 %08X, %p\n", offset, mono_string_to_utf8);

		void* msUnityVersion = Custom_PropUnityVersion();
		LOG("Loaded MonoString from Custom_PropUnityVersion: %p\n", msUnityVersion);

		//TODO: Calls to mono_string_to_utf8 cause a crash
		char* propUnityVersion = mono_string_to_utf8(msUnityVersion);
		LOG("Loaded version from Custom_PropUnityVersion: %s\n", propUnityVersion);
	}

	//void __cdecl GenerateTypeTree(class Object const & __ptr64,class TypeTree & __ptr64,enum TransferInstructionFlags)
	offset = LoadPointer(hProcess, "?GenerateTypeTree@@YAXAEBVObject@@AEAVTypeTree@@W4TransferInstructionFlags@@@Z");
	GenerateTypeTree = (GenerateTypeTree_t)(void*)(baseAddress + (offset - 0x80000000) / 4);
	LOG("LoadedPointer: GenerateTypeTree %08X, %p\n", offset, GenerateTypeTree);

	//private: static class Object * __ptr64 __cdecl Object::Produce(class Unity::Type const * __ptr64,class Unity::Type const * __ptr64,int,struct MemLabelId,enum ObjectCreationMode)
	offset = LoadPointer(hProcess, "?Produce@Object@@CAPEAV1@PEBVType@Unity@@0HUMemLabelId@@W4ObjectCreationMode@@@Z"); //Object::Produce
	Object__Produce = (Object__Produce_t)(void*)(baseAddress + (offset - 0x80000000) / 4);
	LOG("LoadedPointer: Object::Produce %08X, %p\n", offset, Object__Produce);

	if (isDebug) {
		//public: __cdecl TypeTree::TypeTree(struct MemLabelId const & __ptr64) __ptr64
		offset = LoadPointer(hProcess, "??0TypeTree@@QEAA@AEBUMemLabelId@@@Z");
		TypeTree__TypeTree_Development = (TypeTree__TypeTree_Development_t)(void*)(baseAddress + (offset - 0x80000000) / 4);
		LOG("LoadedPointer: TypeTree__TypeTree_Development %08X, %p\n", offset, TypeTree__TypeTree_Development);
	}
	else {
		//public: __cdecl TypeTree::TypeTree(struct MemLabelId) __ptr64
		offset = LoadPointer(hProcess, "??0TypeTree@@QEAA@UMemLabelId@@@Z");
		TypeTree__TypeTree_Release = (TypeTree__TypeTree_Release_t)(void*)(baseAddress + (offset - 0x80000000) / 4);
		LOG("LoadedPointer: TypeTree__TypeTree_Release %08X, %p\n", offset, TypeTree__TypeTree_Release);
	}

	//private: static struct RTTI::RuntimeTypeArray RTTI::ms_runtimeTypes
	offset = LoadPointer(hProcess, "?ms_runtimeTypes@RTTI@@0URuntimeTypeArray@1@A");
	rttiData = (RTTIData*)(baseAddress + (offset - 0x80000000) / 4);
	LOG("LoadedPointer: RTTI::ms_runtimeTypes %08X, %p\n", offset, rttiData);

	//char const * __ptr64 const __ptr64 Unity::CommonString::BufferBegin
	offset = LoadPointer(hProcess, "?BufferBegin@CommonString@Unity@@3QEBDEB");
	const char** bufferBegin = (const char**)(baseAddress + (offset - 0x80000000) / 4);
	LOG("LoadedPointer: Unity::CommonString::BufferBegin %08X, %p\n", offset, bufferBegin);

	globalBuf = *bufferBegin;
	stringsBuf = *bufferBegin;

	LOG("unityVersion: %s\n", unityVersion);

	LOG("Cleaning up\n");
	SymCleanup(hProcess);
}
extern "C"
{
	__declspec(dllexport) void Dump(const char* _unityVersion, bool _isDebug) {
		unityVersion = _unityVersion;
		isDebug = _isDebug;
		InitLogger();
		LOG("hello from UnityStructGen\n");
		LoadPointers();
		LOG("Calculated addresses\n");
		fopen_s(&dumpFile, "structs.dump", "wb");
		fopen_s(&treeFile, "structs.dat", "wb");
		fopen_s(&stringsFile, "strings.dat", "wb");
		fopen_s(&jsonFile, "classes.json", "w");
		const int targetPlatform = 5;
		const bool hasTypeTrees = 1;
		fwrite(unityVersion, 1, 1 + strlen(unityVersion), treeFile);
		LOG("Wrote unity version\n");
		fwrite(&targetPlatform, 4, 1, treeFile);
		LOG("Wrote targetPlatform\n");
		fwrite(&hasTypeTrees, 1, 1, treeFile);
		LOG("Wrote hasTypeTrees\n");
		fpos_t countPos;
		fgetpos(treeFile, &countPos);
		int typeCount = 0;
		fwrite(&typeCount, 4, 1, treeFile);
		LOG("Wrote typeCount\n");
		fprintf(jsonFile, "{");
		for (int classId = 0; classId < 1024; classId++)
		{
			if (GenStruct(classId))
			{
				typeCount++;
			}
		}
		int zero = 0;

		LOG("Generated structs. Found %d types.\n", typeCount);
		fwrite(&zero, 4, 1, treeFile);
		fsetpos(treeFile, &countPos);
		fwrite(&typeCount, 4, 1, treeFile);
		while (strlen(stringsBuf)) {
			fwrite(stringsBuf, 1, 1 + strlen(stringsBuf), stringsFile);
			stringsBuf += 1 + strlen(stringsBuf);
		}
		fprintf(jsonFile, "}");
		fclose(treeFile);
		fclose(dumpFile);
		fclose(stringsFile);
		fclose(jsonFile);

		LOG("Finished dumping\n");
		FreeLogger();
		DebugBreak();
	}
}
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason != DLL_PROCESS_ATTACH)
	{
		return TRUE;
	}
	return TRUE;
}