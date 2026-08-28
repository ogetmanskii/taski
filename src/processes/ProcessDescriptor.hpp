#pragma once

#include <string>
#include <memory>
#include <optional>

#include <util/PathUtils.hpp>

namespace devkit::Processes {
    class ProcessDescriptor {
    public:
        ProcessDescriptor(
            std::wstring executablePath, 
            std::wstring commandLineArgs, 
            std::wstring workingDirectory
        ) : executablePath(std::move(executablePath)),
            commandLineArgs(std::move(commandLineArgs)),
            workingDirectory(std::move(workingDirectory)),
            executablePathNormalized(std::nullopt),
            workingDirectoryNormalized(std::nullopt)
        { }

        std::wstring GetExecutablePath() {
            return executablePath;
        }

        std::wstring GetExecutablePathNormalized() {
            if (executablePathNormalized) {
                return *executablePathNormalized;
            }
            auto value = PathUtils::NormalizePath(executablePath, true, false);
            executablePathNormalized = value;
            return value;
        }

        std::wstring GetCommandLineArgs() {
            return commandLineArgs;
        }

        std::wstring GetWorkingDirectory() {
            return workingDirectory;
        }

        std::wstring GetWorkingDirectoryNormalized() {
            if (workingDirectoryNormalized) {
                return *workingDirectoryNormalized;
            }
            auto value = PathUtils::NormalizePath(workingDirectory, true, false);
            workingDirectoryNormalized = value;
            return value;
        }

    private:
        std::wstring executablePath;  // полный путь до исполняемого файла
        std::optional<std::wstring> executablePathNormalized;

        std::wstring commandLineArgs;  // аргументы командной строки (без исполняемого файла)

        std::wstring workingDirectory; // рабочая директория процесса
        std::optional<std::wstring> workingDirectoryNormalized;
    };
}