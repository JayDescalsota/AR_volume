#pragma once

#include <cmath>
#include <cstdio>

extern int g_failures;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(actual, expected, tol)                                        \
    do {                                                                         \
        const double a_ = (actual);                                              \
        const double e_ = (expected);                                            \
        if (!(std::fabs(a_ - e_) <= (tol))) {                                    \
            std::printf("FAIL %s:%d  %s = %.9f, expected %.9f +/- %.9f\n",       \
                        __FILE__, __LINE__, #actual, a_, e_, (double)(tol));     \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

int runMathTests();
int runVolumeTests();
