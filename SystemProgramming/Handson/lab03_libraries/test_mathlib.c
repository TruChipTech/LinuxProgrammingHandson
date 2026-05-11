/*
 * test_mathlib.c — Test program for mathlib (static & dynamic)
 *
 * Static:  gcc test_mathlib.c -L. -lmathutil -o test_static -lm
 * Dynamic: gcc test_mathlib.c -L. -lmathutil -o test_dynamic -lm
 *          LD_LIBRARY_PATH=. ./test_dynamic
 */

#include <stdio.h>
#include "mathlib.h"

int main(void) {
    printf("=== MathLib Test Suite ===\n");
    printf("Library version: %s\n\n", mathlib_version());

    /* Basic operations */
    printf("--- Basic Operations ---\n");
    printf("add(10, 25)       = %d\n", mathlib_add(10, 25));
    printf("subtract(100, 42) = %d\n", mathlib_subtract(100, 42));
    printf("multiply(7, 8)    = %d\n", mathlib_multiply(7, 8));
    printf("divide(22, 7)     = %.4f\n", mathlib_divide(22.0, 7.0));
    printf("factorial(10)     = %d\n", mathlib_factorial(10));
    printf("gcd(48, 18)       = %d\n", mathlib_gcd(48, 18));
    printf("lcm(12, 18)       = %d\n", mathlib_lcm(12, 18));

    /* Advanced operations */
    printf("\n--- Advanced Operations ---\n");
    printf("power(2, 10)      = %.0f\n", mathlib_power(2, 10));
    printf("sqrt_newton(144)  = %.4f\n", mathlib_sqrt_newton(144.0));
    printf("pi(1000000 terms) = %.10f\n", mathlib_pi_leibniz(1000000));
    printf("is_prime(97)      = %s\n", mathlib_is_prime(97) ? "yes" : "no");
    printf("is_prime(100)     = %s\n", mathlib_is_prime(100) ? "yes" : "no");

    /* Fibonacci */
    int fib[15];
    mathlib_fibonacci(fib, 15);
    printf("fibonacci(15)     = ");
    for (int i = 0; i < 15; i++) printf("%d ", fib[i]);
    printf("\n");

    /* Statistics */
    double data[] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    int n = sizeof(data) / sizeof(data[0]);
    printf("mean(data)        = %.4f\n", mathlib_mean(data, n));
    printf("stddev(data)      = %.4f\n", mathlib_stddev(data, n));

    printf("\n=== All Tests Passed ===\n");
    return 0;
}
