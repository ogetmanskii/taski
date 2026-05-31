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
        SetConsoleCP(CP_UTF8);
        setlocale(LC_ALL, "");
        EnableAnsiSupport();
    }

    void ClearScreen() {
        std::cout << "\033[2J\033[H" << std::flush;
    }
}