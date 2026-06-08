#pragma once

/**
 * @file ErrorCode.h
 * @brief wzlib error codes and Result<T> wrapper.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace wzlib {

/**
 * @brief Error codes returned by wzlib functions.
 *
 * Functions that can fail return either a value (on success) or an ErrorCode
 * (on failure) via the Result<T> wrapper. The codes are intentionally coarse
 * — they describe the *category* of failure, not the specific cause. Use
 * wzlib::log for the detailed human-readable message.
 */
enum class ErrorCode : int32_t {
    Ok = 0,
    FileNotFound,             ///< The requested file does not exist on disk.
    PermissionDenied,         ///< The OS refused access (read-only, locked, ...).
    IOError,                  ///< Generic I/O failure (seek, read, write, ...).
    InvalidSignature,         ///< File does not start with PKG1/PKG2.
    UnsupportedVersion,       ///< File version is newer than what we support.
    EncryptedRegionMismatch,  ///< The IV / region / hash does not match expected.
    CorruptedHeader,          ///< Header fields have invalid values.
    CorruptedDirectory,       ///< Directory tree structure is invalid.
    CorruptedImage,           ///< A WzImage failed checksum or structure check.
    ZlibDecompressFailed,     ///< zlib returned an error.
    InvalidPropertyType,      ///< Unknown Property type tag.
    UolResolutionFailed,      ///< Could not resolve a UOL (broken path or missing target).
    OutOfMemory,              ///< Allocation failed.
    PathNotFound,             ///< get() / findNode() returned no result.
    InvalidArgument,          ///< Caller passed a bad argument.
    NotInitialized,           ///< WzCryptoContext not initialized.
    AlreadyInitialized,       ///< Attempted to re-initialize an already-set context.
    Unknown,                  ///< Unclassified error.
};

/**
 * @brief Returns a short, human-readable string for an ErrorCode.
 */
const char* toString(ErrorCode code);

/**
 * @brief Result<T> is the standard return type for fallible wzlib functions.
 *
 * Usage:
 *   Result<std::unique_ptr<WzFile>> WzFile::open(const std::string& path);
 *   auto r = WzFile::open("Base.wz");
 *   if (r.isError()) {
 *       std::cerr << toString(r.error()) << "\n";
 *       return;
 *   }
 *   auto file = std::move(r).value();
 */
template <typename T>
class Result {
public:
    /** @brief Construct a successful result. */
    Result(T value) : data_(std::move(value)) {}

    /** @brief Construct an error result. */
    static Result error(ErrorCode code) {
        Result r{T{}};
        r.data_ = code;
        return r;
    }

    /** @brief True if this is a successful result. */
    bool isOk() const { return std::holds_alternative<T>(data_); }

    /** @brief True if this is an error result. */
    bool isError() const { return !isOk(); }

    /** @brief Get the value. Precondition: isOk(). */
    const T& value() const& { return std::get<T>(data_); }
    T& value() & { return std::get<T>(data_); }
    T&& value() && { return std::move(std::get<T>(data_)); }

    /** @brief Get the error code. Precondition: isError(). */
    ErrorCode error() const { return std::get<ErrorCode>(data_); }

private:
    std::variant<T, ErrorCode> data_;
};

/**
 * @brief Specialization of Result for void-returning operations.
 */
class VoidResult {
public:
    VoidResult() = default;
    explicit VoidResult(ErrorCode code) : code_(code) {}

    bool isOk() const { return code_ == ErrorCode::Ok; }
    bool isError() const { return !isOk(); }
    ErrorCode error() const { return code_; }

    bool operator==(ErrorCode c) const { return code_ == c; }
    bool operator!=(ErrorCode c) const { return code_ != c; }
    bool operator==(const VoidResult& o) const { return code_ == o.code_; }
    bool operator!=(const VoidResult& o) const { return code_ != o.code_; }

private:
    ErrorCode code_ = ErrorCode::Ok;
};

}  // namespace wzlib
