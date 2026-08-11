#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
#include <thread>

#include <util/StringUtils.hpp>
#include <util/WildcardMatcher.hpp>
#include <console/Console.hpp>

namespace devkit::Processes {

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

    inline void WaitForNoActiveProcess(const std::string& exePath, const std::string& argsPattern) {
        if (argsPattern.empty()) {
            Console::Info("-- Waiting for process to terminate: {}", exePath);
        } else {
            Console::Info("-- Waiting for process to terminate: {}\n   with args: {}", exePath, argsPattern);
        }
        auto wExePath = StringUtils::StringToWString(exePath);
        auto wArgsPattern = StringUtils::StringToWString(argsPattern);
        while (Processes::ProcessExists(wExePath, wArgsPattern)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    inline void WaitForActiveProcess(const std::string& exePath, const std::string& argsPattern) {
        if (argsPattern.empty()) {
            Console::Info("-- Waiting for process: {}", exePath);
        } else {
            Console::Info("-- Waiting for process: {}\n   with args: {}", exePath, argsPattern);
        }
        auto wExePath = StringUtils::StringToWString(exePath);
        auto wArgsPattern = StringUtils::StringToWString(argsPattern);
        while (!Processes::ProcessExists(wExePath, wArgsPattern)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}