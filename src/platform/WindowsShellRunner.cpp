#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <windows.h>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <cctype>
#include <tlhelp32.h>
#include <psapi.h>

#include <processes/Processes.hpp>
#include <ShellRunner.hpp>
#include <console/Console.hpp>
#include <console/Utf8Guard.hpp>
#include <util/StringUtils.hpp>

namespace {

    using namespace devkit;
    using namespace devkit::ShellRunner;

    std::wstring CreateEnvironmentBlock(const std::unordered_map<std::string, std::string>& extraEnv) {
        // Получаем текущее окружение процесса
        LPWCH currentEnv = GetEnvironmentStringsW();
        if (!currentEnv) {
            return L"";
        }

        // Копируем текущее окружение
        std::wstring envBlock;
        LPWCH envPtr = currentEnv;
        while (*envPtr) {
            std::wstring envVar(envPtr);
            envBlock += envVar + L'\0';
            envPtr += envVar.length() + 1;
        }

        FreeEnvironmentStringsW(currentEnv);

        // Добавляем или заменяем переменные
        for (const auto& [key, value] : extraEnv) {
            std::wstring wKey = StringUtils::StringToWString(key);
            std::wstring wValue = StringUtils::StringToWString(value);
            std::wstring newVar = wKey + L"=" + wValue;

            // Проверяем, существует ли уже такая переменная
            size_t pos = 0;
            bool found = false;
            while (pos < envBlock.size()) {
                std::wstring currentVar(&envBlock[pos]);
                size_t eqPos = currentVar.find(L'=');
                if (eqPos != std::wstring::npos) {
                    std::wstring currentKey = currentVar.substr(0, eqPos);
                    if (_wcsicmp(currentKey.c_str(), wKey.c_str()) == 0) {
                        // Заменяем существующую переменную
                        size_t varEnd = pos + currentVar.length() + 1;
                        envBlock.replace(pos, varEnd - pos, newVar + L'\0');
                        found = true;
                        break;
                    }
                }
                pos += currentVar.length() + 1;
            }

            if (!found) {
                // Добавляем новую переменную в конец
                envBlock += newVar + L'\0';
            }
        }

        // Добавляем завершающий нуль (двойной нуль в конце блока)
        envBlock += L'\0';

        return envBlock;
    }

