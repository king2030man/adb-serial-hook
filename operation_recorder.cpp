#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include "MinHook.h"

#pragma comment(lib, "bcrypt.lib")

// File observer only: records files that the target process opens/reads/writes.
// It does not decrypt network traffic and does not copy protected files.
// High entropy is reported as an indicator only; it is NOT proof of encryption.

typedef NTSTATUS (NTAPI *NtCreateFile_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI *NtReadFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtClose_t)(HANDLE);

typedef NTSTATUS (NTAPI *NtQueryInformationFile_t)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

static NtCreateFile_t g_NtCreateFile = nullptr;
static NtReadFile_t g_NtReadFile = nullptr;
static NtWriteFile_t g_NtWriteFile = nullptr;
static NtClose_t g_NtClose = nullptr;
static NtQueryInformationFile_t g_NtQueryInformationFile = nullptr;

struct FileState { std::wstring path; uint64_t readBytes = 0; uint64_t writeBytes = 0; bool interesting = false; };
static std::unordered_map<HANDLE, FileState> g_files;
static std::mutex g_filesMutex;
static thread_local bool g_recursing = false;
static volatile LONG g_installed = 0;
static char g_fileLog[MAX_PATH] = {};

static std::wstring GetModuleDirectory() {
    wchar_t path[MAX_PATH] = {};
    HMODULE mod = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&GetModuleDirectory), &mod)) return L".";
    DWORD n = GetModuleFileNameW(mod, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return L".";
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) *slash = L'\0';
    return path;
}

static void InitFileLogPath() {
    std::wstring dir = GetModuleDirectory();
    std::wstring file = dir + L"\\operation_files.jsonl";
    WideCharToMultiByte(CP_UTF8, 0, file.c_str(), -1, g_fileLog, MAX_PATH, nullptr, nullptr);
}

static std::string JsonEscape(const std::wstring& s) {
    std::string out;
    for (wchar_t c : s) {
        switch (c) {
        case L'\\': out += "\\\\"; break;
        case L'\"': out += "\\\""; break;
        case L'\r': out += "\\r"; break;
        case L'\n': out += "\\n"; break;
        case L'\t': out += "\\t"; break;
        default:
            if (c < 0x80) out.push_back(static_cast<char>(c));
            else { char utf8[5] = {}; int n = WideCharToMultiByte(CP_UTF8, 0, &c, 1, utf8, 4, nullptr, nullptr); out.append(utf8, n); }
        }
    }
    return out;
}

static void LogEvent(const char* type, HANDLE h, const std::wstring& path, uint64_t amount, const char* extra = nullptr) {
    if (!g_fileLog[0]) InitFileLogPath();
    FILE* f = nullptr;
    if (fopen_s(&f, g_fileLog, "ab") != 0 || !f) return;
    SYSTEMTIME st{}; GetLocalTime(&st);
    std::string p = JsonEscape(path);
    fprintf(f, "{\"time\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03u\",\"type\":\"%s\",\"handle\":\"%p\",\"path\":\"%s\",\"bytes\":%llu", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, type, h, p.c_str(), static_cast<unsigned long long>(amount));
    if (extra) fprintf(f, ",%s", extra);
    fprintf(f, "}\n");
    fclose(f);
}

static bool IsCandidatePath(const std::wstring& p) {
    std::wstring x = p;
    for (auto& c : x) c = towlower(c);
    const wchar_t* needles[] = { L"\\temp\\", L"\\tmp\\", L"\\appdata\\local\\temp\\", L"tsm", L"loader", L"preloader", L"firehose", L"programmer", L"download_agent", L".bin", L".mbn", L".elf", L".hex", L".img", L".xml", L".pac", L".scatter", L".da", L".lz4", L".zip", L".7z" };
    for (auto n : needles) if (x.find(n) != std::wstring::npos) return true;
    return false;
}

