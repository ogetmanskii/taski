#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
#include <thread>

#include <processes/ProcessFilter.hpp>
#include <processes/ProcessDescriptor.hpp>
#include <util/StringUtils.hpp>
#include <util/WildcardMatcher.hpp>
#include <console/Console.hpp>

namespace devkit::Processes {

    std::vector<ProcessDescriptor> GetActiveProcesses();

    bool ProcessExists(ProcessFilter& processFilter);

    bool ProcessExists(std::vector<ProcessDescriptor>& processes, ProcessFilter& processFilter);

    void TerminateProcesses(ProcessFilter& processFilter);

    inline void WaitForNoActiveProcess(ProcessFilter& processFilter) {
        Console::Info("-- Wait: {}", processFilter.Describe());
        while (Processes::ProcessExists(processFilter)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    inline void WaitForActiveProcess(ProcessFilter& processFilter) {
        Console::Info("-- Wait: {}", processFilter.Describe());
        while (!Processes::ProcessExists(processFilter)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}