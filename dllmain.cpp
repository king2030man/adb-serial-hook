#define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sspi.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include "MinHook.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "bcrypt.lib")

// ==========================================
// 1. تضمين دوال winmm.dll (للخداع)
// ==========================================
#pragma comment(linker, "/export:CloseDriver=C:\\Windows\\System32\\winmm.CloseDriver")
#pragma comment(linker, "/export:DefDriverProc=C:\\Windows\\System32\\winmm.DefDriverProc")
#pragma comment(linker, "/export:DriverCallback=C:\\Windows\\System32\\winmm.DriverCallback")
#pragma comment(linker, "/export:DrvGetModuleHandle=C:\\Windows\\System32\\winmm.DrvGetModuleHandle")
#pragma comment(linker, "/export:GetDriverModuleHandle=C:\\Windows\\System32\\winmm.GetDriverModuleHandle")
#pragma comment(linker, "/export:OpenDriver=C:\\Windows\\System32\\winmm.OpenDriver")
#pragma comment(linker, "/export:SendDriverMessage=C:\\Windows\\System32\\winmm.SendDriverMessage")
#pragma comment(linker, "/export:PlaySoundA=C:\\Windows\\System32\\winmm.PlaySoundA")
#pragma comment(linker, "/export:PlaySoundW=C:\\Windows\\System32\\winmm.PlaySoundW")
// (أضف بقية دوال winmm كما في الكود السابق لضمان عدم تعطل الأداة)
#pragma comment(linker, "/export:mciSendStringA=C:\\Windows\\System32\\winmm.mciSendStringA")
#pragma comment(linker, "/export:mciSendStringW=C:\\Windows\\System32\\winmm.mciSendStringW")
#pragma comment(linker, "/export:waveOutWrite=C:\\Windows\\System32\\winmm.waveOutWrite")
#pragma comment(linker, "/export:auxOutMessage=C:\\Windows\\System32\\winmm.auxOutMessage")
#pragma comment(linker, "/export:midiOutShortMsg=C:\\Windows\\System32\\winmm.midiOutShortMsg")

// ==========================================
// 2. تعريفات الأنواع (هاتف + تشفير)
// ==========================================
typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *CloseHandle_t)(HANDLE);
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

// دوال التشفير الحديثة (CNG)
typedef NTSTATUS (NTAPI *BCryptEncrypt_t)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
typedef NTSTATUS (NTAPI *BCryptDecrypt_t)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);

CreateFileW_t pOriginalCreateFileW = NULL;
CreateFileA_t pOriginalCreateFileA = NULL;
WriteFile_t pOriginalWriteFile = NULL;
CloseHandle_t pOriginalCloseHandle = NULL;
DeviceIoControl_t pOriginalDeviceIoControl = NULL;
BCryptEncrypt_t pOriginalBCryptEncrypt = NULL;
BCryptDecrypt_t pOriginalBCryptDecrypt = NULL;

// ==========================================
// 3. إدارة مقابض الهاتف (نفس الكود السابق)
// ==========================================
#define MAX_HANDLES 200
struct HandleInfo { HANDLE h; wchar_t portName[256]; bool headerWritten; };
HandleInfo monitoredHandles[MAX_HANDLES] = {0};

