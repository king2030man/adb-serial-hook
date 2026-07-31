#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <wininet.h> // تم إضافة مكتبة WinINet البديلة
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include "MinHook.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")

// ==========================================
// 1. تعريفات الأنواع
// ==========================================
typedef LONG NTSTATUS;
typedef struct _IO_STATUS_BLOCK { union { LONG Status; PVOID Pointer; }; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);

// WinHTTP
typedef BOOL (WINAPI *WinHttpSendRequest_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *WinHttpReadData_t)(HINTERNET, LPVOID, DWORD, LPDWORD);
// WinINet
typedef BOOL (WINAPI *HttpSendRequestW_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD);
typedef BOOL (WINAPI *InternetReadFile_t)(HINTERNET, LPVOID, DWORD, LPDWORD);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;
DeviceIoControl_t pOriginalDeviceIoControl = NULL;
NtWriteFile_t pOriginalNtWriteFile = NULL;
WinHttpSendRequest_t pOriginalWinHttpSendRequest = NULL;
WinHttpReadData_t pOriginalWinHttpReadData = NULL;
HttpSendRequestW_t pOriginalHttpSendRequestW = NULL;
InternetReadFile_t pOriginalInternetReadFile = NULL;

// ==========================================
// 2. إدارة مقابض الهاتف
// ==========================================
#define MAX_HANDLES 200
struct HandleInfo { HANDLE h; wchar_t portName[256]; };
HandleInfo monitoredHandles[MAX_HANDLES] = {0};

void AddHandle(HANDLE h, LPCWSTR name) {
    if (h == NULL || h == INVALID_HANDLE_VALUE) return;
    for(int i=0; i<MAX_HANDLES; i++) {
        if(monitoredHandles[i].h == NULL) {
            monitoredHandles[i].h = h;
            if (name) wcscpy_s(monitoredHandles[i].portName, name);
            else wcscpy_s(monitoredHandles[i].portName, L"Unknown");
            return;
        }
    }
}
bool GetPortName(HANDLE h, wchar_t* outName) {
    for(int i=0; i<MAX_HANDLES; i++) { if(monitoredHandles[i].h == h) { wcscpy_s(outName, 256, monitoredHandles[i].portName); return true; } }
    return false;
}
bool IsMonitored(HANDLE h) {
    for(int i=0; i<MAX_HANDLES; i++) { if(monitoredHandles[i].h == h) return true; }
    return false;
}

// ==========================================
// 3. دالة كتابة اللوق (تم تغيير المسار)
// ==========================================
#define LOG_DIR "C:\\Users\\Public\\tsm_monitor"
#define LOG_FILE "C:\\Users\\Public\\tsm_monitor\\combined_log.txt"

