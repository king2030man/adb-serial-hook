#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <ctime>
#include "MinHook.h"

// ==========================================
// 1. تضمين جميع دوال winmm.dll الأصلية
// ==========================================
#pragma comment(linker, "/export:CloseDriver=C:\\Windows\\System32\\winmm.CloseDriver")
#pragma comment(linker, "/export:DefDriverProc=C:\\Windows\\System32\\winmm.DefDriverProc")
#pragma comment(linker, "/export:DriverCallback=C:\\Windows\\System32\\winmm.DriverCallback")
#pragma comment(linker, "/export:DrvGetModuleHandle=C:\\Windows\\System32\\winmm.DrvGetModuleHandle")
#pragma comment(linker, "/export:GetDriverModuleHandle=C:\\Windows\\System32\\winmm.GetDriverModuleHandle")
#pragma comment(linker, "/export:OpenDriver=C:\\Windows\\System32\\winmm.OpenDriver")
#pragma comment(linker, "/export:SendDriverMessage=C:\\Windows\\System32\\winmm.SendDriverMessage")
#pragma comment(linker, "/export:auxGetDevCapsA=C:\\Windows\\System32\\winmm.auxGetDevCapsA")
#pragma comment(linker, "/export:auxGetDevCapsW=C:\\Windows\\System32\\winmm.auxGetDevCapsW")
#pragma comment(linker, "/export:auxGetNumDevs=C:\\Windows\\System32\\winmm.auxGetNumDevs")
#pragma comment(linker, "/export:auxGetVolume=C:\\Windows\\System32\\winmm.auxGetVolume")
#pragma comment(linker, "/export:auxOutMessage=C:\\Windows\\System32\\winmm.auxOutMessage")
#pragma comment(linker, "/export:auxSetVolume=C:\\Windows\\System32\\winmm.auxSetVolume")
#pragma comment(linker, "/export:joyConfigChanged=C:\\Windows\\System32\\winmm.joyConfigChanged")
#pragma comment(linker, "/export:joyGetDevCapsA=C:\\Windows\\System32\\winmm.joyGetDevCapsA")
#pragma comment(linker, "/export:joyGetDevCapsW=C:\\Windows\\System32\\winmm.joyGetDevCapsW")
#pragma comment(linker, "/export:joyGetNumDevs=C:\\Windows\\System32\\winmm.joyGetNumDevs")
#pragma comment(linker, "/export:joyGetPos=C:\\Windows\\System32\\winmm.joyGetPos")
#pragma comment(linker, "/export:joyGetPosEx=C:\\Windows\\System32\\winmm.joyGetPosEx")
#pragma comment(linker, "/export:joyGetThreshold=C:\\Windows\\System32\\winmm.joyGetThreshold")
#pragma comment(linker, "/export:joyReleaseCapture=C:\\Windows\\System32\\winmm.joyReleaseCapture")
#pragma comment(linker, "/export:joySetCapture=C:\\Windows\\System32\\winmm.joySetCapture")
#pragma comment(linker, "/export:joySetThreshold=C:\\Windows\\System32\\winmm.joySetThreshold")
#pragma comment(linker, "/export:mciDriverNotify=C:\\Windows\\System32\\winmm.mciDriverNotify")
#pragma comment(linker, "/export:mciDriverYield=C:\\Windows\\System32\\winmm.mciDriverYield")
#pragma comment(linker, "/export:mciExecute=C:\\Windows\\System32\\winmm.mciExecute")
#pragma comment(linker, "/export:mciFreeCommandResource=C:\\Windows\\System32\\winmm.mciFreeCommandResource")
#pragma comment(linker, "/export:mciGetCommandResource=C:\\Windows\\System32\\winmm.mciGetCommandResource")
#pragma comment(linker, "/export:mciGetDeviceIDA=C:\\Windows\\System32\\winmm.mciGetDeviceIDA")
#pragma comment(linker, "/export:mciGetDeviceIDW=C:\\Windows\\System32\\winmm.mciGetDeviceIDW")
#pragma comment(linker, "/export:mciGetErrorStringA=C:\\Windows\\System32\\winmm.mciGetErrorStringA")
#pragma comment(linker, "/export:mciGetErrorStringW=C:\\Windows\\System32\\winmm.mciGetErrorStringW")
#pragma comment(linker, "/export:mciSendCommandA=C:\\Windows\\System32\\winmm.mciSendCommandA")
#pragma comment(linker, "/export:mciSendCommandW=C:\\Windows\\System32\\winmm.mciSendCommandW")
#pragma comment(linker, "/export:mciSendStringA=C:\\Windows\\System32\\winmm.mciSendStringA")
#pragma comment(linker, "/export:mciSendStringW=C:\\Windows\\System32\\winmm.mciSendStringW")
#pragma comment(linker, "/export:mciSetYieldProc=C:\\Windows\\System32\\winmm.mciSetYieldProc")
#pragma comment(linker, "/export:midiConnect=C:\\Windows\\System32\\winmm.midiConnect")
#pragma comment(linker, "/export:midiDisconnect=C:\\Windows\\System32\\winmm.midiDisconnect")
#pragma comment(linker, "/export:midiInAddBuffer=C:\\Windows\\System32\\winmm.midiInAddBuffer")
#pragma comment(linker, "/export:midiInClose=C:\\Windows\\System32\\winmm.midiInClose")
#pragma comment(linker, "/export:midiInGetDevCapsA=C:\\Windows\\System32\\winmm.midiInGetDevCapsA")
#pragma comment(linker, "/export:midiInGetDevCapsW=C:\\Windows\\System32\\winmm.midiInGetDevCapsW")
#pragma comment(linker, "/export:midiInGetErrorTextA=C:\\Windows\\System32\\winmm.midiInGetErrorTextA")
#pragma comment(linker, "/export:midiInGetErrorTextW=C:\\Windows\\System32\\winmm.midiInGetErrorTextW")
#pragma comment(linker, "/export:midiInGetID=C:\\Windows\\System32\\winmm.midiInGetID")
#pragma comment(linker, "/export:midiInGetNumDevs=C:\\Windows\\System32\\winmm.midiInGetNumDevs")
#pragma comment(linker, "/export:midiInMessage=C:\\Windows\\System32\\winmm.midiInMessage")
#pragma comment(linker, "/export:midiInOpen=C:\\Windows\\System32\\winmm.midiInOpen")
#pragma comment(linker, "/export:midiInPrepareHeader=C:\\Windows\\System32\\winmm.midiInPrepareHeader")
#pragma comment(linker, "/export:midiInReset=C:\\Windows\\System32\\winmm.midiInReset")
#pragma comment(linker, "/export:midiInStart=C:\\Windows\\System32\\winmm.midiInStart")
#pragma comment(linker, "/export:midiInStop=C:\\Windows\\System32\\winmm.midiInStop")
#pragma comment(linker, "/export:midiInUnprepareHeader=C:\\Windows\\System32\\winmm.midiInUnprepareHeader")
#pragma comment(linker, "/export:midiOutCacheDrumPatches=C:\\Windows\\System32\\winmm.midiOutCacheDrumPatches")
#pragma comment(linker, "/export:midiOutCachePatches=C:\\Windows\\System32\\winmm.midiOutCachePatches")
#pragma comment(linker, "/export:midiOutClose=C:\\Windows\\System32\\winmm.midiOutClose")
#pragma comment(linker, "/export:midiOutGetDevCapsA=C:\\Windows\\System32\\winmm.midiOutGetDevCapsA")
#pragma comment(linker, "/export:midiOutGetDevCapsW=C:\\Windows\\System32\\winmm.midiOutGetDevCapsW")
#pragma comment(linker, "/export:midiOutGetErrorTextA=C:\\Windows\\System32\\winmm.midiOutGetErrorTextA")
#pragma comment(linker, "/export:midiOutGetErrorTextW=C:\\Windows\\System32\\winmm.midiOutGetErrorTextW")
#pragma comment(linker, "/export:midiOutGetID=C:\\Windows\\System32\\winmm.midiOutGetID")
#pragma comment(linker, "/export:midiOutGetNumDevs=C:\\Windows\\System32\\winmm.midiOutGetNumDevs")
#pragma comment(linker, "/export:midiOutGetVolume=C:\\Windows\\System32\\winmm.midiOutGetVolume")
#pragma comment(linker, "/export:midiOutLongMsg=C:\\Windows\\System32\\winmm.midiOutLongMsg")
#pragma comment(linker, "/export:midiOutMessage=C:\\Windows\\System32\\winmm.midiOutMessage")
#pragma comment(linker, "/export:midiOutOpen=C:\\Windows\\System32\\winmm.midiOutOpen")
#pragma comment(linker, "/export:midiOutPrepareHeader=C:\\Windows\\System32\\winmm.midiOutPrepareHeader")
#pragma comment(linker, "/export:midiOutReset=C:\\Windows\\System32\\winmm.midiOutReset")
#pragma comment(linker, "/export:midiOutSetVolume=C:\\Windows\\System32\\winmm.midiOutSetVolume")
#pragma comment(linker, "/export:midiOutShortMsg=C:\\Windows\\System32\\winmm.midiOutShortMsg")
#pragma comment(linker, "/export:midiOutUnprepareHeader=C:\\Windows\\System32\\winmm.midiOutUnprepareHeader")
#pragma comment(linker, "/export:midiStreamClose=C:\\Windows\\System32\\winmm.midiStreamClose")
#pragma comment(linker, "/export:midiStreamOpen=C:\\Windows\\System32\\winmm.midiStreamOpen")
#pragma comment(linker, "/export:midiStreamOut=C:\\Windows\\System32\\winmm.midiStreamOut")
#pragma comment(linker, "/export:midiStreamPause=C:\\Windows\\System32\\winmm.midiStreamPause")
#pragma comment(linker, "/export:midiStreamPosition=C:\\Windows\\System32\\winmm.midiStreamPosition")
#pragma comment(linker, "/export:midiStreamProperty=C:\\Windows\\System32\\winmm.midiStreamProperty")
#pragma comment(linker, "/export:midiStreamRestart=C:\\Windows\\System32\\winmm.midiStreamRestart")
#pragma comment(linker, "/export:midiStreamStop=C:\\Windows\\System32\\winmm.midiStreamStop")
#pragma comment(linker, "/export:mixerClose=C:\\Windows\\System32\\winmm.mixerClose")
#pragma comment(linker, "/export:mixerGetControlDetailsA=C:\\Windows\\System32\\winmm.mixerGetControlDetailsA")
#pragma comment(linker, "/export:mixerGetControlDetailsW=C:\\Windows\\System32\\winmm.mixerGetControlDetailsW")
#pragma comment(linker, "/export:mixerGetDevCapsA=C:\\Windows\\System32\\winmm.mixerGetDevCapsA")
#pragma comment(linker, "/export:mixerGetDevCapsW=C:\\Windows\\System32\\winmm.mixerGetDevCapsW")
#pragma comment(linker, "/export:mixerGetID=C:\\Windows\\System32\\winmm.mixerGetID")
#pragma comment(linker, "/export:mixerGetLineControlsA=C:\\Windows\\System32\\winmm.mixerGetLineControlsA")
#pragma comment(linker, "/export:mixerGetLineControlsW=C:\\Windows\\System32\\winmm.mixerGetLineControlsW")
#pragma comment(linker, "/export:mixerGetLineInfoA=C:\\Windows\\System32\\winmm.mixerGetLineInfoA")
#pragma comment(linker, "/export:mixerGetLineInfoW=C:\\Windows\\System32\\winmm.mixerGetLineInfoW")
#pragma comment(linker, "/export:mixerGetNumDevs=C:\\Windows\\System32\\winmm.mixerGetNumDevs")
#pragma comment(linker, "/export:mixerMessage=C:\\Windows\\System32\\winmm.mixerMessage")
#pragma comment(linker, "/export:mixerOpen=C:\\Windows\\System32\\winmm.mixerOpen")
#pragma comment(linker, "/export:mixerSetControlDetails=C:\\Windows\\System32\\winmm.mixerSetControlDetails")
#pragma comment(linker, "/export:mmDrvInstall=C:\\Windows\\System32\\winmm.mmDrvInstall")
#pragma comment(linker, "/export:mmGetCurrentTask=C:\\Windows\\System32\\winmm.mmGetCurrentTask")
#pragma comment(linker, "/export:mmTaskBlock=C:\\Windows\\System32\\winmm.mmTaskBlock")
#pragma comment(linker, "/export:mmTaskCreate=C:\\Windows\\System32\\winmm.mmTaskCreate")
#pragma comment(linker, "/export:mmTaskSignal=C:\\Windows\\System32\\winmm.mmTaskSignal")
#pragma comment(linker, "/export:mmTaskYield=C:\\Windows\\System32\\winmm.mmTaskYield")
#pragma comment(linker, "/export:PlaySoundA=C:\\Windows\\System32\\winmm.PlaySoundA")
#pragma comment(linker, "/export:PlaySoundW=C:\\Windows\\System32\\winmm.PlaySoundW")
#pragma comment(linker, "/export:sndPlaySoundA=C:\\Windows\\System32\\winmm.sndPlaySoundA")
#pragma comment(linker, "/export:sndPlaySoundW=C:\\Windows\\System32\\winmm.sndPlaySoundW")
#pragma comment(linker, "/export:timeBeginPeriod=C:\\Windows\\System32\\winmm.timeBeginPeriod")
#pragma comment(linker, "/export:timeEndPeriod=C:\\Windows\\System32\\winmm.timeEndPeriod")
#pragma comment(linker, "/export:timeGetDevCaps=C:\\Windows\\System32\\winmm.timeGetDevCaps")
#pragma comment(linker, "/export:timeGetSystemTime=C:\\Windows\\System32\\winmm.timeGetSystemTime")
#pragma comment(linker, "/export:timeGetTime=C:\\Windows\\System32\\winmm.timeGetTime")
#pragma comment(linker, "/export:timeKillEvent=C:\\Windows\\System32\\winmm.timeKillEvent")
#pragma comment(linker, "/export:timeSetEvent=C:\\Windows\\System32\\winmm.timeSetEvent")
#pragma comment(linker, "/export:waveInAddBuffer=C:\\Windows\\System32\\winmm.waveInAddBuffer")
#pragma comment(linker, "/export:waveInClose=C:\\Windows\\System32\\winmm.waveInClose")
#pragma comment(linker, "/export:waveInGetDevCapsA=C:\\Windows\\System32\\winmm.waveInGetDevCapsA")
#pragma comment(linker, "/export:waveInGetDevCapsW=C:\\Windows\\System32\\winmm.waveInGetDevCapsW")
#pragma comment(linker, "/export:waveInGetErrorTextA=C:\\Windows\\System32\\winmm.waveInGetErrorTextA")
#pragma comment(linker, "/export:waveInGetErrorTextW=C:\\Windows\\System32\\winmm.waveInGetErrorTextW")
#pragma comment(linker, "/export:waveInGetID=C:\\Windows\\System32\\winmm.waveInGetID")
#pragma comment(linker, "/export:waveInGetNumDevs=C:\\Windows\\System32\\winmm.waveInGetNumDevs")
#pragma comment(linker, "/export:waveInGetPosition=C:\\Windows\\System32\\winmm.waveInGetPosition")
#pragma comment(linker, "/export:waveInMessage=C:\\Windows\\System32\\winmm.waveInMessage")
#pragma comment(linker, "/export:waveInOpen=C:\\Windows\\System32\\winmm.waveInOpen")
#pragma comment(linker, "/export:waveInPrepareHeader=C:\\Windows\\System32\\winmm.waveInPrepareHeader")
#pragma comment(linker, "/export:waveInReset=C:\\Windows\\System32\\winmm.waveInReset")
#pragma comment(linker, "/export:waveInStart=C:\\Windows\\System32\\winmm.waveInStart")
#pragma comment(linker, "/export:waveInStop=C:\\Windows\\System32\\winmm.waveInStop")
#pragma comment(linker, "/export:waveInUnprepareHeader=C:\\Windows\\System32\\winmm.waveInUnprepareHeader")
#pragma comment(linker, "/export:waveOutBreakLoop=C:\\Windows\\System32\\winmm.waveOutBreakLoop")
#pragma comment(linker, "/export:waveOutClose=C:\\Windows\\System32\\winmm.waveOutClose")
#pragma comment(linker, "/export:waveOutGetDevCapsA=C:\\Windows\\System32\\winmm.waveOutGetDevCapsA")
#pragma comment(linker, "/export:waveOutGetDevCapsW=C:\\Windows\\System32\\winmm.waveOutGetDevCapsW")
#pragma comment(linker, "/export:waveOutGetErrorTextA=C:\\Windows\\System32\\winmm.waveOutGetErrorTextA")
#pragma comment(linker, "/export:waveOutGetErrorTextW=C:\\Windows\\System32\\winmm.waveOutGetErrorTextW")
#pragma comment(linker, "/export:waveOutGetID=C:\\Windows\\System32\\winmm.waveOutGetID")
#pragma comment(linker, "/export:waveOutGetNumDevs=C:\\Windows\\System32\\winmm.waveOutGetNumDevs")
#pragma comment(linker, "/export:waveOutGetPitch=C:\\Windows\\System32\\winmm.waveOutGetPitch")
#pragma comment(linker, "/export:waveOutGetPlaybackRate=C:\\Windows\\System32\\winmm.waveOutGetPlaybackRate")
#pragma comment(linker, "/export:waveOutGetPosition=C:\\Windows\\System32\\winmm.waveOutGetPosition")
#pragma comment(linker, "/export:waveOutGetVolume=C:\\Windows\\System32\\winmm.waveOutGetVolume")
#pragma comment(linker, "/export:waveOutMessage=C:\\Windows\\System32\\winmm.waveOutMessage")
#pragma comment(linker, "/export:waveOutOpen=C:\\Windows\\System32\\winmm.waveOutOpen")
#pragma comment(linker, "/export:waveOutPause=C:\\Windows\\System32\\winmm.waveOutPause")
#pragma comment(linker, "/export:waveOutPrepareHeader=C:\\Windows\\System32\\winmm.waveOutPrepareHeader")
#pragma comment(linker, "/export:waveOutReset=C:\\Windows\\System32\\winmm.waveOutReset")
#pragma comment(linker, "/export:waveOutRestart=C:\\Windows\\System32\\winmm.waveOutRestart")
#pragma comment(linker, "/export:waveOutSetPitch=C:\\Windows\\System32\\winmm.waveOutSetPitch")
#pragma comment(linker, "/export:waveOutSetPlaybackRate=C:\\Windows\\System32\\winmm.waveOutSetPlaybackRate")
#pragma comment(linker, "/export:waveOutSetVolume=C:\\Windows\\System32\\winmm.waveOutSetVolume")
#pragma comment(linker, "/export:waveOutUnprepareHeader=C:\\Windows\\System32\\winmm.waveOutUnprepareHeader")
#pragma comment(linker, "/export:waveOutWrite=C:\\Windows\\System32\\winmm.waveOutWrite")

