#pragma once

/**
 * @file tests/wztest/wztest.h
 * @brief Tiny single-header test framework used by wzlib's unit tests.
 *
 * Why not GTest?  GTest is not available in the build environment and
 * adding a third-party dependency for a few dozen assertions is more
 * friction than it's worth. wztest provides just enough machinery:
 *
 *   TEST(group, name) { ... }      — define a test
 *   EXPECT_EQ(a, b)                — assert equality, continue on failure
 *   EXPECT_TRUE(x)                 — assert truthy, continue on failure
 *   EXPECT_FALSE(x)                — assert falsy, continue on failure
 *   EXPECT_NOT_NULL(p)             — assert non-null, continue
 *   EXPECT_OK(r)                   — assert ErrorCode::Ok / isOk()
 *   ASSERT_EQ(a, b)                — same as EXPECT_EQ, but abort
 *   RUN_TESTS(group)               — main() helper that runs all tests
 *                                     in the given group
 *
 * Tests return void. Run them with ctest or by running the produced
 * executable directly.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace wztest {

struct TestCase {
    const char* group;
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct TestRegistrar {
    TestRegistrar(const char* group, const char* name, std::function<void()> fn) {
        registry().push_back({group, name, std::move(fn)});
    }
};

struct FailureStats {
    int total = 0;
    int failed = 0;
};

inline FailureStats& stats() {
    static FailureStats s;
    return s;
}

inline void reportFailure(const char* file, int line, const char* expr,
                          const std::string& actual = {},
                          const std::string& expected = {}) {
    stats().failed++;
    std::fprintf(stderr, "  FAIL %s:%d: %s", file, line, expr);
    if (!expected.empty() || !actual.empty()) {
        std::fprintf(stderr, "  (actual='%s', expected='%s')",
                     actual.c_str(), expected.c_str());
    }
    std::fprintf(stderr, "\n");
}

inline int runAll(const char* group) {
    std::printf("Running tests in group '%s'...\n", group);
    int ran = 0;
    for (auto& t : registry()) {
        if (std::strcmp(t.group, group) != 0) continue;
        ran++;
        stats().total++;
        std::printf("  [ RUN  ] %s.%s\n", t.group, t.name);
        t.fn();
        std::printf("  [ DONE ] %s.%s\n", t.group, t.name);
    }
    if (ran == 0) {
        std::fprintf(stderr, "  WARN: no tests registered for group '%s'\n",
                     group);
    }
    return stats().failed;
}

}  // namespace wztest

// --- Macros ---------------------------------------------------------------

#define TEST(group, name) \
    static void test_##group##_##name(); \
    static ::wztest::TestRegistrar reg_##group##_##name( \
        #group, #name, test_##group##_##name); \
    static void test_##group##_##name()

namespace wztest {
template <typename T>
inline std::string toStringForTest(const T& v) { return std::to_string(v); }
inline std::string toStringForTest(const std::string& s) { return s; }
inline std::string toStringForTest(const char* s) {
    return s ? std::string(s) : std::string("(null)");
}
}  // namespace wztest

#define EXPECT_EQ(a, b) do { \
    auto _av = (a); auto _bv = (b); \
    if (!(_av == _bv)) { \
        ::wztest::reportFailure(__FILE__, __LINE__, #a " == " #b, \
            ::wztest::toStringForTest(_av), \
            ::wztest::toStringForTest(_bv)); \
    } } while (0)

#define EXPECT_TRUE(x) do { \
    if (!(x)) { ::wztest::reportFailure(__FILE__, __LINE__, #x); } } while (0)

#define EXPECT_FALSE(x) do { \
    if ((x)) { ::wztest::reportFailure(__FILE__, __LINE__, "!(" #x ")"); } } while (0)

#define EXPECT_NOT_NULL(p) do { \
    if ((p) == nullptr) { ::wztest::reportFailure(__FILE__, __LINE__, #p); } } while (0)

#define EXPECT_OK(r) do { \
    if (!(r).isOk()) { ::wztest::reportFailure(__FILE__, __LINE__, #r ".isOk()"); } } while (0)

#define ASSERT_EQ(a, b) do { \
    auto _av = (a); auto _bv = (b); \
    if (!(_av == _bv)) { \
        ::wztest::reportFailure(__FILE__, __LINE__, #a " == " #b); \
        return; \
    } } while (0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { ::wztest::reportFailure(__FILE__, __LINE__, #x); return; } } while (0)

#define RUN_TESTS(group) \
    int main() { return ::wztest::runAll(#group); }
