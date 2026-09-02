#pragma once

#include <string>
#include <memory>
#include <optional>

#include <processes/ProcessDescriptor.hpp>
#include <util/PathUtils.hpp>
#include <util/StringUtils.hpp>
#include <util/WildcardMatcher.hpp>

namespace devkit::Processes {

    class ProcessFilter {
    public:

        ProcessFilter WithExecutablePattern(const std::string& pattern) {
            executablePattern = PathUtils::NormalizePath(StringUtils::StringToWString(pattern), false, false);
            return *this;
        }

        ProcessFilter WithCommandLineArgsPattern(const std::string& pattern) {
            commandLineArgsPattern = StringUtils::StringToWString(pattern);
            return *this;
        }

        ProcessFilter WithWorkingDirectoryPattern(const std::string& pattern) {
            auto value = PathUtils::NormalizePath(StringUtils::StringToWString(pattern), true, true);
            workingDirectoryPattern = value;
            return *this;
        }

        std::string Describe() {
            if (executablePattern) {
                return StringUtils::WStringToString(*executablePattern);
            }
            if (commandLineArgsPattern) {
                return "Process with args: " + StringUtils::WStringToString(*commandLineArgsPattern);
            }
            if (workingDirectoryPattern) {
                return "Process with CWD: " + StringUtils::WStringToString(*workingDirectoryPattern);
            }
            return "Any process";
        }

        void Validate() {
            if (executablePattern) {
                return;
            }
            if (commandLineArgsPattern) {
                return;
            }
            if (workingDirectoryPattern) {
                return;
            }
            throw std::runtime_error("Invalid process filter specified");
        }

        bool Matches(ProcessDescriptor& d) {
            if (executablePattern) {
                if (!MatchProcessName(d.GetExecutablePathNormalized(), *executablePattern)) {
                    return false;
                }
            }
            if (commandLineArgsPattern) {
                if (!WildcardMatcher::MatchWildcard(d.GetCommandLineArgs(), *commandLineArgsPattern)) {
                    return false;
                }
            }
            if (workingDirectoryPattern) {
                if (!WildcardMatcher::MatchWildcard(d.GetWorkingDirectoryNormalized(), *workingDirectoryPattern)) {
                    return false;
                }
            }
            return true;
        }

    private:
        std::optional<std::wstring> executablePattern;
        std::optional<std::wstring> commandLineArgsPattern;
        std::optional<std::wstring> workingDirectoryPattern;

        bool MatchProcessName(const std::wstring& executablePathNormalized, const std::wstring& patternNormalized) {
            // Извлекаем только имя файла для сравнения
            size_t lastSlash = executablePathNormalized.find_last_of(L'/');
            std::wstring actualFileName = (lastSlash != std::wstring::npos) ?
                executablePathNormalized.substr(lastSlash + 1) : executablePathNormalized;

            // Если паттерн содержит '/', сравниваем с полным путем
            if (patternNormalized.find(L'/') != std::wstring::npos) {
                return WildcardMatcher::MatchWildcard(executablePathNormalized, patternNormalized);
            }

            // Иначе сравниваем только имена файлов
            return WildcardMatcher::MatchWildcard(actualFileName, patternNormalized);
        }
    };
}