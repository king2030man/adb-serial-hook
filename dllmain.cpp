#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cctype>

// توجيه دوال الويندوز الأصلية لملف version.dll الحقيقي لمنع الخطأ
#pragma comment(linker, "/export:GetFileVersionInfoA=C:\\Windows\\System32\\version.GetFileVersionInfoA")
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=C:\\Windows\\System32\\version.GetFileVersionInfoByHandle")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=C:\\Windows\\System32\\version.GetFileVersionInfoSizeA")
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=C:\\Windows\\System32\\version.GetFileVersionInfoSizeW")
#pragma comment(linker, "/export:GetFileVersionInfoW=C:\\Windows\\System32\\version.GetFileVersionInfoW")
#pragma comment(linker, "/export:VerFindFileA=C:\\Windows\\System32\\version.VerFindFileA")
#pragma comment(linker, "/export:VerFindFileW=C:\\Windows\\System32\\version.VerFindFileW")
#pragma comment(linker, "/export:VerInstallFileA=C:\\Windows\\System32\\version.VerInstallFileA")
#pragma comment(linker, "/export:VerInstallFileW=C:\\Windows\\System32\\version.VerInstallFileW")
#pragma comment(linker, "/export:VerLanguageNameA=C:\\Windows\\System32\\version.VerLanguageNameA")
#pragma comment(linker, "/export:VerLanguageNameW=C:\\Windows\\System32\\version.VerLanguageNameW")
#pragma comment(linker, "/export:VerQueryValueA=C:\\Windows\\System32\\version.VerQueryValueA")
#pragma comment(linker, "/export:VerQueryValueW=C:\\Windows\\System32\\version.VerQueryValueW")

typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;
HANDLE hSerialPort = INVALID_HANDLE_VALUE;

void LogSerialData(const char* label, const BYTE* buffer, DWORD bufferSize) {
    FILE* logFile;
    // حفظ اللوج في نفس مجلد التشغيل تلقائياً لتفادي مشاكل الصلاحيات
    fopen_s(&logFile, "serial_log.txt", "a+");
    if (logFile) {
        fprintf(logFile, "[%s]: ", label);
        for (DWORD i = 0; i < bufferSize; i++) {
            if (isprint(buffer[i])) {
                fprintf(logFile, "%c", buffer[i]);
            } else {
                fprintf(logFile, "\\x%02X", buffer[i]);
            }
        }
        fprintf(logFile, "\n");
        fclose(logFile);
    }
}

HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    if (lpFileName && wcsstr(lpFileName, L"COM")) {
        hSerialPort = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        char logMsg[128];
        sprintf_s(logMsg, "Opened Serial Port: %ws", lpFileName);
        LogSerialData("INFO", (BYTE*)logMsg, (DWORD)strlen(logMsg));
        return hSerialPort;
    }
    return pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (hFile == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE) {
        LogSerialData("COMMAND SENT", (BYTE*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// دالة الاعتراض عبر الـ IAT دون تعديل بايتات الذاكرة الأصلية
void IATHook(const char* dllName, const char* funcName, LPVOID hookedFunc, LPVOID* origFunc) {
    HMODULE hMods = GetModuleHandleW(NULL);
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hMods;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return;

    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hMods + pDosHeader->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hMods + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    while (pImportDesc->Name) {
        char* name = (char*)((BYTE*)hMods + pImportDesc->Name);
        if (_stricmp(name, dllName) == 0) {
            PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((BYTE*)hMods + pImportDesc->FirstThunk);
            PIMAGE_THUNK_DATA pOrigThunk = (PIMAGE_THUNK_DATA)((BYTE*)hMods + pImportDesc->OriginalFirstThunk);

            while (pThunk->u1.Function) {
                PROC* pFuncAddr = (PROC*)&pThunk->u1.Function;
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hMods + pOrigThunk->u1.AddressOfData);
                
                if (strcmp((char*)pImportByName->Name, funcName) == 0) {
                    DWORD oldProtect;
                    VirtualProtect(pFuncAddr, sizeof(PROC), PAGE_READWRITE, &oldProtect);
                    *origFunc = (LPVOID)*pFuncAddr;
                    *pFuncAddr = (PROC)hookedFunc;
                    VirtualProtect(pFuncAddr, sizeof(PROC), oldProtect, &oldProtect);
                    return;
                }
                pThunk++;
                pOrigThunk++;
            }
        }
        pImportDesc++;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // جلب العناوين الافتراضية كاحتياط
        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFileW");
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WriteFile");

        // تطبيق الاعتراض الآمن
        IATHook("kernel32.dll", "CreateFileW", (LPVOID)HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW);
        IATHook("kernel32.dll", "WriteFile", (LPVOID)HookedWriteFile, (LPVOID*)&pOriginalWriteFile);
    }
    return TRUE;
}
