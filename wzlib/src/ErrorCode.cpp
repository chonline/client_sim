#include "wzlib/ErrorCode.h"

namespace wzlib {

const char* toString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:                     return "OK";
        case ErrorCode::FileNotFound:           return "FileNotFound";
        case ErrorCode::PermissionDenied:       return "PermissionDenied";
        case ErrorCode::IOError:                return "IOError";
        case ErrorCode::InvalidSignature:       return "InvalidSignature";
        case ErrorCode::UnsupportedVersion:     return "UnsupportedVersion";
        case ErrorCode::EncryptedRegionMismatch:return "EncryptedRegionMismatch";
        case ErrorCode::CorruptedHeader:        return "CorruptedHeader";
        case ErrorCode::CorruptedDirectory:    return "CorruptedDirectory";
        case ErrorCode::CorruptedImage:         return "CorruptedImage";
        case ErrorCode::ZlibDecompressFailed:   return "ZlibDecompressFailed";
        case ErrorCode::InvalidPropertyType:    return "InvalidPropertyType";
        case ErrorCode::UolResolutionFailed:    return "UolResolutionFailed";
        case ErrorCode::OutOfMemory:            return "OutOfMemory";
        case ErrorCode::PathNotFound:           return "PathNotFound";
        case ErrorCode::InvalidArgument:        return "InvalidArgument";
        case ErrorCode::NotInitialized:         return "NotInitialized";
        case ErrorCode::AlreadyInitialized:     return "AlreadyInitialized";
        case ErrorCode::Unknown:                return "Unknown";
    }
    return "Unknown";
}

}  // namespace wzlib