    HANDLE CreateChildProcess(
      const std::string& command,
      const std::string& workingDirectory,
      const std::unordered_map<std::string, std::string>& environment,
      bool createNewProcessGroup,
      HANDLE& hStdOutRead,
      HANDLE& hStdOutWrite
    ) {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
            throw std::runtime_error("Failed to create pipe");
        }
        SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFOW siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));

        siStartInfo.cb = sizeof(STARTUPINFOW);
        siStartInfo.hStdOutput = hStdOutWrite;
        siStartInfo.hStdError = hStdOutWrite;
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        auto ToWide = [](const std::string& str) -> std::wstring {
            if (str.empty()) return L"";
            int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
            std::wstring result(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
            if (!result.empty() && result.back() == L'\0') {
                result.pop_back();
            }
            return result;
        };

        std::wstring wWorkingDir;
        if (!workingDirectory.empty()) {
            wWorkingDir = ToWide(workingDirectory);
            std::replace(wWorkingDir.begin(), wWorkingDir.end(), L'/', L'\\');
        }

        std::string cmdLine = "cmd.exe /c " + command;
        std::wstring wCmdLine = ToWide(cmdLine);

        // Для CreateProcessW нужна модифицируемая строка с запасом
        wCmdLine.push_back(L'\0');
        wCmdLine.push_back(L'\0');

        std::wstring envBlock = CreateEnvironmentBlock(environment);

        DWORD creationFlags = CREATE_UNICODE_ENVIRONMENT;
        if (createNewProcessGroup) {
            creationFlags |= CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
        }

        BOOL bSuccess = CreateProcessW(
            NULL,
            &wCmdLine[0],
            NULL,
            NULL,
            TRUE,
            creationFlags,
            envBlock.empty() ? NULL : (LPVOID) envBlock.data(),
            wWorkingDir.empty() ? NULL : wWorkingDir.c_str(),
            &siStartInfo,
            &piProcInfo
        );

        if (!bSuccess) {
            DWORD error = GetLastError();
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);

            std::stringstream ss;
            ss << "Failed to create process. Error: " << error;
            throw std::runtime_error(ss.str());
        }

        CloseHandle(piProcInfo.hThread);
        CloseHandle(hStdOutWrite);

        return piProcInfo.hProcess;
    }

    void ReadProcessOutput(HANDLE hStdOutRead, HANDLE hProcess, bool printToConsole = true) {
        CHAR chBuffer[4096];
        DWORD dwRead;
        BOOL bSuccess;

        while (true) {
            DWORD waitResult = WaitForSingleObject(hProcess, 0);
            if (waitResult == WAIT_OBJECT_0) {
                DWORD dwAvail;
                if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &dwAvail, NULL) && dwAvail > 0) {
                    bSuccess = ReadFile(hStdOutRead, chBuffer, min(sizeof(chBuffer) - 1, dwAvail), &dwRead, NULL);
                    if (bSuccess && dwRead > 0) {
                        chBuffer[dwRead] = '\0';
                        if (printToConsole) {
                            std::cout << chBuffer;
                            std::cout.flush();
                        }
                    }
                }
                break;
            }

            DWORD dwAvail;
            if (!PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &dwAvail, NULL) || dwAvail == 0) {
                Sleep(10);
                continue;
            }
            bSuccess = ReadFile(hStdOutRead, chBuffer, sizeof(chBuffer) - 1, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) {
                break;
            }

            chBuffer[dwRead] = '\0';
            if (printToConsole) {
                std::cout << chBuffer;
                std::cout.flush();
            }
        }
    }

    void TerminateProcessTree(DWORD processId) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };

        if (Process32First(hSnapshot, &pe)) {
            do {
                if (pe.th32ParentProcessID == processId) {
                    HANDLE hChild = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hChild) {
                        TerminateProcess(hChild, 1);
                        CloseHandle(hChild);
                    }
                    TerminateProcessTree(pe.th32ProcessID);
                }
            } while (Process32Next(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (hProcess) {
            TerminateProcess(hProcess, 1);
            CloseHandle(hProcess);
        }
    }

    inline bool FindAndErase(std::string& text, const std::string& searchText) {
        int result = text.find(searchText);
        // Удаляем начало строки, оставляем только окончание равное длине searchText + запас 8 символов
        if (text.size() > (searchText.size() + 8)) {
            text.erase(0, text.size() - searchText.size() - 8);
        }
        return result != std::string::npos;
    }

    // Простой запуск с ожиданием завершения
    RunResult RunCommand(
        const std::string& command,
        const std::string& workingDirectory,
        const std::unordered_map<std::string, std::string>& environment,
        bool createNewProcessGroup,
        float timeoutSeconds
    ) {
        HANDLE hStdOutRead, hStdOutWrite;
        HANDLE hProcess = CreateChildProcess(command, workingDirectory, environment, createNewProcessGroup, hStdOutRead, hStdOutWrite);

        std::thread outputThread([hStdOutRead, hProcess]() {
            ReadProcessOutput(hStdOutRead, hProcess, true);
        });

        DWORD exitCode;

        if (timeoutSeconds < 0) {
            WaitForSingleObject(hProcess, INFINITE);
            GetExitCodeProcess(hProcess, &exitCode);
        } else {
            DWORD waitResult = WaitForSingleObject(hProcess, timeoutSeconds * 1000);
            if (waitResult == WAIT_OBJECT_0) {
                GetExitCodeProcess(hProcess, &exitCode);
            } else if (waitResult == WAIT_TIMEOUT) {
                TerminateProcessTree(GetProcessId(hProcess));
                outputThread.join();
                CloseHandle(hStdOutRead);
                CloseHandle(hProcess);
                return RunResult {
                    .exitCode = std::nullopt,
                    .timedOut = true,
                    .errorCode = std::nullopt
                };
            } else {
                outputThread.join();

                CloseHandle(hStdOutRead);
                CloseHandle(hProcess);

                return RunResult {
                   .exitCode = std::nullopt,
                   .timedOut = false,
                   .errorCode = GetLastError()
                };
            }
        }
        outputThread.join();
        CloseHandle(hStdOutRead);
        CloseHandle(hProcess);

        return RunResult {
            .exitCode = exitCode,
            .timedOut = false,
            .errorCode = std::nullopt
        };
    }

    // Запуск с отсоединением после указанного времени
    RunResult RunCommand(
        const std::string& command,
        const std::string& workingDirectory,
        const std::unordered_map<std::string, std::string>& environment,
        float detachAfterSeconds,
        bool createNewProcessGroup
    ) {
        if (detachAfterSeconds <= 0) {
            HANDLE hStdOutRead, hStdOutWrite;
            HANDLE hProcess = CreateChildProcess(command, workingDirectory, environment, createNewProcessGroup, hStdOutRead, hStdOutWrite);

            CloseHandle(hStdOutRead);
            CloseHandle(hProcess);
            return RunResult {
                .exitCode = std::nullopt,
                .timedOut = false,
                .errorCode = std::nullopt
            };
        }

        HANDLE hStdOutRead, hStdOutWrite;
        HANDLE hProcess = CreateChildProcess(command, workingDirectory, environment, createNewProcessGroup, hStdOutRead, hStdOutWrite);

        std::atomic<bool> shouldStop { false };
        std::string fullOutput;

        // Поток для чтения вывода
        std::thread outputThread([hStdOutRead, &shouldStop, &fullOutput]() {
            CHAR chBuffer[4096];
            DWORD dwRead;

            while (!shouldStop) {
                // Неблокирующее чтение с таймаутом
                DWORD bytesAvailable = 0;
                if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
                    if (ReadFile(hStdOutRead, chBuffer, min(sizeof(chBuffer) - 1, bytesAvailable), &dwRead, NULL)) {
                        chBuffer[dwRead] = '\0';
                        fullOutput += chBuffer;
                        std::cout << chBuffer;
                        std::cout.flush();
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        });

        // Ждем указанное время или завершения процесса
        DWORD waitResult = WaitForSingleObject(hProcess, detachAfterSeconds * 1000);

        if (waitResult == WAIT_TIMEOUT) {
            shouldStop = true;
            outputThread.join();
            CloseHandle(hStdOutRead);
            CloseHandle(hProcess);
            return RunResult {
                .exitCode = std::nullopt,
                .timedOut = false,
                .errorCode = std::nullopt
            };
        } else {
            shouldStop = true;
            outputThread.join();
            DWORD exitCode;
            GetExitCodeProcess(hProcess, &exitCode);
            CloseHandle(hStdOutRead);
            CloseHandle(hProcess);
            return RunResult {
                .exitCode = exitCode,
                .timedOut = false,
                .errorCode = std::nullopt
            };
        }
    }

    // Запуск с отсоединением после получения определенного сообщения
    RunResult RunCommand(
        const std::string& command,
        const std::string& workingDirectory,
        const std::unordered_map<std::string, std::string>& environment,
        const std::string& detachAfterMessage,
        bool createNewProcessGroup,
        float timeoutSeconds
    ) {
        HANDLE hStdOutRead, hStdOutWrite;
        HANDLE hProcess = CreateChildProcess(command, workingDirectory, environment, createNewProcessGroup, hStdOutRead, hStdOutWrite);
        auto startedAt = std::chrono::high_resolution_clock::now();

        std::string accumulatedOutput;
        std::atomic<DWORD> processExitCode { 0 };
        std::atomic<bool> messageFound { false };
        std::atomic<bool> timedOut { false };
        std::atomic<bool> shouldStop { false };
        std::atomic<bool> processEnded { false };

        // Поток для чтения и поиска сообщения
        std::thread outputThread([hStdOutRead, &accumulatedOutput, &messageFound, &shouldStop, &detachAfterMessage, hProcess, &processEnded, &processExitCode]() {
            CHAR chBuffer[4096];
            DWORD dwRead;

            while (!shouldStop && !processEnded) {
                DWORD bytesAvailable = 0;
                if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL)) {
                    if (bytesAvailable > 0) {
                        if (ReadFile(hStdOutRead, chBuffer, min(sizeof(chBuffer) - 1, bytesAvailable), &dwRead, NULL)) {
                            chBuffer[dwRead] = '\0';
                            std::string newOutput(chBuffer);
                            accumulatedOutput += newOutput;
                            std::cout << newOutput;
                            std::cout.flush();

                            // Проверяем, содержит ли вывод искомое сообщение
                            if (!messageFound && FindAndErase(accumulatedOutput, detachAfterMessage)) {
                                messageFound = true;
                                break;
                            }
                        }
                    } else {
                        // Проверяем, не завершился ли процесс
                        DWORD exitCode;
                        if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                            processEnded = true;
                            processExitCode = exitCode;
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                } else {
                    // Ошибка в PeekNamedPipe, возможно процесс завершился
                    DWORD exitCode;
                    if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                        processEnded = true;
                        processExitCode = exitCode;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            // Читаем оставшийся вывод
            if (messageFound || processEnded) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                DWORD bytesAvailable = 0;
                while (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
                    if (ReadFile(hStdOutRead, chBuffer, min(sizeof(chBuffer) - 1, bytesAvailable), &dwRead, NULL)) {
                        chBuffer[dwRead] = '\0';
                        accumulatedOutput += chBuffer;
                        std::cout << chBuffer;
                        std::cout.flush();

                        if (!messageFound && FindAndErase(accumulatedOutput, detachAfterMessage)) {
                            messageFound = true;
                        }
                    } else {
                        break;
                    }
                }
            }
        });

        // Ждем либо нахождения сообщения, либо завершения процесса
        while (!messageFound && !processEnded && !timedOut) {
            auto elapsedTime = std::chrono::high_resolution_clock::now() - startedAt;
            if (timeoutSeconds >= 0 && std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() >= timeoutSeconds) {
                // timeout
                timedOut = true;
                TerminateProcessTree(GetProcessId(hProcess));
                break;
            }
            DWORD exitCode;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                processEnded = true;
                processExitCode = exitCode;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Даем потоку время для завершения чтения оставшихся данных
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        shouldStop = true;
        if (outputThread.joinable()) {
            outputThread.join();
        }
        CloseHandle(hStdOutRead);
        CloseHandle(hProcess);

        if (processEnded) {
            return RunResult {
                .exitCode = processExitCode,
                .timedOut = timedOut,
                .errorCode = std::nullopt
            };
        } else {
            return RunResult {
                .exitCode = std::nullopt,
                .timedOut = timedOut,
                .errorCode = std::nullopt
            };
        }
    }
}

namespace devkit::ShellRunner {

    RunResult Run(const RunSpec spec) {
        Console::Utf8Guard utf8(spec.utf8);
        if (spec.detachAfterSeconds.has_value()) {
            return RunCommand(
                spec.command,
                spec.workingDirectory,
                spec.environment,
                *spec.detachAfterSeconds,
                spec.createNewProcessGroup
            );
        } else if (spec.detachAfterMessage.has_value()) {
            return RunCommand(
                spec.command,
                spec.workingDirectory,
                spec.environment,
                *spec.detachAfterMessage,
                spec.createNewProcessGroup,
                spec.timeoutSeconds
            );
        } else {
            return RunCommand(
                spec.command,
                spec.workingDirectory,
                spec.environment,
                spec.createNewProcessGroup,
                spec.timeoutSeconds
            );
        }
    }
}