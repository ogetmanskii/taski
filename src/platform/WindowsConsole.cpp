#include <iostream>
#include <iomanip>
#define NOMINMAX
#include <Windows.h>

#include <console/Console.hpp>

namespace devkit::Console {

    void EnableAnsiSupport() {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(out, &mode);
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    void Init() {
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

    std::pair<int /* width */, int /* height */> GetConsoleBounds() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            return std::make_pair(
                csbi.srWindow.Right - csbi.srWindow.Left + 1,
                csbi.srWindow.Bottom - csbi.srWindow.Top + 1
            );
        }
        return std::make_pair(80, 25);
    }
}