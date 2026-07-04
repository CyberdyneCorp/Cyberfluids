#pragma once

/// Minimal, dependency-free test harness. Keeps the test suite aligned with the
/// project's "no generic third-party libraries" ethos and mobile-friendliness.
/// A test file includes this header, uses CF_CHECK / CF_CHECK_CLOSE, and returns
/// cftest::failures from main().

#include <cmath>
#include <cstdio>

namespace cftest {
inline int failures = 0;
}  // namespace cftest

#define CF_CHECK(cond)                                                     \
    do {                                                                   \
        if (!(cond)) {                                                     \
            ++cftest::failures;                                            \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                  \
    } while (0)

#define CF_CHECK_CLOSE(a, b, tol)                                          \
    do {                                                                   \
        const double _cf_d = std::fabs(static_cast<double>(a) -            \
                                       static_cast<double>(b));            \
        if (_cf_d > (tol)) {                                               \
            ++cftest::failures;                                            \
            std::printf("FAIL %s:%d: |%g - %g| = %g > %g\n", __FILE__,     \
                        __LINE__, static_cast<double>(a),                  \
                        static_cast<double>(b), _cf_d,                     \
                        static_cast<double>(tol));                         \
        }                                                                  \
    } while (0)
