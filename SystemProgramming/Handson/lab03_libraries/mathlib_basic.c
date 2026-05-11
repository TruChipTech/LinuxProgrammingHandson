/*
 * mathlib_basic.c — Basic Math Utility Functions
 */

#include "mathlib.h"

const char *mathlib_version(void) {
    return "mathlib 1.0.0";
}

int mathlib_add(int a, int b) {
    return a + b;
}

int mathlib_subtract(int a, int b) {
    return a - b;
}

int mathlib_multiply(int a, int b) {
    return a * b;
}

double mathlib_divide(double a, double b) {
    if (b == 0.0) {
        return 0.0;  /* Error: division by zero */
    }
    return a / b;
}

int mathlib_factorial(int n) {
    if (n < 0) return -1;
    if (n <= 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int mathlib_gcd(int a, int b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int mathlib_lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    return (a / mathlib_gcd(a, b)) * b;
}