// ==========================================
// 2. تعريفات MinHook والثوابت
// ==========================================
typedef LONG NTSTATUS;
typedef struct _IO_STATUS_BLOCK { union { LONG Status; PVOID Pointer; }; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef HANDLE (WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *DeviceIoControl_t)(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtDeviceIoControlFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, ULONG, PVOID, ULONG, PVOID, ULONG);

CreateFileW_t pOriginalCreateFileW = NULL;
WriteFile_t pOriginalWriteFile = NULL;
DeviceIoControl_t pOriginalDeviceIoControl = NULL;
NtWriteFile_t pOriginalNtWriteFile = NULL;
NtDeviceIoControlFile_t pOriginalNtDeviceIoControlFile = NULL;

#define MAX_HANDLES 200

// هيكل لحفظ مقبض المنفذ واسمه
struct HandleInfo {
    HANDLE h;
    wchar_t portName[256];
};
HandleInfo monitoredHandles[MAX_HANDLES] = {0};

#define LOG_DIR "C:\\all_port_usb_mobile_monitor"
#define LOG_FILE "C:\\all_port_usb_mobile_monitor\\usb_com_log.txt"

// ==========================================
// 3. دوال إدارة المقابض (Handles)
// ==========================================
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
    for(int i=0; i<MAX_HANDLES; i++) {
        if(monitoredHandles[i].h == h) {
            wcscpy_s(outName, 256, monitoredHandles[i].portName);
            return true;
        }
    }
    return false;
}

