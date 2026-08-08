#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cwctype>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include "MinHook.h"

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

// Records local file activity from the target process.
// It does not decrypt network traffic or attempt to bypass protected content.
typedef LONG NTSTATUS;
typedef NTSTATUS (NTAPI *NtCreateFile_t)(PHANDLE, ACCESS_MASK, PVOID, PVOID, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI *NtReadFile_t)(HANDLE, HANDLE, PVOID, PVOID, PVOID, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PVOID, PVOID, PVOID, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtClose_t)(HANDLE);

static NtCreateFile_t g_NtCreateFile = nullptr;
static NtReadFile_t g_NtReadFile = nullptr;
static NtWriteFile_t g_NtWriteFile = nullptr;
static NtClose_t g_NtClose = nullptr;

struct FileState { std::wstring path; uint64_t readBytes=0; uint64_t writeBytes=0; };
static std::unordered_map<HANDLE, FileState> g_files;
static std::mutex g_mutex;
static thread_local bool g_busy=false;
static char g_log[MAX_PATH] = {};

static std::wstring ModuleDir() {
    wchar_t p[MAX_PATH] = {};
    HMODULE m = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&ModuleDir), &m)) return L".";
    DWORD n=GetModuleFileNameW(m,p,MAX_PATH); if(!n || n>=MAX_PATH) return L".";
    wchar_t* s=wcsrchr(p,L'\\'); if(s) *s=L'\0'; return p;
}
static void InitLog(){ std::wstring p=ModuleDir()+L"\\operation_files.jsonl"; WideCharToMultiByte(CP_UTF8,0,p.c_str(),-1,g_log,MAX_PATH,nullptr,nullptr); }
static std::string Esc(const std::wstring& s){ std::string o; for(wchar_t c:s){ if(c==L'\\')o+="\\\\"; else if(c==L'\"')o+="\\\""; else if(c==L'\r')o+="\\r"; else if(c==L'\n')o+="\\n"; else if(c<128)o+=(char)c; } return o; }
static void Log(const char* type,HANDLE h,const std::wstring& path,uint64_t bytes){
    if(!g_log[0]) InitLog(); FILE* f=nullptr; if(fopen_s(&f,g_log,"ab")||!f)return;
    SYSTEMTIME t{}; GetLocalTime(&t); std::string p=Esc(path);
    fprintf(f,"{\"time\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03u\",\"type\":\"%s\",\"handle\":\"%p\",\"path\":\"%s\",\"bytes\":%llu}\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,type,h,p.c_str(),(unsigned long long)bytes); fclose(f);
}
static bool Candidate(const std::wstring& p){ std::wstring x=p; for(auto& c:x)c=towlower(c); const wchar_t* a[]={L"\\temp\\",L"\\tmp\\",L"appdata\\local\\temp",L"loader",L"preloader",L"firehose",L"programmer",L"download_agent",L".bin",L".mbn",L".elf",L".hex",L".img",L".xml",L".pac",L".scatter",L".da",L".lz4",L".zip",L".7z"}; for(auto n:a)if(x.find(n)!=std::wstring::npos)return true; return false; }
static NTSTATUS NTAPI HookCreate(PHANDLE fh,ACCESS_MASK a,PVOID oa,PVOID ios,PLARGE_INTEGER al,ULONG fa,ULONG sa,ULONG cd,ULONG co,PVOID eb,ULONG el){
    NTSTATUS st=g_NtCreateFile(fh,a,oa,ios,al,fa,sa,cd,co,eb,el);
    if(!g_busy && st>=0 && fh && *fh){ std::wstring p=L""; if(Candidate(p)){ std::lock_guard<std::mutex> l(g_mutex); g_files[*fh]={p,0,0}; Log("FILE_OPEN",*fh,p,0); } }
    return st;
}
static NTSTATUS NTAPI HookRead(HANDLE h,HANDLE e,PVOID ar,PVOID ac,PVOID ios,PVOID b,ULONG n,PLARGE_INTEGER off,PULONG k){ NTSTATUS st=g_NtReadFile(h,e,ar,ac,ios,b,n,off,k); if(!g_busy){std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){i->second.readBytes+=n;Log("FILE_READ",h,i->second.path,n);}} return st; }
static NTSTATUS NTAPI HookWrite(HANDLE h,HANDLE e,PVOID ar,PVOID ac,PVOID ios,PVOID b,ULONG n,PLARGE_INTEGER off,PULONG k){ NTSTATUS st=g_NtWriteFile(h,e,ar,ac,ios,b,n,off,k); if(!g_busy){std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){i->second.writeBytes+=n;Log("FILE_WRITE",h,i->second.path,n);}} return st; }
static NTSTATUS NTAPI HookClose(HANDLE h){ FileState s;bool found=false;{std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){s=i->second;g_files.erase(i);found=true;}} NTSTATUS st=g_NtClose(h);if(found)Log("FILE_CLOSE",h,s.path,s.readBytes+s.writeBytes);return st; }
static DWORD WINAPI Install(LPVOID){InitLog();HMODULE n=GetModuleHandleW(L"ntdll.dll");if(!n)return 0;LPVOID c=GetProcAddress(n,"NtCreateFile"),r=GetProcAddress(n,"NtReadFile"),w=GetProcAddress(n,"NtWriteFile"),x=GetProcAddress(n,"NtClose");if(!c||!r||!w||!x)return 0;g_NtCreateFile=(NtCreateFile_t)c;g_NtReadFile=(NtReadFile_t)r;g_NtWriteFile=(NtWriteFile_t)w;g_NtClose=(NtClose_t)x;MH_CreateHook(c,&HookCreate,(LPVOID*)&g_NtCreateFile);MH_CreateHook(r,&HookRead,(LPVOID*)&g_NtReadFile);MH_CreateHook(w,&HookWrite,(LPVOID*)&g_NtWriteFile);MH_CreateHook(x,&HookClose,(LPVOID*)&g_NtClose);MH_EnableHook(c);MH_EnableHook(r);MH_EnableHook(w);MH_EnableHook(x);return 0;}
struct Start{Start(){CreateThread(nullptr,0,Install,nullptr,0,nullptr);}};static Start g_start;
