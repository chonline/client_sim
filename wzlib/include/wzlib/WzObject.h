#pragma once

/**
 * @file WzObject.h
 * @brief Base class for all WZ value objects.
 *
 * Every value stored in a WzNode (other than a child node) is a WzObject
 * subclass. Concrete types include:
 *   - WzInt / WzDouble / WzString / WzVector  — scalars
 *   - WzUol                                    — symbol link
 *   - WzPng                                    — bitmap (raw or compressed)
 *   - WzSound                                  — audio
 *
 * The base class provides type() for runtime discrimination and convenience
 * accessors (asInt() / asString() / etc.) that return std::optional so the
 * caller doesn't need to know the concrete type.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wzlib {

class WzInt;
class WzDouble;
class WzString;
class WzVector;
class WzUol;
class WzPng;
class WzSound;

/** @brief Discriminator for WzObject subclasses. */
enum class ObjectType : uint8_t {
    Null      = 0,
    Short     = 2,    ///< 16-bit signed integer.
    Int       = 3,    ///< 32-bit signed integer.
    Long      = 20,   ///< 64-bit signed integer.
    Float     = 4,    ///< IEEE 754 single precision.
    Double    = 5,    ///< IEEE 754 double precision.
    String    = 8,    ///< UTF-8 string (decrypted from WZ format).
    Uol       = 11,   ///< Symbol link to another node.
    Vector2D  = 16,   ///< 2D integer vector.
    Convex    = 17,   ///< Convex polygon.
    Png       = 19,   ///< Bitmap (decoded into RGBA pixel buffer).
    Sound     = 22,   ///< Audio (MP3 or PCM).
    RawData   = 23,   ///< Opaque byte sequence.
    Video     = 25,   ///< Video reference.
    Property  = 30,   ///< A nested property table (technically a WzNode).
    Unknown   = 0xFF,
};

class WzObject {
public:
    virtual ~WzObject() = default;
    virtual ObjectType type() const = 0;

    /** @brief Returns this object as a WzInt, or nullptr if it isn't one. */
    virtual const WzInt*    asInt()    const { return nullptr; }
    virtual const WzDouble* asDouble() const { return nullptr; }
    virtual const WzString* asString() const { return nullptr; }
    virtual const WzVector* asVector() const { return nullptr; }
    virtual const WzUol*    asUol()    const { return nullptr; }
    virtual const WzPng*    asPng()    const { return nullptr; }
    virtual const WzSound*  asSound()  const { return nullptr; }

    /** @brief Convenience: returns the int value, or nullopt if not an int. */
    std::optional<int32_t>    tryInt()    const;
    /** @brief Convenience: returns the double value, or nullopt if not numeric. */
    std::optional<double>     tryDouble() const;
    /** @brief Convenience: returns the string value, or nullopt if not a string. */
    std::optional<std::string> tryString() const;

protected:
    WzObject() = default;
};

}  // namespace wzlib
