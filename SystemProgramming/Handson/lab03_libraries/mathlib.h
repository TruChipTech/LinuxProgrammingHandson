/*
 * mathlib.h — Math Utilities Library Interface
 *
 * Used for both static (.a) and shared (.so) library exercises.
 */

#ifndef MATHLIB_H
#define MATHLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Basic operations (mathlib_basic.c) ---- */

int     mathlib_add(int a, int b);
int     mathlib_subtract(int a, int b);
int     mathlib_multiply(int a, int b);
double  mathlib_divide(double a, double b);
int     mathlib_factorial(int n);
int     mathlib_gcd(int a, int b);
int     mathlib_lcm(int a, int b);

/* ---- Advanced operations (mathlib_advanced.c) ---- */

double  mathlib_power(double base, int exp);
double  mathlib_sqrt_newton(double x);
double  mathlib_pi_leibniz(int terms);
int     mathlib_is_prime(int n);
void    mathlib_fibonacci(int *buf, int count);
double  mathlib_mean(const double *data, int n);
double  mathlib_stddev(const double *data, int n);

/* ---- Library info ---- */

const char *mathlib_version(void);

#ifdef __cplusplus
}
#endif

#endif /* MATHLIB_H */
