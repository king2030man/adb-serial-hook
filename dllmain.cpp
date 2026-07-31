خطأ غريب جداً! ما حدث هو أنك قمت بنسخ **رسالة الخطأ القديمة** (التي تحتوي على `Run $sources` و `error C2001`) ولصقتها داخل ملف الكود `dllmain.cpp` في GitHub! 

مترجم C++ حاول أن يقرأ رسالة الخطأ وكأنها كود برمجي، فطبع لك هذه الأخطاء العشوائية.

### الحل:
1. اذهب إلى ملف `dllmain.cpp` في GitHub.
2. امسح كل ما بداخله (اجعله فارغاً تماماً، تأكد أنه لا يوجد فيه أي حرف من رسائل الخطأ).
3. انسخ **فقط** الكود الموجود داخل المربع الأسود بالأسفل (بدون أي كلام قبله أو بعده)، والصقه في الملف.

إليك الكود النظيف مرة أخرى لتسهيل النسخ:

```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <wininet.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include "MinHook.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")

typedef LONG NTSTATUS;
typedef struct _IO_STATUS_BLOCK { union { LONG Status; PVOID Pointer; }; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef int (WSAAPI *send_t)(SOCKET, const char*, int, int);
typedef int (WSAAPI *recv_t)(SOCKET, char*, int, int);
typedef BOOL (WINAPI *WinHttpSendRequest_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *WinHttpReadData_t)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL (WINAPI *HttpSendRequestW_t)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD);
typedef BOOL (WINAPI *InternetReadFile_t)(HINTERNET, LPVOID, DWORD, LPDWORD);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;
DeviceIoControl_t pOriginalDeviceIoControl = NULL;
NtWriteFile_t pOriginalNtWriteFile = NULL;
send_t pOriginalSend = NULL;
recv_t pOriginalRecv = NULL;
WinHttpSendRequest_t pOriginalWinHttpSendRequest = NULL;
WinHttpReadData_t pOriginalWinHttpReadData = NULL;
HttpSendRequestW_t pOriginalHttpSendRequestW = NULL;
InternetReadFile_t pOriginalInternetReadFile = NULL;

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
            fprintf(logFile, "Port: INTERNET\n");
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

int WSAAPI HookedSend(SOCKET s, const char* buf, int len, int flags) {
    LogData("NET_SOCKET_SEND", L"INTERNET", buf, len);
    return pOriginalSend(s, buf, len, flags);
}

int WSAAPI HookedRecv(SOCKET s, char* buf, int len, int flags) {
    int result = pOriginalRecv(s, buf, len, flags);
    if (result > 0) LogData("NET_SOCKET_RECV", L"INTERNET", buf, result);
    return result;
}

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        FILE* testFile; 
        fopen_s(&testFile, "C:\\Users\\Public\\test_inject.txt", "w");
        if (testFile) { 
            fprintf(testFile, "DLL Injected Successfully!\n"); 
            
            MH_STATUS initStatus = MH_Initialize();
            fprintf(testFile, "MH_Initialize Status: %d\n", initStatus);
            
            if (initStatus == MH_OK) {
                fprintf(testFile, "Hook ws2_32.send: %d\n", MH_CreateHookApi(L"ws2_32.dll", "send", &HookedSend, (LPVOID*)&pOriginalSend));
                fprintf(testFile, "Hook ws2_32.recv: %d\n", MH_CreateHookApi(L"ws2_32.dll", "recv", &HookedRecv, (LPVOID*)&pOriginalRecv));
                fprintf(testFile, "Hook winhttp.Send: %d\n", MH_CreateHookApi(L"winhttp.dll", "WinHttpSendRequest", &HookedWinHttpSendRequest, (LPVOID*)&pOriginalWinHttpSendRequest));
                fprintf(testFile, "Hook winhttp.Read: %d\n", MH_CreateHookApi(L"winhttp.dll", "WinHttpReadData", &HookedWinHttpReadData, (LPVOID*)&pOriginalWinHttpReadData));
                fprintf(testFile, "Hook wininet.Send: %d\n", MH_CreateHookApi(L"wininet.dll", "HttpSendRequestW", &HookedHttpSendRequestW, (LPVOID*)&pOriginalHttpSendRequestW));
                fprintf(testFile, "Hook wininet.Read: %d\n", MH_CreateHookApi(L"wininet.dll", "InternetReadFile", &HookedInternetReadFile, (LPVOID*)&pOriginalInternetReadFile));
                fprintf(testFile, "Hook kernel32.CreateFileW: %d\n", MH_CreateHookApi(L"kernel32.dll", "CreateFileW", &HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW));
                fprintf(testFile, "Hook kernel32.WriteFile: %d\n", MH_CreateHookApi(L"kernel32.dll", "WriteFile", &HookedWriteFile, (LPVOID*)&pOriginalWriteFile));
                fprintf(testFile, "Hook kernel32.DeviceIoControl: %d\n", MH_CreateHookApi(L"kernel32.dll", "DeviceIoControl", &HookedDeviceIoControl, (LPVOID*)&pOriginalDeviceIoControl));
                fprintf(testFile, "Hook ntdll.NtWriteFile: %d\n", MH_CreateHookApi(L"ntdll.dll", "NtWriteFile", &HookedNtWriteFile, (LPVOID*)&pOriginalNtWriteFile));
                
                fprintf(testFile, "MH_EnableHook Status: %d\n", MH_EnableHook(MH_ALL_HOOKS));
            }
            fclose(testFile); 
        }
    }
    return TRUE;
}
```