static const char* DetectMagic(const BYTE* b, DWORD n) {
    if (n >= 4 && b[0] == 0x7F && b[1] == 'E' && b[2] == 'L' && b[3] == 'F') return "ELF";
    if (n >= 2 && b[0] == 'M' && b[1] == 'Z') return "PE/MZ";
    if (n >= 4 && b[0] == 0x50 && b[1] == 0x4B && (b[2] == 3 || b[2] == 5 || b[2] == 7) && (b[3] == 4 || b[3] == 6 || b[3] == 8)) return "ZIP";
    if (n >= 4 && b[0] == 0x1F && b[1] == 0x8B && b[2] == 0x08) return "GZIP";
    if (n >= 4 && b[0] == 0x28 && b[1] == 0xB5 && b[2] == 0x2F && b[3] == 0xFD) return "ZSTD";
    if (n >= 4 && b[0] == 0x04 && b[1] == 0x22 && b[2] == 0x4D && b[3] == 0x18) return "LZ4";
    return "unknown";
}

static double Entropy(const BYTE* b, DWORD n) {
    if (!b || n == 0) return 0.0;
    uint64_t counts[256] = {};
    for (DWORD i = 0; i < n; ++i) ++counts[b[i]];
    double h = 0.0;
    for (int i = 0; i < 256; ++i) { if (!counts[i]) continue; double p = static_cast<double>(counts[i]) / n; h -= p * log2(p); }
    return h;
}

static void AnalyzeCompletedFile(const std::wstring& path, HANDLE originalHandle) {
    if (!IsCandidatePath(path)) return;
    g_recursing = true;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { LogEvent("FILE_UNAVAILABLE", originalHandle, path, 0, "\"reason\":\"open_failed_or_deleted\""); g_recursing = false; return; }
    LARGE_INTEGER sz{}; GetFileSizeEx(h, &sz);
    BYTE sample[65536] = {}; DWORD got = 0; ReadFile(h, sample, sizeof(sample), &got, nullptr);
    const char* magic = DetectMagic(sample, got); double ent = Entropy(sample, got); char extra[256];
    snprintf(extra, sizeof(extra), "\"size\":%lld,\"magic\":\"%s\",\"sample_bytes\":%lu,\"entropy_bits\":%.3f,\"high_entropy_indicator\":%s", static_cast<long long>(sz.QuadPart), magic, static_cast<unsigned long>(got), ent, (ent >= 7.5 && strcmp(magic, "unknown") == 0) ? "true" : "false");
    LogEvent("FILE_ANALYSIS", originalHandle, path, static_cast<uint64_t>(sz.QuadPart), extra);
    CloseHandle(h); g_recursing = false;
}

static std::wstring ObjectPath(POBJECT_ATTRIBUTES oa) {
    if (!oa || !oa->ObjectName || !oa->ObjectName->Buffer || !oa->ObjectName->Length) return L"";
    return std::wstring(oa->ObjectName->Buffer, oa->ObjectName->Length / sizeof(wchar_t));
}

static NTSTATUS NTAPI HookNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength) {
    if (!g_NtCreateFile) return (NTSTATUS)0xC0000001L;
    NTSTATUS st = g_NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    if (!g_recursing && NT_SUCCESS(st) && FileHandle && *FileHandle) {
        std::wstring path = ObjectPath(ObjectAttributes);
        if (IsCandidatePath(path)) { std::lock_guard<std::mutex> lock(g_filesMutex); g_files[*FileHandle] = {path, 0, 0, true}; LogEvent("FILE_OPEN", *FileHandle, path, 0); }
    }
    return st;
}

static NTSTATUS NTAPI HookNtReadFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) {
    NTSTATUS st = g_NtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    if (!g_recursing) { std::lock_guard<std::mutex> lock(g_filesMutex); auto it = g_files.find(FileHandle); if (it != g_files.end()) { uint64_t n = NT_SUCCESS(st) && IoStatusBlock ? static_cast<uint64_t>(IoStatusBlock->Information) : 0; it->second.readBytes += n; if (n) LogEvent("FILE_READ", FileHandle, it->second.path, n); } }
    return st;
}

