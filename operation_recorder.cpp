#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <cstdio>
#include <cstdint>
#include <cwctype>
#include <string>
#include <unordered_map>
#include <mutex>
#include "MinHook.h"

typedef NTSTATUS (NTAPI *NtCreateFile_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI *NtReadFile_t)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI *NtClose_t)(HANDLE);

static NtCreateFile_t g_NtCreateFile=nullptr;
static NtReadFile_t g_NtReadFile=nullptr;
static NtWriteFile_t g_NtWriteFile=nullptr;
static NtClose_t g_NtClose=nullptr;
struct FileState{std::wstring path;uint64_t readBytes=0;uint64_t writeBytes=0;};
static std::unordered_map<HANDLE,FileState> g_files;
static std::mutex g_mutex;
static char g_logPath[MAX_PATH]={};
static std::wstring ModuleDirectory(){wchar_t p[MAX_PATH]={};HMODULE m=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&ModuleDirectory),&m))return L".";DWORD n=GetModuleFileNameW(m,p,MAX_PATH);if(!n||n>=MAX_PATH)return L".";wchar_t*s=wcsrchr(p,L'\\');if(s)*s=L'\0';return p;}
static void InitLog(){std::wstring p=ModuleDirectory()+L"\\operation_files.jsonl";WideCharToMultiByte(CP_UTF8,0,p.c_str(),-1,g_logPath,MAX_PATH,nullptr,nullptr);}
static std::string Esc(const std::wstring&s){std::string o;for(wchar_t c:s){if(c==L'\\')o+="\\\\";else if(c==L'\"')o+="\\\"";else if(c==L'\r')o+="\\r";else if(c==L'\n')o+="\\n";else if(c==L'\t')o+="\\t";else if(c>=0&&c<128)o+=(char)c;}return o;}
static void Log(const char*type,HANDLE h,const std::wstring&p,uint64_t bytes){if(!g_logPath[0])InitLog();FILE*f=nullptr;if(fopen_s(&f,g_logPath,"ab")||!f)return;SYSTEMTIME t{};GetLocalTime(&t);std::string e=Esc(p);fprintf(f,"{\"time\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03u\",\"type\":\"%s\",\"handle\":\"%p\",\"path\":\"%s\",\"bytes\":%llu}\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,type,h,e.c_str(),(unsigned long long)bytes);fclose(f);}
static std::wstring ObjectPath(POBJECT_ATTRIBUTES oa){if(!oa||!oa->ObjectName||!oa->ObjectName->Buffer||!oa->ObjectName->Length)return L"";return std::wstring(oa->ObjectName->Buffer,oa->ObjectName->Length/sizeof(wchar_t));}
static bool Candidate(const std::wstring&p){if(p.empty())return false;std::wstring x=p;for(wchar_t&c:x)c=towlower(c);const wchar_t*n[]={L"\\temp\\",L"\\tmp\\",L"appdata\\local\\temp",L"loader",L"preloader",L"firehose",L"programmer",L"download_agent",L".bin",L".mbn",L".elf",L".hex",L".img",L".xml",L".pac",L".scatter",L".da",L".lz4",L".zip",L".7z"};for(auto s:n)if(x.find(s)!=std::wstring::npos)return true;return false;}
static NTSTATUS NTAPI HookCreate(PHANDLE fh,ACCESS_MASK a,POBJECT_ATTRIBUTES oa,PIO_STATUS_BLOCK ios,PLARGE_INTEGER al,ULONG fa,ULONG sa,ULONG cd,ULONG co,PVOID eb,ULONG el){if(!g_NtCreateFile)return STATUS_UNSUCCESSFUL;NTSTATUS st=g_NtCreateFile(fh,a,oa,ios,al,fa,sa,cd,co,eb,el);if(NT_SUCCESS(st)&&fh&&*fh){std::wstring p=ObjectPath(oa);if(Candidate(p)){std::lock_guard<std::mutex>l(g_mutex);g_files[*fh]={p,0,0};Log("FILE_OPEN",*fh,p,0);}}return st;}
static NTSTATUS NTAPI HookRead(HANDLE h,HANDLE e,PIO_APC_ROUTINE ar,PVOID ac,PIO_STATUS_BLOCK ios,PVOID b,ULONG n,PLARGE_INTEGER off,PULONG k){if(!g_NtReadFile)return STATUS_UNSUCCESSFUL;NTSTATUS st=g_NtReadFile(h,e,ar,ac,ios,b,n,off,k);std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){uint64_t got=NT_SUCCESS(st)&&ios?(uint64_t)ios->Information:0;i->second.readBytes+=got;if(got)Log("FILE_READ",h,i->second.path,got);}return st;}
static NTSTATUS NTAPI HookWrite(HANDLE h,HANDLE e,PIO_APC_ROUTINE ar,PVOID ac,PIO_STATUS_BLOCK ios,PVOID b,ULONG n,PLARGE_INTEGER off,PULONG k){if(!g_NtWriteFile)return STATUS_UNSUCCESSFUL;NTSTATUS st=g_NtWriteFile(h,e,ar,ac,ios,b,n,off,k);std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){uint64_t put=NT_SUCCESS(st)&&ios?(uint64_t)ios->Information:0;i->second.writeBytes+=put;if(put)Log("FILE_WRITE",h,i->second.path,put);}return st;}
static NTSTATUS NTAPI HookClose(HANDLE h){FileState s{};bool found=false;{std::lock_guard<std::mutex>l(g_mutex);auto i=g_files.find(h);if(i!=g_files.end()){s=i->second;g_files.erase(i);found=true;}}if(!g_NtClose)return STATUS_UNSUCCESSFUL;NTSTATUS st=g_NtClose(h);if(found)Log("FILE_CLOSE",h,s.path,s.readBytes+s.writeBytes);return st;}
static DWORD WINAPI Install(LPVOID){InitLog();for(int i=0;i<300;i++){HMODULE n=GetModuleHandleW(L"ntdll.dll");if(n&&MH_Initialize()!=MH_ERROR_NOT_INITIALIZED){LPVOID c=GetProcAddress(n,"NtCreateFile"),r=GetProcAddress(n,"NtReadFile"),w=GetProcAddress(n,"NtWriteFile"),x=GetProcAddress(n,"NtClose");if(c&&r&&w&&x){MH_STATUS a=MH_CreateHook(c,&HookCreate,(LPVOID*)&g_NtCreateFile);MH_STATUS b=MH_CreateHook(r,&HookRead,(LPVOID*)&g_NtReadFile);MH_STATUS d=MH_CreateHook(w,&HookWrite,(LPVOID*)&g_NtWriteFile);MH_STATUS e=MH_CreateHook(x,&HookClose,(LPVOID*)&g_NtClose);if((a==MH_OK||a==MH_ERROR_ALREADY_CREATED)&&(b==MH_OK||b==MH_ERROR_ALREADY_CREATED)&&(d==MH_OK||d==MH_ERROR_ALREADY_CREATED)&&(e==MH_OK||e==MH_ERROR_ALREADY_CREATED)){MH_EnableHook(c);MH_EnableHook(r);MH_EnableHook(w);MH_EnableHook(x);return 0;}}}Sleep(100);}return 0;}
struct Starter{Starter(){CreateThread(nullptr,0,Install,nullptr,0,nullptr);}};static Starter g_starter;
