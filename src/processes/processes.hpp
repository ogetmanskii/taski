#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include "../util/match_wildcard.hpp"
#include "../util/util.hpp"

namespace devkit {

    struct ProcessInfo {
        std::wstring name;  // полный путь до исполняемого файла
        std::wstring args;  // аргументы командной строки (без исполняемого файла)
    };

    std::vector<ProcessInfo> GetActiveProcesses();

    bool ProcessExists(
        const std::wstring& processName, 
        const std::wstring& processArgs = L"*"
    );

    bool ProcessExists(
        const std::vector<ProcessInfo>& processes,
        const std::wstring& processName, 
        const std::wstring& processArgs = L"*"
    );

    void TerminateProcesses(
        const std::wstring& processName, 
        const std::wstring& processArgs = L"*"
    );

}