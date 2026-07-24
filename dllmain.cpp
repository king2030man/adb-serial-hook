#include <windows.h>
#include <cstdio>
#include <cstring>

// توجيه كافة دوال الـ Lua 5.1 الأساسية للملف الأصلي (سنقوم بتسمية الأصلي إلى lua51_orig.dll)
#pragma comment(linker, "/export:lua_newstate=lua51_orig.lua_newstate")
#pragma comment(linker, "/export:lua_close=lua51_orig.lua_close")
#pragma comment(linker, "/export:lua_newthread=lua51_orig.lua_newthread")
#pragma comment(linker, "/export:lua_atpanic=lua51_orig.lua_atpanic")
#pragma comment(linker, "/export:lua_gettop=lua51_orig.lua_gettop")
#pragma comment(linker, "/export:lua_settop=lua51_orig.lua_settop")
#pragma comment(linker, "/export:lua_pushvalue=lua51_orig.lua_pushvalue")
#pragma comment(linker, "/export:lua_remove=lua51_orig.lua_remove")
#pragma comment(linker, "/export:lua_insert=lua51_orig.lua_insert")
#pragma comment(linker, "/export:lua_replace=lua51_orig.lua_replace")
#pragma comment(linker, "/export:lua_checkstack=lua51_orig.lua_checkstack")
#pragma comment(linker, "/export:lua_xmove=lua51_orig.lua_xmove")
#pragma comment(linker, "/export:lua_isnumber=lua51_orig.lua_isnumber")
#pragma comment(linker, "/export:lua_isstring=lua51_orig.lua_isstring")
#pragma comment(linker, "/export:lua_iscfunction=lua51_orig.lua_iscfunction")
#pragma comment(linker, "/export:lua_isuserdata=lua51_orig.lua_isuserdata")
#pragma comment(linker, "/export:lua_type=lua51_orig.lua_type")
#pragma comment(linker, "/export:lua_typename=lua51_orig.lua_typename")
#pragma comment(linker, "/export:lua_equal=lua51_orig.lua_equal")
#pragma comment(linker, "/export:lua_rawequal=lua51_orig.lua_rawequal")
#pragma comment(linker, "/export:lua_lessthan=lua51_orig.lua_lessthan")
#pragma comment(linker, "/export:lua_tonumber=lua51_orig.lua_tonumber")
#pragma comment(linker, "/export:lua_tointeger=lua51_orig.lua_tointeger")
#pragma comment(linker, "/export:lua_toboolean=lua51_orig.lua_toboolean")
#pragma comment(linker, "/export:lua_tolstring=lua51_orig.lua_tolstring")
#pragma comment(linker, "/export:lua_objlen=lua51_orig.lua_objlen")
#pragma comment(linker, "/export:lua_tocfunction=lua51_orig.lua_tocfunction")
#pragma comment(linker, "/export:lua_touserdata=lua51_orig.lua_touserdata")
#pragma comment(linker, "/export:lua_tothread=lua51_orig.lua_tothread")
#pragma comment(linker, "/export:lua_topopointer=lua51_orig.lua_topopointer")
#pragma comment(linker, "/export:lua_pushnil=lua51_orig.lua_pushnil")
#pragma comment(linker, "/export:lua_pushnumber=lua51_orig.lua_pushnumber")
#pragma comment(linker, "/export:lua_pushinteger=lua51_orig.lua_pushinteger")
#pragma comment(linker, "/export:lua_pushlstring=lua51_orig.lua_pushlstring")
#pragma comment(linker, "/export:lua_pushstring=lua51_orig.lua_pushstring")
#pragma comment(linker, "/export:lua_pushvfstring=lua51_orig.lua_pushvfstring")
#pragma comment(linker, "/export:lua_pushfstring=lua51_orig.lua_pushfstring")
#pragma comment(linker, "/export:lua_pushcclosure=lua51_orig.lua_pushcclosure")
#pragma comment(linker, "/export:lua_pushboolean=lua51_orig.lua_pushboolean")
#pragma comment(linker, "/export:lua_pushlightuserdata=lua51_orig.lua_pushlightuserdata")
#pragma comment(linker, "/export:lua_pushthread=lua51_orig.lua_pushthread")
#pragma comment(linker, "/export:lua_gettable=lua51_orig.lua_gettable")
#pragma comment(linker, "/export:lua_getfield=lua51_orig.lua_getfield")
#pragma comment(linker, "/export:lua_rawget=lua51_orig.lua_rawget")
#pragma comment(linker, "/export:lua_rawgeti=lua51_orig.lua_rawgeti")
#pragma comment(linker, "/export:lua_createtable=lua51_orig.lua_createtable")
#pragma comment(linker, "/export:lua_newuserdata=lua51_orig.lua_newuserdata")
#pragma comment(linker, "/export:lua_getmetatable=lua51_orig.lua_getmetatable")
#pragma comment(linker, "/export:lua_getfenv=lua51_orig.lua_getfenv")
#pragma comment(linker, "/export:lua_settable=lua51_orig.lua_settable")
#pragma comment(linker, "/export:lua_setfield=lua51_orig.lua_setfield")
#pragma comment(linker, "/export:lua_rawset=lua51_orig.lua_rawset")
#pragma comment(linker, "/export:lua_rawseti=lua51_orig.lua_rawseti")
#pragma comment(linker, "/export:lua_setmetatable=lua51_orig.lua_setmetatable")
#pragma comment(linker, "/export:lua_setfenv=lua51_orig.lua_setfenv")
#pragma comment(linker, "/export:lua_call=lua51_orig.lua_call")
#pragma comment(linker, "/export:lua_pcall=lua51_orig.lua_pcall")
#pragma comment(linker, "/export:lua_cpcall=lua51_orig.lua_cpcall")
#pragma comment(linker, "/export:lua_load=lua51_orig.lua_load")
#pragma comment(linker, "/export:lua_dump=lua51_orig.lua_dump")
#pragma comment(linker, "/export:lua_yield=lua51_orig.lua_yield")
#pragma comment(linker, "/export:lua_resume=lua51_orig.lua_resume")
#pragma comment(linker, "/export:lua_status=lua51_orig.lua_status")
#pragma comment(linker, "/export:lua_gc=lua51_orig.lua_gc")
#pragma comment(linker, "/export:lua_error=lua51_orig.lua_error")
#pragma comment(linker, "/export:lua_next=lua51_orig.lua_next")
#pragma comment(linker, "/export:lua_concat=lua51_orig.lua_concat")
#pragma comment(linker, "/export:lua_getallocf=lua51_orig.lua_getallocf")
#pragma comment(linker, "/export:lua_setallocf=lua51_orig.lua_setallocf")
#pragma comment(linker, "/export:lua_getstack=lua51_orig.lua_getstack")
#pragma comment(linker, "/export:lua_getinfo=lua51_orig.lua_getinfo")
#pragma comment(linker, "/export:lua_getlocal=lua51_orig.lua_getlocal")
#pragma comment(linker, "/export:lua_setlocal=lua51_orig.lua_setlocal")
#pragma comment(linker, "/export:lua_getupvalue=lua51_orig.lua_getupvalue")
#pragma comment(linker, "/export:lua_setupvalue=lua51_orig.lua_setupvalue")
#pragma comment(linker, "/export:lua_sethook=lua51_orig.lua_sethook")
#pragma comment(linker, "/export:lua_gethook=lua51_orig.lua_gethook")
#pragma comment(linker, "/export:lua_gethookmask=lua51_orig.lua_gethookmask")
#pragma comment(linker, "/export:lua_gethookcount=lua51_orig.lua_gethookcount")
#pragma comment(linker, "/export:luaopen_base=lua51_orig.luaopen_base")
#pragma comment(linker, "/export:luaopen_table=lua51_orig.luaopen_table")
#pragma comment(linker, "/export:luaopen_io=lua51_orig.luaopen_io")
#pragma comment(linker, "/export:luaopen_os=lua51_orig.luaopen_os")
#pragma comment(linker, "/export:luaopen_string=lua51_orig.luaopen_string")
#pragma comment(linker, "/export:luaopen_math=lua51_orig.luaopen_math")
#pragma comment(linker, "/export:luaopen_debug=lua51_orig.luaopen_debug")
#pragma comment(linker, "/export:luaopen_package=lua51_orig.luaopen_package")
#pragma comment(linker, "/export:luaL_openlibs=lua51_orig.luaL_openlibs")

