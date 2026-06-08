// StringResult is a header-only template; this .cpp exists so CMake can
// always include the file in the build (some generators dislike header-only
// translation units). We compile a single no-op.

#include "wzlib/StringResult.h"

namespace wzlib {
// Intentionally empty.
}  // namespace wzlib
