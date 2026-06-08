#include "wztest/wztest.h"

#include "wzlib/WzInt.h"
#include "wzlib/WzNode.h"
#include "wzlib/WzString.h"
#include "wzlib/WzUol.h"

using namespace wzlib;

TEST(wznode, add_and_find_child) {
    WzNode root("");
    auto a = std::make_unique<WzNode>("a", &root);
    a->setValue(std::make_unique<WzInt>(42));
    root.addChild(std::move(a));

    WzNode* ca = root.child("a");
    EXPECT_NOT_NULL(ca);
    auto v = ca->getInt();
    EXPECT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

TEST(wznode, get_with_path) {
    WzNode root("");
    auto a = std::make_unique<WzNode>("a", &root);
    auto b = std::make_unique<WzNode>("b", a.get());
    b->setValue(std::make_unique<WzString>("hello"));
    a->addChild(std::move(b));
    root.addChild(std::move(a));

    WzNode* nb = root.get("a/b");
    EXPECT_NOT_NULL(nb);
    auto v = nb->getString();
    EXPECT_TRUE(v.has_value());
    EXPECT_EQ(*v, std::string("hello"));
}

TEST(wznode, get_with_leading_slash) {
    WzNode root("");
    auto x = std::make_unique<WzNode>("x", &root);
    x->setValue(std::make_unique<WzInt>(7));
    root.addChild(std::move(x));

    WzNode* nx = root.get("/x");
    EXPECT_NOT_NULL(nx);
    auto v = nx->getInt();
    EXPECT_TRUE(v.has_value());
    EXPECT_EQ(*v, 7);
}

TEST(wznode, full_path) {
    WzNode root("");
    auto a = std::make_unique<WzNode>("a", &root);
    auto b = std::make_unique<WzNode>("b", a.get());
    a->addChild(std::move(b));
    root.addChild(std::move(a));

    WzNode* nb = root.get("a/b");
    EXPECT_NOT_NULL(nb);
    EXPECT_EQ(nb->fullPath(), std::string("a/b"));
}

TEST(wznode, root_returns_top) {
    WzNode root("");
    auto a = std::make_unique<WzNode>("a", &root);
    auto b = std::make_unique<WzNode>("b", a.get());
    a->addChild(std::move(b));
    root.addChild(std::move(a));

    WzNode* nb = root.get("a/b");
    EXPECT_NOT_NULL(nb);
    EXPECT_TRUE(nb->root() == &root);
}

RUN_TESTS(wznode)
