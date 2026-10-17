#pragma once

#include <chrono>
#include <thread>
#include <vector>

#include "console/Console.hpp"
#include "processes/ProcessDescriptor.hpp"
#include "processes/ProcessFilter.hpp"

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
}  // namespace devkit::Processes