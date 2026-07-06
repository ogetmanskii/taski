#include "console_logging.hpp"
#include <Windows.h>

namespace devkit {
    static void EnableAnsiSupport() {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(out, &mode);
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    void InitConsole() {
        EnableAnsiSupport();
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        TransformUtf8Strings(false);
    }

    void ClearScreen() {
        std::cout << "\033[2J\033[H" << std::flush;
    }

    void TransformUtf8Strings(bool transform) {
        setlocale(LC_ALL, transform ? ".ACP" : "C");
    }
}