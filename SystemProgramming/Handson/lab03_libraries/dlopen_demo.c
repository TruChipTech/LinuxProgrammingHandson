/*
 * dlopen_demo.c — Dynamic loading with dlopen/dlsym
 *
 * Build: gcc dlopen_demo.c -o dlopen_demo -ldl
 * Run:   LD_LIBRARY_PATH=. ./dlopen_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main(void) {
    printf("=== dlopen() Dynamic Loading Demo ===\n\n");

    /* Open the shared library at runtime */
    void *handle = dlopen("./libmathutil.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }
    printf("[+] Library loaded successfully\n");

    /* Clear any existing errors */
    dlerror();

    /* Look up a function symbol */
    typedef int (*add_fn)(int, int);
    add_fn add = (add_fn)dlsym(handle, "mathlib_add");
    char *err = dlerror();
    if (err) {
        fprintf(stderr, "dlsym failed: %s\n", err);
        dlclose(handle);
        return 1;
    }
    printf("[+] Found symbol: mathlib_add\n");
    printf("    mathlib_add(100, 200) = %d\n", add(100, 200));

    /* Look up another function */
    typedef int (*is_prime_fn)(int);
    is_prime_fn is_prime = (is_prime_fn)dlsym(handle, "mathlib_is_prime");
    err = dlerror();
    if (err) {
        fprintf(stderr, "dlsym failed: %s\n", err);
        dlclose(handle);
        return 1;
    }
    printf("[+] Found symbol: mathlib_is_prime\n");
    printf("    is_prime(97)  = %s\n", is_prime(97) ? "YES" : "NO");
    printf("    is_prime(100) = %s\n", is_prime(100) ? "YES" : "NO");

    /* Look up version function */
    typedef const char *(*version_fn)(void);
    version_fn version = (version_fn)dlsym(handle, "mathlib_version");
    err = dlerror();
    if (err) {
        fprintf(stderr, "dlsym failed: %s\n", err);
    } else {
        printf("[+] Library version: %s\n", version());
    }

    /* Try a symbol that doesn't exist */
    void *bad = dlsym(handle, "nonexistent_function");
    err = dlerror();
    if (err) {
        printf("[!] Expected error: %s\n", err);
    }

    /* Close the library */
    dlclose(handle);
    printf("\n[+] Library unloaded\n");
    printf("=== Demo Complete ===\n");

    return 0;
}
