#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cctype>

// توجيه دوال الويندوز الأصلية لملف version.dll الحقيقي لمنع الخطأ 0xc000007b
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
    fopen_s(&logFile, "C:\\Program Files (x86)\\OneClick-Tool\\serial_log.txt", "a+");
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
    if (pOriginalCreateFileW == NULL) {
        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFileW");
    }
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName && wcsstr(lpFileName, L"COM")) {
        hSerialPort = hFile;
        char logMsg[100];
        sprintf_s(logMsg, "Opened Serial Port: %ws", lpFileName);
        LogSerialData("INFO", (BYTE*)logMsg, (DWORD)strlen(logMsg));
    }
    return hFile;
}

BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (pOriginalWriteFile == NULL) {
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WriteFile");
    }
    if (hFile == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE) {
        LogSerialData("COMMAND SENT", (BYTE*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

void InstallHook(const char* functionName, LPVOID hookedFunction) {
    void* pTarget = (void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), functionName);
    if (!pTarget) return;
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    BYTE jump[5] = { 0xE9, 0, 0, 0, 0 };
    DWORD relativeAddress = (DWORD)hookedFunction - (DWORD)pTarget - 5;
    memcpy(&jump[1], &relativeAddress, 4);
    memcpy(pTarget, jump, 5);
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFileW");
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WriteFile");
        InstallHook("CreateFileW", (LPVOID)HookedCreateFileW);
        InstallHook("WriteFile", (LPVOID)HookedWriteFile);
    }
    return TRUE;
}
