#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <winternl.h>

#include <processes/Processes.hpp>
#include <processes/ProcessDescriptor.hpp>
#include <util/PathUtils.hpp>
#include <util/WildcardMatcher.hpp>

namespace devkit::Processes {

    using namespace PathUtils;
    using namespace WildcardMatcher;

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

    static inline ProcessDescriptor GetExtendedProcessInfo(DWORD processId) {
        typedef NTSTATUS(WINAPI* PNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
        static HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        
        std::wstring executablePath;
        std::wstring commandLineArgs;
        std::wstring workingDirectory;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (!hProcess) {
            return ProcessDescriptor(executablePath, commandLineArgs, workingDirectory);
        }

        wchar_t buffer[MAX_PATH];
        DWORD size = MAX_PATH;

        // imagepath
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
            executablePath = buffer;
        }

        if (ntdll) {
            auto NtQueryInformationProcess =
                (PNtQueryInformationProcess) GetProcAddress(ntdll, "NtQueryInformationProcess");

            if (NtQueryInformationProcess) {
                PROCESS_BASIC_INFORMATION pbi;
                if (NtQueryInformationProcess(hProcess, ProcessBasicInformation,
                    &pbi, sizeof(pbi), nullptr) == 0) {
                    PEB peb;

                    // cmdLine
                    if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), nullptr)) {
                        RTL_USER_PROCESS_PARAMETERS params;
                        if (ReadProcessMemory(hProcess, peb.ProcessParameters,
                            &params, sizeof(params), nullptr)) {

                            std::vector<wchar_t> buffer(params.CommandLine.Length / sizeof(wchar_t) + 1);
                            if (ReadProcessMemory(hProcess, params.CommandLine.Buffer,
                                buffer.data(), params.CommandLine.Length, nullptr)) {
                                commandLineArgs.assign(buffer.data(), params.CommandLine.Length / sizeof(wchar_t));
                                commandLineArgs = ExtractArgs(commandLineArgs);
                            }
                        }
                    }

                    // currentDir
                    SIZE_T bytesRead = 0;
                    if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &bytesRead)) {
                        BYTE* processParams = (BYTE*) peb.ProcessParameters;
                        UNICODE_STRING currentDirPath;
                        SIZE_T curDirOffset;
#ifdef _WIN64
                        curDirOffset = 0x38;
#else
                        curDirOffset = 0x24;
#endif
                        if (ReadProcessMemory(hProcess, processParams + curDirOffset,
                            &currentDirPath, sizeof(UNICODE_STRING), &bytesRead)) {

                            if (currentDirPath.Length > 0 && currentDirPath.Buffer) {
                                std::vector<wchar_t> buffer(currentDirPath.Length / sizeof(wchar_t) + 1);
                                if (ReadProcessMemory(hProcess, currentDirPath.Buffer,
                                    buffer.data(), currentDirPath.Length, &bytesRead)) {
                                    workingDirectory.assign(buffer.data(), currentDirPath.Length / sizeof(wchar_t));
                                }
                            }
                        }
                    }
                }
            }
        }
        CloseHandle(hProcess);

        return ProcessDescriptor(executablePath, commandLineArgs, workingDirectory);
    }

    // Получение полного пути к исполняемому файлу процесса
    static std::wstring GetProcessImagePath(DWORD processId) {
        std::wstring path;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

        if (hProcess) {
            CloseHandle(hProcess);
        }

        return path;
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
    static void ForEachActiveProcess(std::function<bool(DWORD processId)>& callback) {
        auto processIds = GetActiveProcessIds();

        for (DWORD pid : processIds) {
            if (!callback(pid)) {
                break;
            }
        }
    }

    static std::function<bool(DWORD)> CreateProcessExistsCallback(ProcessFilter& processFilter, bool& found) {
        found = false;
        return [&found, &processFilter](DWORD processId) -> bool {
            ProcessDescriptor d = GetExtendedProcessInfo(processId);
            if (processFilter.Matches(d)) {
                found = true;
                return false; // Stop
            }
            return true;
        };
    }

    static std::function<bool(DWORD)> CreateTerminateProcessCallback(ProcessFilter& processFilter) {
        return [&processFilter](DWORD processId) -> bool {
            ProcessDescriptor d = GetExtendedProcessInfo(processId);
            if (processFilter.Matches(d)) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
            return true;
        };
    }

    static std::function<bool(DWORD)> CreateCollectProcessCallback(std::vector<ProcessDescriptor>& vector) {
        return [&vector](DWORD processId) -> bool {
            ProcessDescriptor d = GetExtendedProcessInfo(processId);
            vector.push_back(d);
            return true;
        };
    }

    bool ProcessExists(ProcessFilter& processFilter) {
        bool found = false;
        auto callback = CreateProcessExistsCallback(processFilter, found);
        ForEachActiveProcess(callback);
        return found;
    }

    bool ProcessExists(std::vector<ProcessDescriptor>& processes, ProcessFilter& processFilter) {
        for (auto& process : processes) {
            if (processFilter.Matches(process)) {
                return true;
            }
        }
        return false;
    }

    void TerminateProcesses(ProcessFilter& processFilter) {
        processFilter.Validate();
        auto callback = CreateTerminateProcessCallback(processFilter);
        ForEachActiveProcess(callback);
    }

    std::vector<ProcessDescriptor> GetActiveProcesses() {
        std::vector<ProcessDescriptor> vector;
        auto callback = CreateCollectProcessCallback(vector);
        ForEachActiveProcess(callback);
        return vector;
    }
}