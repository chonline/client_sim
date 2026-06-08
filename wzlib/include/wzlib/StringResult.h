#pragma once

/**
 * @file StringResult.h
 * @brief A tiny "result + log message" carrier used for human-friendly
 *        error reporting.
 *
 * Many wzlib operations want to return both a value and a string (for
 * logging). The standard Result<T> covers the value, but we also want
 * to attach a free-form string. StringResult<T> adds that.
 */

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "wzlib/ErrorCode.h"

namespace wzlib {

template <typename T>
class StringResult {
public:
    static StringResult ok(T value, std::string message = {}) {
        StringResult r;
        r.data_.template emplace<OkTag>(std::move(value), std::move(message));
        return r;
    }
    static StringResult err(ErrorCode code, std::string message) {
        StringResult r;
        r.data_.template emplace<ErrorCode>(code);
        r.message_ = std::move(message);
        return r;
    }

    bool isOk() const { return std::holds_alternative<OkTag>(data_); }
    bool isError() const { return !isOk(); }
    ErrorCode error() const { return std::get<ErrorCode>(data_); }
    const std::string& message() const { return message_; }

    const T& value() const& { return std::get<OkTag>(data_).value; }
    T& value() & { return std::get<OkTag>(data_).value; }
    T&& value() && { return std::move(std::get<OkTag>(data_).value); }

private:
    struct OkTag {
        T value;
        std::string message;
    };
    std::variant<OkTag, ErrorCode> data_;
    std::string message_;
};

}  // namespace wzlib
