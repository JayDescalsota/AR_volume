#include "test_common.h"

int g_failures = 0;

int runMathTests();

int main() {
    const int failuresBefore = g_failures;
    runMathTests();
    runVolumeTests();

    if (g_failures == failuresBefore) {
        std::printf("OK: all volume core tests passed\n");
        return 0;
    }
    std::printf("FAILED: %d assertion(s)\n", g_failures - failuresBefore);
    return 1;
}
