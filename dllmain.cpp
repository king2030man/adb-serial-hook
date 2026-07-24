#include <cwchar>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>

#if defined(__has_include)
#  if __has_include(<windows.h>)
#    include <windows.h>
#  else
#    define NO_WINDOWS_H
#  endif
#else
#  include <windows.h>
#endif

#ifdef NO_WINDOWS_H
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef wchar_t WCHAR;
typedef const WCHAR* LPCWSTR;
typedef unsigned long DWORD;
typedef int BOOL;
typedef void* LPSECURITY_ATTRIBUTES;
typedef void* LPOVERLAPPED;
typedef DWORD* LPDWORD;
typedef unsigned char BYTE;
#define WINAPI __stdcall
#define APIENTRY __stdcall
#endif

// MinHook API declarations to avoid requiring the MinHook header include path
typedef int MH_STATUS;
#define MH_OK 0
#define MH_ALL_HOOKS ((LPVOID)-1)

extern "C" {
    MH_STATUS WINAPI MH_Initialize();
    MH_STATUS WINAPI MH_CreateHookApi(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID* ppOriginal);
    MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget);
}

// تعريف المؤشرات للدوال الأصلية في الويندوز
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;

// متغير لحفظ مقبض (Handle) منفذ السيريال الذي تفتحه الأداة
HANDLE hSerialPort = INVALID_HANDLE_VALUE;

// دالة كتابة اللوج إلى ملف نصي
void LogSerialData(const char* label, const BYTE* buffer, DWORD bufferSize) {
    FILE* logFile;
    fopen_s(&logFile, "C:\\Program Files (x86)\\OneClick-Tool\\serial_log.txt", "a+");
    if (logFile) {
        fprintf(logFile, "[%s]: ", label);
        for (DWORD i = 0; i < bufferSize; i++) {
            // حفظ البيانات كـ Hex ونص عادي في نفس الوقت
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

// دالة الاعتراض الخاصة بـ CreateFileW (لمعرفة هل الأداة تفتح منفذ COM)
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
    // التحقق إذا كان الاسم يحتوي على "\\.\" أو "COM" (صيغة منافذ السيريال في الويندوز)
    if (lpFileName && wcsstr(lpFileName, L"COM")) {
        hSerialPort = hFile; // حفظ المقبض لتتبعه في دالة الكتابة
        
        // تسجيل اسم المنفذ الذي تم فتحه
        char logMsg[100];
        sprintf_s(logMsg, "Opened Serial Port: %ws", lpFileName);
        LogSerialData("INFO", (BYTE*)logMsg, strlen(logMsg));
    }
    return hFile;
}

// دالة الاعتراض الخاصة بـ WriteFile (هنا يتم صيد الأوامر المرسلة للهاتف)
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    // إذا كانت الأداة تكتب داخل منفذ السيريال الذي رصدناه سابقاً
    if (hFile == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE) {
        // اقتناص الأمر وحفظه فوراً في الملف النصي
        LogSerialData("COMMAND SENT", (BYTE*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// تفعيل خطة الاعتراض عند تشغيل الـ DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // تهيئة مكتبة MinHook
        if (MH_Initialize() == MH_OK) {
            // عمل Hook لدالة CreateFileW
            MH_CreateHookApi(L"kernel32", "CreateFileW", &HookedCreateFileW, reinterpret_cast<LPVOID*>(&pOriginalCreateFileW));
            // عمل Hook لدالة WriteFile
            MH_CreateHookApi(L"kernel32", "WriteFile", &HookedWriteFile, reinterpret_cast<LPVOID*>(&pOriginalWriteFile));
            
            // تشغيل الـ Hooks
            MH_EnableHook(MH_ALL_HOOKS);
        }
    }
    return TRUE;
}
