#include <iostream>
#include <filesystem>
#include "../logging/console_logging.hpp"
#include "lua.hpp"
#include "../util/util.hpp"
#include "tasks.hpp"

namespace devkit {

    extern "C" int LuaDownloadFile(lua_State* L) {
        std::string url = lua_tostring(L, 1);
        std::string localPath = lua_tostring(L, 2);

        info("-- Download file:\n    {}\n    -> {}", url, localPath);
        try {
            DownloadFile(url, localPath);
            lua_pushboolean(L, true);
        } catch (const std::exception& e) {
            info("-- Error: {}", e.what());
            lua_pushboolean(L, false);
        }
        return 1;
    }

    extern "C" int LuaExtractZipFile(lua_State* L) {
        std::string zipFilePath = lua_tostring(L, 1);
        std::string destDir = lua_tostring(L, 2);

        info("-- Extract file:\n    {}\n    -> {}", zipFilePath, destDir);
        try {
            ExtractZipFile(zipFilePath, destDir);
            lua_pushboolean(L, true);
        } catch (const std::exception& e) {
            info("-- Error: {}", e.what());
            lua_pushboolean(L, false);
        }
        return 1;
    }

    void ExecuteLuaFile(const std::string& file) {
        lua_State* L = luaL_newstate();

        // download_file(url, local_path): bool
        lua_register(L, "download_file", LuaDownloadFile);
        // extract_zip_file(zipFilePath, destDir): bool
        lua_register(L, "extract_zip_file", LuaExtractZipFile);

        luaL_openlibs(L);

        bool error = false;
        std::string errorMessage;
        if (luaL_dofile(L, file.c_str()) != LUA_OK) {
            errorMessage = lua_tostring(L, -1);
            lua_pop(L, 1);  // Удаляем сообщение об ошибке из стека
        }
        lua_close(L);
        if (error) {
            throw std::runtime_error(errorMessage);
        }
    }
}