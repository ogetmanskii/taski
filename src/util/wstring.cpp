#include <string>
#include <Windows.h>

namespace devkit {
    std::wstring StringToWString(const std::string& str) {
        if (str.empty()) return std::wstring();

        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), NULL, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), &wstr[0], sizeNeeded);
        return wstr;
    }

    std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();

        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int) wstr.size(), NULL, 0, NULL, NULL);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int) wstr.size(), &str[0], sizeNeeded, NULL, NULL);
        return str;
    }
}