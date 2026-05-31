#include <gtest/gtest.h>
#include "../src/logging/console_logging.hpp"
#include "../src/util/util.hpp"

using namespace devkit;

TEST(DownloadFileTests, DISABLED_DownloadFileTest) {
    info("Downloading file...");
    DownloadFile(
        "https://m.stripe.network/out-4.5.45.js",
        "C:\\Users\\User\\Downloads\\out.js",
        false,
        true
    );
    info("Done!");
}