bool IsMonitored(HANDLE h) {
    for(int i=0; i<MAX_HANDLES; i++) { if(monitoredHandles[i].h == h) return true; }
    return false;
}

// ==========================================
// 4. دالة كتابة اللوق المنظمة (محدثة)
// ==========================================
void LogData(const char* type, const wchar_t* portName, const BYTE* buffer, DWORD bufferSize) {
    if (bufferSize == 0 || buffer == NULL) return;
    
    // تصفية البيانات العشوائية
    int printableCount = 0;
    for (DWORD i = 0; i < bufferSize; i++) { if (isprint(buffer[i]) || isspace(buffer[i])) printableCount++; }
    if (bufferSize > 10 && (printableCount * 100 / bufferSize) < 20) return;
    
    CreateDirectoryA(LOG_DIR, NULL);
    FILE* logFile;
    fopen_s(&logFile, LOG_FILE, "a+");
    if (logFile) {
        // كتابة العلامة والوقت عند كل عملية إرسال
        time_t now = time(0);
        tm tstruct;
        char buf[80];
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        
        fprintf(logFile, "\n********************************\n");
        fprintf(logFile, "Time: %s\n", buf);
        
        // تحويل اسم المنفذ من wchar_t إلى char عادي للطباعة
        char portNameA[256];
        wcstombs_s(NULL, portNameA, portName, 256);
        fprintf(logFile, "Port: %s\n", portNameA);
        
        fprintf(logFile, "Type: %s\n", type);
        fprintf(logFile, "Data: ");
        
        for (DWORD i = 0; i < bufferSize; i++) {
            if (isprint(buffer[i]) || buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') {
                fprintf(logFile, "%c", buffer[i]);
            } else {
                fprintf(logFile, "\\x%02X", buffer[i]);
            }
        }
        fprintf(logFile, "\n");
        fclose(logFile);
    }
}

