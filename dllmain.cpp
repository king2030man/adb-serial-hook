#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cctype>

// تعريف المؤشرات للدوال الأصلية في الويندوز
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;

HANDLE hSerialPort = INVALID_HANDLE_VALUE;

// دالة كتابة اللوج إلى ملف نصي
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

// دالة الاعتراض لفتح المنفذ
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    if (pOriginalCreateFileW == NULL) {
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
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

// دالة الاعتراض لكتابة الأوامر
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (pOriginalWriteFile == NULL) {
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");
    }

    if (hFile == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE) {
        LogSerialData("COMMAND SENT", (BYTE*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// كتابة التوجيه اليدوي بدون مكتبات خارجية (Hotpatching)
void InstallHook(const char* functionName, LPVOID hookedFunction) {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    void* pTarget = (void*)GetProcAddress(hKernel32, functionName);
    if (!pTarget) return;

    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    // حفظ أول 5 بايتات للدالة الأصلية للقفز إليها لاحقاً
    BYTE jump[5] = { 0xE9, 0, 0, 0, 0 };
    DWORD relativeAddress = (DWORD)hookedFunction - (DWORD)pTarget - 5;
    memcpy(&jump[1], &relativeAddress, 4);
    memcpy(pTarget, jump, 5);
    
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // الحصول على الدوال الأصلية أولاً قبل التعديل عليها
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(hKernel32, "WriteFile");

        // تثبيت الاعتراض تلقائياً بالاعتماد على النظام فقط
        InstallHook("CreateFileW", (LPVOID)HookedCreateFileW);
        InstallHook("WriteFile", (LPVOID)HookedWriteFile);
    }
    return TRUE;
}
