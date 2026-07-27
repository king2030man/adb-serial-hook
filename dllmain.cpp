#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cctype>

// توجيه دوال winmm.dll الأصلية لمنع انهيار البرنامج
#pragma comment(linker, "/export:PlaySoundA=C:\\Windows\\System32\\winmm.PlaySoundA")
#pragma comment(linker, "/export:PlaySoundW=C:\\Windows\\System32\\winmm.PlaySoundW")
#pragma comment(linker, "/export:sndPlaySoundA=C:\\Windows\\System32\\winmm.sndPlaySoundA")
#pragma comment(linker, "/export:sndPlaySoundW=C:\\Windows\\System32\\winmm.sndPlaySoundW")
#pragma comment(linker, "/export:waveOutGetNumDevs=C:\\Windows\\System32\\winmm.waveOutGetNumDevs")
#pragma comment(linker, "/export:waveOutOpen=C:\\Windows\\System32\\winmm.waveOutOpen")
#pragma comment(linker, "/export:waveOutWrite=C:\\Windows\\System32\\winmm.waveOutWrite")
#pragma comment(linker, "/export:waveOutClose=C:\\Windows\\System32\\winmm.waveOutClose")
#pragma comment(linker, "/export:midiOutGetNumDevs=C:\\Windows\\System32\\winmm.midiOutGetNumDevs")
#pragma comment(linker, "/export:timeGetTime=C:\\Windows\\System32\\winmm.timeGetTime")
#pragma comment(linker, "/export:timeBeginPeriod=C:\\Windows\\System32\\winmm.timeBeginPeriod")
#pragma comment(linker, "/export:timeEndPeriod=C:\\Windows\\System32\\winmm.timeEndPeriod")
#pragma comment(linker, "/export:joyGetNumDevs=C:\\Windows\\System32\\winmm.joyGetNumDevs")
#pragma comment(linker, "/export:mciSendStringA=C:\\Windows\\System32\\winmm.mciSendStringA")
#pragma comment(linker, "/export:mciSendStringW=C:\\Windows\\System32\\winmm.mciSendStringW")
#pragma comment(linker, "/export:mciGetErrorStringA=C:\\Windows\\System32\\winmm.mciGetErrorStringA")
#pragma comment(linker, "/export:mciGetErrorStringW=C:\\Windows\\System32\\winmm.mciGetErrorStringW")

// تعريف مؤشرات الدوال الأصلية
typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE (WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);

CreateFileW_t pOriginalCreateFileW = NULL;
CreateFileA_t pOriginalCreateFileA = NULL;
WriteFile_t pOriginalWriteFile = NULL;
DeviceIoControl_t pOriginalDeviceIoControl = NULL;

HANDLE hSerialPort = INVALID_HANDLE_VALUE;

// دالة كتابة اللوج
void LogSerialData(const char* label, const BYTE* buffer, DWORD bufferSize) {
    CreateDirectoryA("E:\\adb_recored", NULL);
    CreateDirectoryA("E:\\adb_recored\\oneclike_serial_port_log", NULL);
    
    const char* logPath = "E:\\adb_recored\\oneclike_serial_port_log\\serial_log.txt";
    FILE* logFile;
    fopen_s(&logFile, logPath, "a+");
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

// اعتراض CreateFileW
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName && wcsstr(lpFileName, L"COM")) {
        hSerialPort = hFile;
        char logMsg[128];
        sprintf_s(logMsg, "Opened Serial Port (W): %ws", lpFileName);
        LogSerialData("INFO", (BYTE*)logMsg, (DWORD)strlen(logMsg));
    }
    return hFile;
}

// اعتراض CreateFileA
HANDLE WINAPI HookedCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName && strstr(lpFileName, "COM")) {
        hSerialPort = hFile;
        char logMsg[128];
        sprintf_s(logMsg, "Opened Serial Port (A): %s", lpFileName);
        LogSerialData("INFO", (BYTE*)logMsg, (DWORD)strlen(logMsg));
    }
    return hFile;
}

// اعتراض WriteFile
BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (hFile == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE) {
        LogSerialData("COMMAND (WriteFile)", (BYTE*)lpBuffer, nNumberOfBytesToWrite);
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// اعتراض DeviceIoControl
BOOL WINAPI HookedDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    if (hDevice == hSerialPort && hSerialPort != INVALID_HANDLE_VALUE && nInBufferSize > 0) {
        LogSerialData("COMMAND (IOCTL)", (BYTE*)lpInBuffer, nInBufferSize);
    }
    return pOriginalDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
}

// دالة الحقن الآمنة
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
        
        // === اختبار التحميل ===
        CreateDirectoryA("E:\\adb_recored", NULL);
        CreateDirectoryA("E:\\adb_recored\\oneclike_serial_port_log", NULL);
        FILE* testFile;
        fopen_s(&testFile, "E:\\adb_recored\\oneclike_serial_port_log\\dll_loaded.txt", "w");
        if (testFile) {
            fprintf(testFile, "winmm.dll is successfully loaded!\n");
            fclose(testFile);
        }
        // ======================

        pOriginalCreateFileW = (CreateFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFileW");
        pOriginalCreateFileA = (CreateFileA_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateFileA");
        pOriginalWriteFile = (WriteFile_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "WriteFile");
        pOriginalDeviceIoControl = (DeviceIoControl_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "DeviceIoControl");

        IATHook("kernel32.dll", "CreateFileW", (LPVOID)HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW);
        IATHook("kernel32.dll", "CreateFileA", (LPVOID)HookedCreateFileA, (LPVOID*)&pOriginalCreateFileA);
        IATHook("kernel32.dll", "WriteFile", (LPVOID)HookedWriteFile, (LPVOID*)&pOriginalWriteFile);
        IATHook("kernel32.dll", "DeviceIoControl", (LPVOID)HookedDeviceIoControl, (LPVOID*)&pOriginalDeviceIoControl);
    }
    return TRUE;
}