// ==========================================
// 5. دوال الـ Hooking (التنصت)
// ==========================================
HANDLE WINAPI HookedCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    HANDLE hFile = pOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    if (lpFileName) {
        if (wcsstr(lpFileName, L"COM") || wcsstr(lpFileName, L"\\\\?\\")) {
            AddHandle(hFile, lpFileName);
        }
    }
    return hFile;
}

BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hFile)) {
        wchar_t portName[256];
        if (GetPortName(hFile, portName)) {
            LogData("WRITE (COM/USB)", portName, (BYTE*)lpBuffer, nNumberOfBytesToWrite);
        }
    }
    return pOriginalWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI HookedDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
    if (IsMonitored(hDevice) && lpInBuffer != NULL && nInBufferSize > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) {
            LogData("USB_IOCTL_SEND", portName, (BYTE*)lpInBuffer, nInBufferSize);
        }
    }
    BOOL result = pOriginalDeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
    // تسجيل رد الهاتف أيضاً
    if (IsMonitored(hDevice) && lpOutBuffer != NULL && lpBytesReturned != NULL && *lpBytesReturned > 0) {
        wchar_t portName[256];
        if (GetPortName(hDevice, portName)) {
            LogData("USB_IOCTL_RESPONSE", portName, (BYTE*)lpOutBuffer, *lpBytesReturned);
        }
    }
    return result;
}

