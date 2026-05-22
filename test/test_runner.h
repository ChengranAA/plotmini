/* Minimal test runner -- no external dependencies */
#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdlib.h>

static int _tests_run  = 0;
static int _tests_fail = 0;
static int _tests_skip = 0;
static const char *_current_test = NULL;

#define TEST(name)                                                       \
    static int test_##name(void);                                        \
    static int test_##name(void)

#define RUN_TEST(name) do {                                              \
    _current_test = #name;                                               \
    _tests_run++;                                                        \
    int _r = test_##name();                                              \
    if      (_r == 77) { _tests_skip++; printf("  SKIP %s\n", #name); } \
    else if (_r != 0)  { _tests_fail++; printf("  FAIL %s\n", #name); } \
    else               { printf("  OK   %s\n", #name); }                \
} while(0)

#define ASSERT_TRUE(cond)  do { if (!(cond)) { printf("    %s:%d: ASSERT_TRUE(%s) failed\n", __FILE__, __LINE__, #cond); return 1; } } while(0)
#define ASSERT_FALSE(cond) do { if  ((cond)) { printf("    %s:%d: ASSERT_FALSE(%s) failed\n", __FILE__, __LINE__, #cond); return 1; } } while(0)
#define ASSERT_EQ(a, b)    do { if ((a) != (b)) { printf("    %s:%d: ASSERT_EQ(%s, %s)  %d != %d\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); return 1; } } while(0)
#define ASSERT_FEQ(a, b, eps) do { double _da = (double)(a); double _db = (double)(b); if ((_da - _db) > (eps) || (_db - _da) > (eps)) { printf("    %s:%d: ASSERT_FEQ %g != %g (eps=%g)\n", __FILE__, __LINE__, _da, _db, (double)(eps)); return 1; } } while(0)
#define SKIP(reason)       do { (void)(reason); return 77; } while(0)

#define TEST_MAIN()                                                      \
    int main(void) {                                                     \
        printf("=== %s ===\n", __FILE__);

#define TEST_MAIN_END()                                                  \
        printf("--- %d run, %d failed, %d skipped ---\n",               \
               _tests_run, _tests_fail, _tests_skip);                    \
        return _tests_fail > 0 ? 1 : 0;                                  \
    }

#endif /* TEST_RUNNER_H */
