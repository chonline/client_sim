#include "wzlib/Log.h"

#include <cstdio>
#include <mutex>

namespace wzlib::log {

namespace {
Level g_level = Level::Warn;
std::mutex g_mutex;

const char* levelPrefix(Level level) {
    switch (level) {
        case Level::Error: return "[E]";
        case Level::Warn:  return "[W]";
        case Level::Info:  return "[I]";
        case Level::Debug: return "[D]";
    }
    return "[?]";
}
}  // namespace

void setLevel(Level level) { g_level = level; }
Level getLevel() { return g_level; }

void emit(Level level, const char* fmt, va_list args) {
    if (static_cast<int32_t>(level) > static_cast<int32_t>(g_level)) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    std::fprintf(stderr, "wzlib %s ", levelPrefix(level));
    std::vfprintf(stderr, fmt, args);
    std::fputc('\n', stderr);
}

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(Level::Error, fmt, args);
    va_end(args);
}

void warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(Level::Warn, fmt, args);
    va_end(args);
}

void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(Level::Info, fmt, args);
    va_end(args);
}

void debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    emit(Level::Debug, fmt, args);
    va_end(args);
}

}  // namespace wzlib::log
