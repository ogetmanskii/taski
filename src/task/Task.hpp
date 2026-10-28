#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShellRunner.hpp"
#include "util/StringUtils.hpp"

namespace devkit {

struct Task {
  // name
  const std::string name;

  // group
  const std::optional<std::string> group;

  // description
  const std::optional<std::string> description;

  // hidden
  const bool hidden;

  // utf8
  const bool utf8;

  // depends-on
  const std::vector<std::string> dependsOn;

  // before
  const std::vector<std::string> before;

  // after
  const std::vector<std::string> after;

  // cmd
  const std::vector<std::string> commands;

  // work-dir
  const std::optional<std::filesystem::path> workingDirectory;

  // env
  const std::optional<std::unordered_map<std::string, std::string>> env;

  // timeout
  const float timeout;

  // exit-codes
  const std::vector<int> exitCodes;

  void Run() const {
    std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();

    ShellRunner::RunSpec spec = {.command = StringUtils::JoinShellCommands(commands),
                                 .workingDirectory = cwd,
                                 .environment = env.value_or(std::unordered_map<std::string, std::string>()),
                                 .utf8 = utf8,
                                 .createNewProcessGroup = false,
                                 .detachAfterSeconds = std::nullopt,
                                 .detachAfterMessage = std::nullopt,
                                 .timeoutSeconds = timeout};

    ShellRunner::RunResult result = ShellRunner::Run(spec);
    if (result.timedOut) {
      throw std::runtime_error("Task " + name + " timed out");
    }
    if (!ShellRunner::IsValidExitCode(result, exitCodes)) {
      throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(*result.exitCode));
    }
    if (result.errorCode) {
      throw std::runtime_error("Task " + name + " failed: error " + std::to_string(*result.errorCode));
    }
  }
};

}  // namespace devkit