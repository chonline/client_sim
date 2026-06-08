#pragma once

/**
 * @file Log.h
 * @brief Lightweight logging for wzlib.
 *
 * The library writes diagnostic messages to stderr at four levels. By default
 * only warnings and errors are emitted. Call setLogLevel() to change this.
 *
 * The log module is intentionally minimal: it does not allocate strings, does
 * not use iostreams, and is thread-safe via a single mutex. This keeps the
 * parser hot path free of log-related overhead when logging is disabled.
 */

#include <cstdarg>
#include <cstdint>

namespace wzlib::log {

/** @brief Log severity levels. Higher numeric value = more verbose. */
enum class Level : int32_t {
    Error = 0,
    Warn  = 1,
    Info  = 2,
    Debug = 3,
};

/** @brief Set the global log level. Default: Warn. */
void setLevel(Level level);

/** @brief Get the current log level. */
Level getLevel();

/** @brief Emit an error message. Always shown (level >= Error). */
void error(const char* fmt, ...);

/** @brief Emit a warning. Shown when level >= Warn. */
void warn(const char* fmt, ...);

/** @brief Emit an info message. Shown when level >= Info. */
void info(const char* fmt, ...);

/** @brief Emit a debug message. Shown when level >= Debug. */
void debug(const char* fmt, ...);

/**
 * @brief Variadic variant accepting va_list.
 *
 * Useful for forwarding to vfprintf or wrapping in another logger. Not normally
 * called directly by user code.
 */
void emit(Level level, const char* fmt, va_list args);

}  // namespace wzlib::log