void LogData(const char* type, const wchar_t* portName, const char* buffer, DWORD bufferSize) {
    if (bufferSize == 0 || buffer == NULL) return;
    
    if (portName != NULL && (wcsstr(portName, L"COM") || wcsstr(portName, L"USB"))) {
        int printableCount = 0;
        for (DWORD i = 0; i < bufferSize; i++) { if (isprint(buffer[i]) || isspace(buffer[i])) printableCount++; }
        if (bufferSize > 10 && (printableCount * 100 / bufferSize) < 20) return;
    }
    
    CreateDirectoryA(LOG_DIR, NULL);
    FILE* logFile;
    fopen_s(&logFile, LOG_FILE, "a+");
    if (logFile) {
        time_t now = time(0);
        tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        
        fprintf(logFile, "\n********************************\n");
        fprintf(logFile, "Time: %s\n", buf);
        if (portName != NULL) {
            char portNameA[256];
            WideCharToMultiByte(CP_ACP, 0, portName, -1, portNameA, 256, NULL, NULL);
            fprintf(logFile, "Port: %s\n", portNameA);
        } else {
            fprintf(logFile, "Port: INTERNET (HTTPS/INet)\n");
        }
        fprintf(logFile, "Type: %s\n", type);
        fprintf(logFile, "Data: ");
        for (DWORD i = 0; i < bufferSize; i++) {
            if (isprint(buffer[i]) || buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') fprintf(logFile, "%c", buffer[i]);
            else fprintf(logFile, "\\x%02X", (BYTE)buffer[i]);
        }
        fprintf(logFile, "\n");
        fclose(logFile);
    }
}

// ==========================================
// 4. هوكات الهاتف (COM/USB)
// ==========================================
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName) {
        if (wcsstr(lpFileName, L"COM") || wcsstr(lpFileName, L"\\\\?\\")) AddHandle(hFile, lpFileName);
    }
    return hFile;
}
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hFile)) {
        wchar_t portName[256];
        if (GetPortName(hFile, portName)) LogData("PHONE_WRITE", portName, (const char*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
BOOL WINAPI HookedDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hDevice) && lpInBuffer != NULL && nInBufferSize > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) LogData("PHONE_USB_SEND", portName, (const char*)lpInBuffer, nInBufferSize);
    }
    BOOL result = pOriginalDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    if (IsMonitored(hDevice) && lpOutBuffer != NULL && lpBytesReturned != NULL && *lpBytesReturned > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) LogData("PHONE_USB_RESPONSE", portName, (const char*)lpOutBuffer, *lpBytesReturned);
    }
    return result;
}
NTSTATUS NTAPI HookedNtWriteFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) {
    if (IsMonitored(FileHandle) && Buffer != NULL && Length > 0) {
        wchar_t portName[256];
        if (GetPortName(FileHandle, portName)) LogData("PHONE_NT_WRITE", portName, (const char*)Buffer, Length);
    }
    return pOriginalNtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

// ==========================================
// 5. هوكات الشبكة (WinHTTP + WinINet)
// ==========================================
BOOL WINAPI HookedWinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext) {
    if (dwOptionalLength > 0 && lpOptional != NULL) LogData("NET_WINHTTP_SEND", L"INTERNET", (const char*)lpOptional, dwOptionalLength);
    return pOriginalWinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength, dwTotalLength, dwContext);
}
BOOL WINAPI HookedWinHttpReadData(HINTERNET hRequest, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead) {
    BOOL result = pOriginalWinHttpReadData(hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    if (result && lpdwNumberOfBytesRead != NULL && *lpdwNumberOfBytesRead > 0) LogData("NET_WINHTTP_RECV", L"INTERNET", (const char*)lpBuffer, *lpdwNumberOfBytesRead);
    return result;
}
BOOL WINAPI HookedHttpSendRequestW(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength) {
    if (dwOptionalLength > 0 && lpOptional != NULL) LogData("NET_WININET_SEND", L"INTERNET", (const char*)lpOptional, dwOptionalLength);
    return pOriginalHttpSendRequestW(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
}
BOOL WINAPI HookedInternetReadFile(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead) {
    BOOL result = pOriginalInternetReadFile(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
    if (result && lpdwNumberOfBytesRead != NULL && *lpdwNumberOfBytesRead > 0) LogData("NET_WININET_RECV", L"INTERNET", (const char*)lpBuffer, *lpdwNumberOfBytesRead);
    return result;
}

// ==========================================
// 6. نقطة الدخول للـ DLL
// ==========================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        // --- اختبار التحقق من الحقن ---
        // سيتم إنشاء هذا الملف فور حقن الـ DLL للتأكد من أنه يعمل
        FILE* testFile; 
        fopen_s(&testFile, "C:\\Users\\Public\\test_inject.txt", "w");
        if (testFile) { fprintf(testFile, "DLL Injected Successfully!\n"); fclose(testFile); }
        // ------------------------------

        if (MH_Initialize() == MH_OK) {
            // هوكات الهاتف
            MH_CreateHookApi(L"kernel32.dll", "CreateFileW", &HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW);
            MH_CreateHookApi(L"kernel32.dll", "WriteFile", &HookedWriteFile, (LPVOID*)&pOriginalWriteFile);
            MH_CreateHookApi(L"kernel32.dll", "DeviceIoControl", &HookedDeviceIoControl, (LPVOID*)&pOriginalDeviceIoControl);
            MH_CreateHookApi(L"ntdll.dll", "NtWriteFile", &HookedNtWriteFile, (LPVOID*)&pOriginalNtWriteFile);
            
            // هوكات الشبكة
            MH_CreateHookApi(L"winhttp.dll", "WinHttpSendRequest", &HookedWinHttpSendRequest, (LPVOID*)&pOriginalWinHttpSendRequest);
            MH_CreateHookApi(L"winhttp.dll", "WinHttpReadData", &HookedWinHttpReadData, (LPVOID*)&pOriginalWinHttpReadData);
            MH_CreateHookApi(L"wininet.dll", "HttpSendRequestW", &HookedHttpSendRequestW, (LPVOID*)&pOriginalHttpSendRequestW);
            MH_CreateHookApi(L"wininet.dll", "InternetReadFile", &HookedInternetReadFile, (LPVOID*)&pOriginalInternetReadFile);
            
            MH_EnableHook(MH_ALL_HOOKS);
        }
    }
    return TRUE;
}
