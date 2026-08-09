#pragma once

#include <string>

namespace devkit::PathUtils {
    static inline std::string NormalizePath(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        //std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    static inline std::wstring NormalizePath(const std::wstring& path) {
        std::wstring result = path;
        std::replace(result.begin(), result.end(), L'\\', L'/');
        //std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }
}