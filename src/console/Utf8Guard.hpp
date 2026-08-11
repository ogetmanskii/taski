#pragma once

#include <console/Console.hpp>

namespace devkit::Console {
    class Utf8Guard {
    public:

        Utf8Guard() = delete;

        Utf8Guard(bool forceUtf8)
            : forceUtf8(forceUtf8) {

            if (forceUtf8) {
                TransformUtf8Strings(true);
            }
        }

        ~Utf8Guard() {
            TransformUtf8Strings(false);
        }

        const bool forceUtf8;
    };
}