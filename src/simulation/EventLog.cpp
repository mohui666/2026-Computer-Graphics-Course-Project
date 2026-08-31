#include "simulation/EventLog.h"

#include <utility>

namespace mine {

void EventLog::add(double time, LogLevel level, std::string source, std::string message) {
    entries_.push_back({time, level, std::move(source), std::move(message)});
    if (entries_.size() > 600) {
        entries_.erase(entries_.begin(), entries_.begin() + 100);
    }
}

const char* toString(LogLevel level) {
    switch (level) {
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Safety: return "SAFETY";
    }
    return "UNKNOWN";
}

} // namespace mine
