#pragma once

#include <string>
#include <sstream>

namespace devkit::Console {
    class DurationFormatter {
    public:
        static std::string Format(long long millis) {
            if (millis < 0) millis = 0;

            long long totalSeconds = millis / 1000;
            long long days = totalSeconds / 86400;
            long long hours = (totalSeconds % 86400) / 3600;
            long long minutes = (totalSeconds % 3600) / 60;
            long long seconds = totalSeconds % 60;
            long long remainingMillis = millis % 1000;

            std::ostringstream result;

            AppendIfNonZero(result, days, "d");
            AppendIfNonZero(result, hours, "h");
            AppendIfNonZero(result, minutes, "m");

            if (seconds > 0 || remainingMillis > 0 || result.tellp() == 0) {
                if (remainingMillis > 0) {
                    result << seconds << "."
                        << std::setfill('0') << std::setw(3) << remainingMillis << "s";
                } else {
                    result << seconds << "s";
                }
            }

            if (result.tellp() == 0) {
                return "0s";
            }

            std::string str = result.str();
            if (!str.empty() && str.back() == ' ') {
                str.pop_back();
            }
            return str;
        }

    private:
        static void AppendIfNonZero(std::ostringstream& oss, long long value, const char* suffix) {
            if (value > 0) {
                oss << value << suffix << " ";
            }
        }
    };
}