void AddHandle(HANDLE h, LPCWSTR name) {
    if (h == NULL || h == INVALID_HANDLE_VALUE) return;
    for(int i=0; i<MAX_HANDLES; i++) {
        if(monitoredHandles[i].h == NULL) {
            monitoredHandles[i].h = h;
            monitoredHandles[i].headerWritten = false;
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
bool IsFirstWrite(HANDLE h) {
    for(int i=0; i<MAX_HANDLES; i++) { 
        if(monitoredHandles[i].h == h) { 
            if (monitoredHandles[i].headerWritten) return false;
            monitoredHandles[i].headerWritten = true;
            return true;
        } 
    }
    return false;
}
void RemoveHandle(HANDLE h) {
    for(int i=0; i<MAX_HANDLES; i++) {
        if(monitoredHandles[i].h == h) {
            monitoredHandles[i].h = NULL;
            monitoredHandles[i].headerWritten = false;
            return;
        }
    }
}

// ==========================================
// 4. دوال كتابة اللوق
// ==========================================
#define LOG_DIR "C:\\Users\\Public\\tsm_monitor"
#define PHONE_LOG_FILE "C:\\Users\\Public\\tsm_monitor\\phone_log.txt"
#define SSL_LOG_FILE "C:\\Users\\Public\\tsm_monitor\\ssl_log.txt"

void LogPhoneData(HANDLE h, const wchar_t* portName, const char* buffer, DWORD bufferSize) {
    if (bufferSize == 0 || buffer == NULL) return;
    if (portName != NULL && (wcsstr(portName, L"COM") || wcsstr(portName, L"USB") || wcsstr(portName, L"usb"))) {
        int printableCount = 0;
        for (DWORD i = 0; i < bufferSize; i++) { if (isprint(buffer[i]) || isspace(buffer[i])) printableCount++; }
        if (bufferSize > 10 && (printableCount * 100 / bufferSize) < 20) return;
    }
    CreateDirectoryA(LOG_DIR, NULL);
    FILE* logFile;
    fopen_s(&logFile, PHONE_LOG_FILE, "a+");
    if (logFile) {
        if (IsFirstWrite(h)) {
            time_t now = time(0); tm tstruct; char buf[80];
            localtime_s(&tstruct, &now);
            strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
            fprintf(logFile, "************************************\nTime: %s\n", buf);
            if (portName != NULL) {
                char portNameA[256];
                WideCharToMultiByte(CP_ACP, 0, portName, -1, portNameA, 256, NULL, NULL);
                fprintf(logFile, "Port: %s\n", portNameA);
            } else { fprintf(logFile, "Port: UNKNOWN\n"); }
            fprintf(logFile, "Data:\n");
        }
        for (DWORD i = 0; i < bufferSize; i++) {
            if (isprint(buffer[i]) || buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') fprintf(logFile, "%c", buffer[i]);
            else fprintf(logFile, "\\x%02X", (BYTE)buffer[i]);
        }
        fprintf(logFile, "\n"); fclose(logFile);
    }
}

void LogSSLData(const char* type, const char* buffer, DWORD bufferSize) {
    if (bufferSize == 0 || buffer == NULL) return;
    CreateDirectoryA(LOG_DIR, NULL);
    FILE* logFile;
    fopen_s(&logFile, SSL_LOG_FILE, "a+");
    if (logFile) {
        time_t now = time(0); tm tstruct; char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        fprintf(logFile, "************************************\nTime: %s\nType: %s\nData:\n", buf, type);
        for (DWORD i = 0; i < bufferSize; i++) {
            if (isprint(buffer[i]) || buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') fprintf(logFile, "%c", buffer[i]);
            else fprintf(logFile, "\\x%02X", (BYTE)buffer[i]);
        }
        fprintf(logFile, "\n"); fclose(logFile);
    }
}

// ==========================================
// 5. هوكات الهاتف (COM/USB)
// ==========================================
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName) {
        if ((wcsstr(lpFileName, L"COM") || wcsstr(lpFileName, L"usb#")) && !wcsstr(lpFileName, L"C:\\")) AddHandle(hFile, lpFileName);
    }
    return hFile;
}
HANDLE WINAPI HookedCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName) {
        if ((strstr(lpFileName, "COM") || strstr(lpFileName, "usb#")) && !strstr(lpFileName, "C:\\")) {
            wchar_t wPath[256];
            MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wPath, 256);
            AddHandle(hFile, wPath);
        }
    }
    return hFile;
}
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hFile)) {
        wchar_t portName[256];
        if (GetPortName(hFile, portName)) LogPhoneData(hFile, portName, (const char*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
BOOL WINAPI HookedCloseHandle(HANDLE hObject) {
    if (IsMonitored(hObject)) {
        CreateDirectoryA(LOG_DIR, NULL);
        FILE* logFile;
        fopen_s(&logFile, PHONE_LOG_FILE, "a+");
        if (logFile) { fprintf(logFile, "****************************************\n"); fclose(logFile); }
        RemoveHandle(hObject);
    }
    return pOriginalCloseHandle(hObject);
}
BOOL WINAPI HookedDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hDevice) && lpInBuffer != NULL && nInBufferSize > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) LogPhoneData(hDevice, portName, (const char*)lpInBuffer, nInBufferSize);
    }
    BOOL result = pOriginalDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    if (IsMonitored(hDevice) && lpOutBuffer != NULL && lpBytesReturned != NULL && *lpBytesReturned > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) LogPhoneData(hDevice, portName, (const char*)lpOutBuffer, *lpBytesReturned);
    }
    return result;
}

