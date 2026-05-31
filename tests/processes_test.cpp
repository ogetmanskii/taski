#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "../src/logging/console_logging.hpp"
#include "../src/processes/processes.hpp"
#include "../src/util/util.hpp"

using namespace devkit;

TEST(MainTests, Init) {
	devkit::InitConsole();
}

TEST(MainTests, GetActiveProcesses) {
	std::vector<ProcessInfo> processes = GetActiveProcesses();

	for (ProcessInfo& processInfo : processes) {
		info("Active process: {},\nargs: {}\n", WStringToString(processInfo.name), WStringToString(processInfo.args));
	}

	ASSERT_TRUE(processes.size() > 0);
}

TEST(MainTests, IsActiveProcess) {
	std::vector<ProcessInfo> processes = GetActiveProcesses();

	for (ProcessInfo& process : processes) {
		ASSERT_TRUE(ProcessExists(processes, process.name));
	}

	std::wstring notExistantProcessName = L"E:/this-process-not-exists.exe";
	ASSERT_FALSE(ProcessExists(processes, notExistantProcessName));
}