#pragma once

#include <string>
#include <algorithm>
#include <filesystem>

namespace devkit::PathUtils {
    
    inline std::string NormalizePath(const std::string& path, bool removeTrailingSlash, bool makeAbsolute) {
        std::string result = path;
        if (makeAbsolute) {
            std::filesystem::path p(path);
            if (!p.is_absolute()) {
                p = std::filesystem::current_path() / p;
                result = p.string();
            }
        }
        std::replace(result.begin(), result.end(), '\\', '/');
        if (removeTrailingSlash && result.ends_with("/")) {
            result.resize(result.size() - 1);
        }
        return result;
    }

    inline std::wstring NormalizePath(const std::wstring& path, bool removeTrailingSlash, bool makeAbsolute) {
        std::wstring result = path;
        if (makeAbsolute) {
            std::filesystem::path p(path);
            if (!p.is_absolute()) {
                p = std::filesystem::current_path() / p;
                result = p.wstring();
            }
        }
        std::replace(result.begin(), result.end(), L'\\', L'/');
        if (removeTrailingSlash && result.ends_with(L"/")) {
            result.resize(result.size() - 1);
        }
        return result;
    }
}