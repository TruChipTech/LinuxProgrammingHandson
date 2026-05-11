/*
 * mathlib_advanced.c — Advanced Math Utility Functions
 */

#include "mathlib.h"
#include <math.h>

double mathlib_power(double base, int exp) {
    if (exp < 0) {
        base = 1.0 / base;
        exp = -exp;
    }
    double result = 1.0;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

double mathlib_sqrt_newton(double x) {
    if (x < 0) return -1.0;
    if (x == 0) return 0.0;
    double guess = x / 2.0;
    for (int i = 0; i < 50; i++) {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

double mathlib_pi_leibniz(int terms) {
    double pi = 0.0;
    for (int i = 0; i < terms; i++) {
        double term = 1.0 / (2.0 * i + 1.0);
        if (i % 2 == 0)
            pi += term;
        else
            pi -= term;
    }
    return pi * 4.0;
}

int mathlib_is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return 0;
    }
    return 1;
}

void mathlib_fibonacci(int *buf, int count) {
    if (count <= 0) return;
    buf[0] = 0;
    if (count == 1) return;
    buf[1] = 1;
    for (int i = 2; i < count; i++) {
        buf[i] = buf[i - 1] + buf[i - 2];
    }
}

double mathlib_mean(const double *data, int n) {
    if (n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += data[i];
    }
    return sum / n;
}

double mathlib_stddev(const double *data, int n) {
    if (n <= 1) return 0.0;
    double avg = mathlib_mean(data, n);
    double sum_sq = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = data[i] - avg;
        sum_sq += diff * diff;
    }
    return sqrt(sum_sq / (n - 1));
}