// مؤشر الدالة الأصلية التي سنقوم باعتراضها وقراءة محتواها
typedef int (*luaL_loadbuffer_t)(void* L, const char* buff, size_t sz, const char* name);
luaL_loadbuffer_t pOriginal_luaL_loadbuffer = NULL;

// دالة حفظ اللوج الذكية
void LogLuaScript(const char* name, const char* buff, size_t sz) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(exePath, MAX_PATH, L"lua_scripts_log.txt");

    FILE* file;
    _wfopen_s(&file, exePath, L"a+");
    if (file) {
        fprintf(file, "\n--- [CAPTURED SCRIPT: %s | Size: %zu] ---\n", name ? name : "Unknown", sz);
        fwrite(buff, 1, sz, file);
        fprintf(file, "\n----------------------------------------\n");
        fclose(file);
    }
}

// دالة الاعتراض الخاصة بنا: هنا يتم اقتناص السكربت كاملاً قبل تشغيله
extern "C" __declspec(dllexport) int luaL_loadbuffer(void* L, const char* buff, size_t sz, const char* name) {
    if (pOriginal_luaL_loadbuffer == NULL) {
        HMODULE hOrig = GetModuleHandleW(L"lua51_orig.dll");
        if (!hOrig) hOrig = LoadLibraryW(L"lua51_orig.dll");
        if (hOrig) {
            pOriginal_luaL_loadbuffer = (luaL_loadbuffer_t)GetProcAddress(hOrig, "luaL_loadbuffer");
        }
    }

    // حفظ كود الـ Lua النصي فوراً في ملف اللوج
    if (buff && sz > 0) {
        LogLuaScript(name, buff, sz);
    }

    // تمرير الكود للدالة الأصلية ليشتغل البرنامج بدون أي تأخير أو خطأ
    if (pOriginal_luaL_loadbuffer) {
        return pOriginal_luaL_loadbuffer(L, buff, sz, name);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
