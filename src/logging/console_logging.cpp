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
    }

    void ClearScreen() {
        std::cout << "\033[2J\033[H" << std::flush;
    }

    void SetConsoleUtf8Encoding() {
        SetConsoleCP(CP_UTF8);
        setlocale(LC_ALL, "");
    }

    void SetConsoleDefaultEncoding() {
        SetConsoleCP(0);
        setlocale(LC_ALL, "C");
    }
}