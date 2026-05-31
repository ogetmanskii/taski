#include "args.hpp"
#include "../../include/CLI11.hpp"
#include <filesystem>
#include <iostream>
#include <optional>

namespace devkit {
    
    const Args GetArgs(int argc, char* argv[]) {
        CLI::App app;
        app.allow_windows_style_options(true);
        argv = app.ensure_utf8(argv);

        Args args {
            .currentPath = std::filesystem::current_path().string(),
            .environmentFile = "environment.yml",
            .dotEnvFile = ".env"
        };

        app.add_option("--dir,-d", args.currentPath, "Set current working directory")->check(CLI::ExistingDirectory);
        app.add_option("--file,-f", args.environmentFile, "Set file name instead of environment.yml")->check(CLI::ExistingFile);
        app.add_option("--env,-e", args.dotEnvFile, "Set dot env file instead of .env")->check(CLI::ExistingFile);
        app.add_flag("--version,-v", args.versionCommand, "Show version");
        
        auto* listCmd = app.add_subcommand("list", "List services and tasks");

        auto* upCmd = app.add_subcommand("up", "Start services");
        upCmd->add_option("services", args.upServicesList, "List of services to start. Empty list means all services")
            ->required(false);

        auto* downCmd = app.add_subcommand("down", "Stop services");
        downCmd->add_option("services", args.downServicesList, "List of services to stop. Empty list means all services")
            ->required(false);

        auto* taskCmd = app.add_subcommand("run", "Run tasks");
        taskCmd->add_option("run", args.runList, "List of tasks to run")
            ->required(true);

        auto* psCmd = app.add_subcommand("ps", "Print status of all services");

        auto* menuCmd = app.add_subcommand("menu", "Show interactive menu");

        try {
            app.parse(argc, argv);
            if (*listCmd) {
                args.listCommand = true;
            } else if (*upCmd) {
                args.upCommand = true;
            } else if (*downCmd) {
                args.downCommand = true;
            } else if (*psCmd) {
                args.printStatus = true;
            } else if (*menuCmd) {
                args.menu = true;
            }
        } catch (const CLI::ParseError& e) {
            args.exitCode = app.exit(e);
        }
        return args;
    }
}