NTSTATUS NTAPI HookedNtWriteFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) {
    if (IsMonitored(FileHandle) && Buffer != NULL && Length > 0) {
        wchar_t portName[256];
        if (GetPortName(FileHandle, portName)) {
            LogData("NT_WRITE (COM/USB)", portName, (BYTE*)Buffer, Length);
        }
    }
    return pOriginalNtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

NTSTATUS NTAPI HookedNtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength) {
    return pOriginalNtDeviceIoControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

// ==========================================
// 6. نقطة الدخول للـ DLL
// ==========================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        if (MH_Initialize() == MH_OK) {
            MH_CreateHookApi(L"kernel32.dll", "CreateFileW", &HookedCreateFileW, (LPVOID*)&pOriginalCreateFileW);
            MH_CreateHookApi(L"kernel32.dll", "WriteFile", &HookedWriteFile, (LPVOID*)&pOriginalWriteFile);
            MH_CreateHookApi(L"kernel32.dll", "DeviceIoControl", &HookedDeviceIoControl, (LPVOID*)&pOriginalDeviceIoControl);
            MH_CreateHookApi(L"ntdll.dll", "NtWriteFile", &HookedNtWriteFile, (LPVOID*)&pOriginalNtWriteFile);
            MH_CreateHookApi(L"ntdll.dll", "NtDeviceIoControlFile", &HookedNtDeviceIoControlFile, (LPVOID*)&pOriginalNtDeviceIoControlFile);
            MH_EnableHook(MH_ALL_HOOKS);
        }
    }
    return TRUE;
}