static NTSTATUS NTAPI HookNtWriteFile(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) {
    NTSTATUS st = g_NtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    if (!g_recursing) { std::lock_guard<std::mutex> lock(g_filesMutex); auto it = g_files.find(FileHandle); if (it != g_files.end()) { uint64_t n = NT_SUCCESS(st) && IoStatusBlock ? static_cast<uint64_t>(IoStatusBlock->Information) : 0; it->second.writeBytes += n; if (n) LogEvent("FILE_WRITE", FileHandle, it->second.path, n); } }
    return st;
}

static NTSTATUS NTAPI HookNtClose(HANDLE FileHandle) {
    FileState state; bool found = false;
    if (!g_recursing) { std::lock_guard<std::mutex> lock(g_filesMutex); auto it = g_files.find(FileHandle); if (it != g_files.end()) { state = it->second; g_files.erase(it); found = true; } }
    NTSTATUS st = g_NtClose(FileHandle);
    if (found) { char extra[192]; snprintf(extra, sizeof(extra), "\"read_bytes\":%llu,\"write_bytes\":%llu", static_cast<unsigned long long>(state.readBytes), static_cast<unsigned long long>(state.writeBytes)); LogEvent("FILE_CLOSE", FileHandle, state.path, 0, extra); if (state.writeBytes || state.readBytes) AnalyzeCompletedFile(state.path, FileHandle); }
    return st;
}

static DWORD WINAPI InstallThread(LPVOID) {
    InitFileLogPath();
    for (int i = 0; i < 300; ++i) {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll) {
            auto create = reinterpret_cast<LPVOID>(GetProcAddress(ntdll, "NtCreateFile"));
            auto read = reinterpret_cast<LPVOID>(GetProcAddress(ntdll, "NtReadFile"));
            auto write = reinterpret_cast<LPVOID>(GetProcAddress(ntdll, "NtWriteFile"));
            auto close = reinterpret_cast<LPVOID>(GetProcAddress(ntdll, "NtClose"));
            if (create && read && write && close) {
                if (!g_NtCreateFile) {
                    g_NtCreateFile = reinterpret_cast<NtCreateFile_t>(create); g_NtReadFile = reinterpret_cast<NtReadFile_t>(read); g_NtWriteFile = reinterpret_cast<NtWriteFile_t>(write); g_NtClose = reinterpret_cast<NtClose_t>(close);
                    MH_STATUS a = MH_CreateHook(create, &HookNtCreateFile, reinterpret_cast<LPVOID*>(&g_NtCreateFile));
                    MH_STATUS b = MH_CreateHook(read, &HookNtReadFile, reinterpret_cast<LPVOID*>(&g_NtReadFile));
                    MH_STATUS c = MH_CreateHook(write, &HookNtWriteFile, reinterpret_cast<LPVOID*>(&g_NtWriteFile));
                    MH_STATUS d = MH_CreateHook(close, &HookNtClose, reinterpret_cast<LPVOID*>(&g_NtClose));
                    if (a == MH_OK || a == MH_ERROR_ALREADY_CREATED) {
                        if (MH_EnableHook(create) == MH_OK || MH_EnableHook(create) == MH_ERROR_ENABLED) {}
                        if (MH_EnableHook(read) == MH_OK || MH_EnableHook(read) == MH_ERROR_ENABLED) {}
                        if (MH_EnableHook(write) == MH_OK || MH_EnableHook(write) == MH_ERROR_ENABLED) {}
                        if (MH_EnableHook(close) == MH_OK || MH_EnableHook(close) == MH_ERROR_ENABLED) {}
                        InterlockedExchange(&g_installed, 1); return 0;
                    }
                    (void)b; (void)c; (void)d; g_NtCreateFile = nullptr; g_NtReadFile = nullptr; g_NtWriteFile = nullptr; g_NtClose = nullptr;
                }
            }
        }
        Sleep(100);
    }
    return 0;
}

struct AutoStart { AutoStart() { DisableThreadLibraryCalls(GetModuleHandleW(nullptr)); CreateThread(nullptr, 0, InstallThread, nullptr, 0, nullptr); } };
static AutoStart g_autoStart;
