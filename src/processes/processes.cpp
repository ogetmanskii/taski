#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <winternl.h>

#include "processes.hpp"

namespace devkit {
    static inline std::wstring ExtractArgs(const std::wstring& fullCmdLine) {
        if (fullCmdLine.empty()) return L"";

        // Пропускаем путь к исполняемому файлу (может быть в кавычках или без)
        size_t startPos = 0;
        if (fullCmdLine[0] == L'"') {
            size_t closingQuote = fullCmdLine.find(L'"', 1);
            if (closingQuote != std::wstring::npos) {
                startPos = closingQuote + 1;
            }
        } else {
            size_t firstSpace = fullCmdLine.find(L' ');
            if (firstSpace != std::wstring::npos) {
                startPos = firstSpace + 1;
            } else {
                return L"";  // Нет аргументов
            }
        }

        // Убираем начальные пробелы
        while (startPos < fullCmdLine.length() && fullCmdLine[startPos] == L' ') {
            ++startPos;
        }

        return fullCmdLine.substr(startPos);
    }

    static inline std::wstring GetProcessCommandLine(DWORD processId) {
        std::wstring cmdLine;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

        if (hProcess) {
            // Используем NtQueryInformationProcess для получения PEB
            typedef NTSTATUS(WINAPI* PNtQueryInformationProcess)(
                HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

            HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
            if (ntdll) {
                auto NtQueryInformationProcess =
                    (PNtQueryInformationProcess) GetProcAddress(ntdll, "NtQueryInformationProcess");

                if (NtQueryInformationProcess) {
                    PROCESS_BASIC_INFORMATION pbi;
                    if (NtQueryInformationProcess(hProcess, ProcessBasicInformation,
                        &pbi, sizeof(pbi), nullptr) == 0) {

                        PEB peb;
                        if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), nullptr)) {
                            RTL_USER_PROCESS_PARAMETERS params;
                            if (ReadProcessMemory(hProcess, peb.ProcessParameters,
                                &params, sizeof(params), nullptr)) {

                                std::vector<wchar_t> buffer(params.CommandLine.Length / sizeof(wchar_t) + 1);
                                if (ReadProcessMemory(hProcess, params.CommandLine.Buffer,
                                    buffer.data(), params.CommandLine.Length, nullptr)) {
                                    cmdLine.assign(buffer.data(), params.CommandLine.Length / sizeof(wchar_t));
                                }
                            }
                        }
                    }
                }
            }
            CloseHandle(hProcess);
        }

        return cmdLine;
    }

    // Получение полного пути к исполняемому файлу процесса
    static std::wstring GetProcessImagePath(DWORD processId) {
        std::wstring path;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

        if (hProcess) {
            wchar_t buffer[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
                path = buffer;
            }
            CloseHandle(hProcess);
        }

        return path;
    }

    static ProcessInfo GetProcessInfo(DWORD processId) {
        ProcessInfo info;
        info.name = GetProcessImagePath(processId);
        std::wstring fullCmdLine = GetProcessCommandLine(processId);
        info.args = ExtractArgs(fullCmdLine);
        return info;
    }

    static std::vector<DWORD> GetActiveProcessIds() {
        std::vector<DWORD> processIds;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(pe32);

            if (Process32FirstW(snapshot, &pe32)) {
                do {
                    processIds.push_back(pe32.th32ProcessID);
                } while (Process32NextW(snapshot, &pe32));
            }
            CloseHandle(snapshot);
        }

        return processIds;
    }

    // Обход активных процессов с callback-функцией
    static void ForEachActiveProcess(std::function<bool(DWORD processId)> callback) {
        auto processIds = GetActiveProcessIds();

        for (DWORD pid : processIds) {
            if (!callback(pid)) {
                break;
            }
        }
    }

    // Проверка совпадения имени процесса (с нормализацией и поддержкой wildcard)
    static bool IsProcessNameMatch(const std::wstring& actualPath, const std::wstring& searchPattern) {
        std::wstring normalizedActual = NormalizePath(actualPath);
        std::wstring normalizedPattern = NormalizePath(searchPattern);

        // Извлекаем только имя файла из actualPath для сравнения
        size_t lastSlash = normalizedActual.find_last_of(L'/');
        std::wstring actualFileName = (lastSlash != std::wstring::npos) ?
            normalizedActual.substr(lastSlash + 1) : normalizedActual;

        // Если паттерн содержит '/', сравниваем с полным путем
        if (normalizedPattern.find(L'/') != std::wstring::npos) {
            return MatchWildcard(normalizedActual, normalizedPattern);
        }

        // Иначе сравниваем только имена файлов
        return MatchWildcard(actualFileName, normalizedPattern);
    }

    // Проверка совпадения аргументов с поддержкой wildcard
    static bool IsArgsMatch(const std::wstring& actualArgs, const std::wstring& searchPattern) {
        if (searchPattern.empty() && actualArgs.empty()) {
            return true;
        }

        std::wstring normalizedArgs = NormalizePath(actualArgs);
        std::wstring normalizedPattern = NormalizePath(searchPattern);

        return MatchWildcard(normalizedArgs, normalizedPattern);
    }

    // Callback для поиска процесса по имени и аргументам
    static std::function<bool(DWORD)> CreateProcessExistsCallback(
        const std::wstring& processName,
        const std::wstring& processArgs,
        bool& found) {
        found = false;

        return [&found, processName, processArgs](DWORD processId) -> bool {
            ProcessInfo info = GetProcessInfo(processId);

            if (IsProcessNameMatch(info.name, processName) &&
                IsArgsMatch(info.args, processArgs)) {
                found = true;
                return false;  // Прекращаем обход
            }

            return true;  // Продолжаем обход
        };
    }

    // Callback для терминации процессов по имени и аргументам
    static std::function<bool(DWORD)> CreateTerminateProcessCallback(
        const std::wstring& processName,
        const std::wstring& processArgs) {
        return [processName, processArgs](DWORD processId) -> bool {
            ProcessInfo info = GetProcessInfo(processId);

            if (IsProcessNameMatch(info.name, processName) &&
                IsArgsMatch(info.args, processArgs)) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }

            return true;  // Продолжаем обход для поиска других совпадений
        };
    }

    // Callback для сборки процессов в vector
    static std::function<bool(DWORD)> CreateCollectProcessCallback(std::vector<ProcessInfo>& vector) {
        return [&vector](DWORD processId) -> bool {
            ProcessInfo info = GetProcessInfo(processId);
            vector.push_back(info);
            return true;
        };
    }

    // Проверка существования процесса (удобная обертка)
    bool ProcessExists(const std::wstring& processName, const std::wstring& processArgs) {
        bool found = false;
        auto callback = CreateProcessExistsCallback(processName, processArgs, found);
        ForEachActiveProcess(callback);
        return found;
    }

    bool ProcessExists(
        const std::vector<ProcessInfo>& processes,
        const std::wstring& processName,
        const std::wstring& processArgs) {

        for (auto& process : processes) {
            if (IsProcessNameMatch(process.name, processName) && IsArgsMatch(process.args, processArgs)) {
                return true;
            }
        }
        return false;
    }

    // Терминация процессов (удобная обертка)
    void TerminateProcesses(const std::wstring& processName, const std::wstring& processArgs) {
        auto callback = CreateTerminateProcessCallback(processName, processArgs);
        ForEachActiveProcess(callback);
    }

    std::vector<ProcessInfo> GetActiveProcesses() {
        std::vector<ProcessInfo> vector;
        auto callback = CreateCollectProcessCallback(vector);
        ForEachActiveProcess(callback);
        return vector;
    }
}