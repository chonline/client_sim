// Tests for wzlib invariants the M1 UI relies on.
//
// The WzPng-decodeRgba byte-order tests run anywhere wzlib compiles.
// The WzNode::get tests are pure-C++ tests that construct node trees
// by hand, so they too run anywhere.
//
// (Real-WZ fixture tests will be added in a follow-up once the user
// has placed a KMS sample under tests/fixtures/wz/.)

#include "wztest/wztest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "wzlib/WzInt.h"
#include "wzlib/WzNode.h"
#include "wzlib/WzPng.h"
#include "wzlib/WzString.h"

using wzlib::WzInt;
using wzlib::WzNode;
using wzlib::WzPng;
using wzlib::WzString;

// ---- WzPng::decodeRgba byte order ----------------------------------------
//
// The UI's RgbaToPixmap helper assumes wzlib writes pixels in
// (R, G, B, A) order, 8 bits per channel, top-to-bottom, no padding.
// That is the byte order QImage::Format_RGBA8888 also uses, so the
// two line up with no swizzle. This test pins that contract.

TEST(WzPngDecodeRgba, ARGB8888_WritesRgbaOrder) {
    // 2x1 image: pixel A is fully-red, pixel B is fully-green, with
    // alpha = 0x80 in pixel A and 0xFF in pixel B. The packed WZ
    // bytes for ARGB8888 are the same R,G,B,A order wzlib is expected
    // to copy through.
    const uint8_t src[8] = {
        0xFF, 0x00, 0x00, 0x80,   // pixel 0: R=255 G=0   B=0   A=128
        0x00, 0xFF, 0x00, 0xFF,   // pixel 1: R=0   G=255 B=0   A=255
    };
    WzPng png(WzPng::Format::ARGB8888, /*w=*/2, /*h=*/1,
              std::vector<uint8_t>(src, src + sizeof(src)));

    const auto& out = png.decodeRgba();
    ASSERT_EQ(out.size(), std::size_t(8));
    EXPECT_EQ(out[0], 0xFF); EXPECT_EQ(out[1], 0x00);
    EXPECT_EQ(out[2], 0x00); EXPECT_EQ(out[3], 0x80);
    EXPECT_EQ(out[4], 0x00); EXPECT_EQ(out[5], 0xFF);
    EXPECT_EQ(out[6], 0x00); EXPECT_EQ(out[7], 0xFF);
}

TEST(WzPngDecodeRgba, ARGB4444_PreservesAlphaInHighNibble) {
    // ARGB4444 packs 0xARGB. A=0xF R=0x0 G=0x0 B=0x0 should expand to
    // (R=0, G=0, B=0, A=255) - i.e. transparent black with full alpha.
    const uint8_t src[2] = { 0x00, 0xF0 };  // high byte 0xF0 = A:F R:0
    WzPng png(WzPng::Format::ARGB4444, /*w=*/1, /*h=*/1,
              std::vector<uint8_t>(src, src + sizeof(src)));
    const auto& out = png.decodeRgba();
    ASSERT_EQ(out.size(), std::size_t(4));
    EXPECT_EQ(out[0], 0x00);  // R
    EXPECT_EQ(out[1], 0x00);  // G
    EXPECT_EQ(out[2], 0x00);  // B
    EXPECT_EQ(out[3], 0xFF);  // A (0xF expanded)
}

// ---- WzNode::get on a synthetic tree -------------------------------------
//
// The UI's WzTreeModel builds its tree on top of WzNode. These tests
// pin the in-memory path-resolution contract that the model's
// fetchMore() and onActivated() logic depend on.

TEST(WzNodeGet, AbsolutePathFromRoot) {
    auto root  = std::make_unique<WzNode>("");
    auto map   = std::make_unique<WzNode>("Map");
    auto back  = std::make_unique<WzNode>("Back");
    map->addChild(std::move(back));
    root->addChild(std::move(map));

    WzNode* got = root->getFromRoot("Map/Back");
    ASSERT_TRUE(got != nullptr);
    EXPECT_EQ(got->name(), std::string("Back"));
}

TEST(WzNodeGet, ParentNavigation) {
    auto root = std::make_unique<WzNode>("");
    auto a    = std::make_unique<WzNode>("A");
    auto b    = std::make_unique<WzNode>("B");
    b->addChild(std::make_unique<WzNode>("leaf"));
    a->addChild(std::move(b));
    root->addChild(std::move(a));

    WzNode* leaf = root->getFromRoot("A/B/leaf");
    ASSERT_TRUE(leaf != nullptr);
    WzNode* back = leaf->get("../..");
    ASSERT_TRUE(back != nullptr);
    EXPECT_EQ(back->name(), std::string("A"));
}

TEST(WzNodeGet, MissingSegmentReturnsNull) {
    auto root = std::make_unique<WzNode>("");
    WzNode* got = root->getFromRoot("nope");
    EXPECT_TRUE(got == nullptr);
}

TEST(WzNodeGet, ValueAccessors) {
    WzNode n("k");
    n.setValue(std::make_unique<WzInt>(42));
    auto iv = n.getInt();
    ASSERT_TRUE(iv.has_value());
    EXPECT_EQ(*iv, 42);
}

int main() {
    int failed = 0;
    failed += wztest::runAll("WzPngDecodeRgba");
    failed += wztest::runAll("WzNodeGet");
    if (failed == 0) {
        std::printf("All test_wz_structure tests passed.\n");
    }
    return failed;
}
