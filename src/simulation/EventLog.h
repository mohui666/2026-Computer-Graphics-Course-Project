#pragma once

#include <string>
#include <vector>

namespace mine {

enum class LogLevel { Info, Warning, Error, Safety };

struct LogEntry {
    double time = 0.0;
    LogLevel level = LogLevel::Info;
    std::string source;
    std::string message;
};

class EventLog {
public:
    void add(double time, LogLevel level, std::string source, std::string message);
    void clear() { entries_.clear(); }
    [[nodiscard]] const std::vector<LogEntry>& entries() const { return entries_; }

private:
    std::vector<LogEntry> entries_;
};

const char* toString(LogLevel level);

} // namespace mine
