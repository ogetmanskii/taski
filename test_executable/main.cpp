#include <iostream>
#include <unordered_map>
#include <string_view>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>

std::vector<std::pair<std::string, std::string>> ParseArgs(int argc, char* argv[]) {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(argc / 2);
    for (int i = 1; i < argc; ++i) {
        std::string v { argv[i] };
        if (v.starts_with("--")) {
            // Новая опция
            result.emplace_back(v, std::string {});
        } else if (!result.empty()) {
            // Значение
            std::string& value = result.back().second;
            if (value.empty()) {
                value = v;
            } else {
                value += ' ';
                value += v;
            }
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    auto args = ParseArgs(argc, argv);
    if (args.empty()) {
        std::cout << "Usage:\n   --sleep [seconds]\n   --print Something\n   --print-env ENV_VAR\n";
        return 0;
    }
    for (auto it = args.begin(); it != args.end(); it++) {
        auto& pair = *it;
        if (pair.first == "--sleep") {
            int seconds = std::stoi(std::string(pair.second));
            if (seconds > 0) {
                for (int i = seconds; i > 0; i--) {
                    std::cout << "Waiting " << i << " seconds..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        } else if (pair.first == "--print") {
            std::cout << pair.second << std::endl;
        } else if (pair.first == "--print-env") {
            std::string envName = pair.second;
            if (!envName.empty()) {
                char* v = std::getenv(envName.c_str());
                if (v != nullptr) {
                    std::cout << envName << " = " << std::string(v) << std::endl;
                } else {
                    std::cout << envName << " is null" << std::endl;
                }
            }
        }
    }
}