// ==========================================
// 6. هوكات التشفير (اختراق metacomm.dll)
// ==========================================
NTSTATUS NTAPI HookedBCryptEncrypt(BCRYPT_KEY_HANDLE hKey, PUCHAR pbInput, DWORD cbInput, VOID *pPaddingInfo, PUCHAR pbIV, DWORD cbIV, PUCHAR pbOutput, DWORD cbOutput, DWORD *pcbResult, ULONG dwFlags) {
    // التقاط البيانات قبل تشفيرها (النص الواضح)
    if (pbInput != NULL && cbInput > 0) {
        LogSSLData("SEND (Plain Text before Encrypt)", (const char*)pbInput, cbInput);
    }
    return pOriginalBCryptEncrypt(hKey, pbInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags);
}

NTSTATUS NTAPI HookedBCryptDecrypt(BCRYPT_KEY_HANDLE hKey, PUCHAR pbInput, DWORD cbInput, VOID *pPaddingInfo, PUCHAR pbIV, DWORD cbIV, PUCHAR pbOutput, DWORD cbOutput, DWORD *pcbResult, ULONG dwFlags) {
    // استدعاء الدالة الأصلية لفك التشفير
    NTSTATUS status = pOriginalBCryptDecrypt(hKey, pbInput, cbInput, pPaddingInfo, pbIV, cbIV, pbOutput, cbOutput, pcbResult, dwFlags);
    
    // إذا نجح فك التشفير، نلتقط البيانات الواضحة (رد السيرفر)
    if (status == 0 && pbOutput != NULL && pcbResult != NULL && *pcbResult > 0) {
        LogSSLData("RECEIVE (Plain Text after Decrypt)", (const char*)pbOutput, *pcbResult);
    }
    return status;
}

// ==========================================
// 7. نقطة الدخول
// ==========================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        
        FILE* testFile; 
        fopen_s(&testFile, "C:\\Users\\Public\\test_inject.txt", "w");
        if (testFile) { 
            fprintf(testFile, "DLL Injected Successfully!\n"); 
            
            if (MH_Initialize() == MH_OK) {
                // هوكات الهاتف
                MH_CreateHookApi(L"kernel32.dll", "CreateFileA", &HookedCreateFileA, (LPVOID*)&pOriginalCreateFileA);
                MH_CreateHookApi(L"kernel32.dll", "CreateFileW", &HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW);
                MH_CreateHookApi(L"kernel32.dll", "WriteFile", &HookedWriteFile, (LPVOID*)&pOriginalWriteFile);
                MH_CreateHookApi(L"kernel32.dll", "CloseHandle", &HookedCloseHandle, (LPVOID*)&pOriginalCloseHandle);
                MH_CreateHookApi(L"kernel32.dll", "DeviceIoControl", &HookedDeviceIoControl, (LPVOID*)&pOriginalDeviceIoControl);
                
                // هوكات التشفير الحديثة (CNG)
                MH_CreateHookApi(L"bcrypt.dll", "BCryptEncrypt", &HookedBCryptEncrypt, (LPVOID*)&pOriginalBCryptEncrypt);
                MH_CreateHookApi(L"bcrypt.dll", "BCryptDecrypt", &HookedBCryptDecrypt, (LPVOID*)&pOriginalBCryptDecrypt);
                
                MH_EnableHook(MH_ALL_HOOKS);
                fprintf(testFile, "All Hooks Enabled (Phone + Crypto).\n");
            }
            fclose(testFile); 
        }
    }
    return TRUE;
}
