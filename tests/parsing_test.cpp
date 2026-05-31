#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include "../src/logging/console_logging.hpp"
#include "../src/util/util.hpp"
#include "../src/parsing/parsing.hpp"
#include "../src/services/services.hpp"
#include "../src/processes/processes.hpp"

using namespace devkit;

const std::filesystem::path SAMPLES_PATH = std::filesystem::current_path() / ".." / ".." / "tests" / "samples";

TEST(ParsingTests, ParsingTestsInit) {
    InitConsole();
}

//TEST(ParsingTests, ParseYamlFile) {
//    std::vector<Service> services = ParseServicesYml(SAMPLES_PATH / "services.yml", {});
//    auto processes = GetActiveProcesses();
//    for (auto& service : services) {
//        service.Status(processes);
//    }
//    ASSERT_EQ(services.size(), 3);
//    ASSERT_EQ(services[0].definition.name, "Nginx");
//}