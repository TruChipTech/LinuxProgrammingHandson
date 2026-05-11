/*
 * elf_patch_target.c — Target binary for ELF patching exercise
 *
 * Build: gcc -g elf_patch_target.c -o patch_me
 * 
 * Exercise: Patch the binary to change "Access Denied" to "Access Granted"
 * and/or bypass the authentication check.
 */

#include <stdio.h>
#include <string.h>

/* This string is the patching target — find it with: strings -t x patch_me */
static const char *auth_message_fail = "Access Denied";
static const char *auth_message_pass = "Access Allowed";

int check_authentication(const char *password) {
    /* Simple check — target for binary patching */
    if (strcmp(password, "s3cur3_p4ss") == 0) {
        return 1;  /* Authenticated */
    }
    return 0;  /* Failed */
}

int main(void) {
    const char *test_password = "wrong_password";

    printf("=== ELF Patch Target ===\n");
    printf("Checking password: '%s'\n", test_password);

    if (check_authentication(test_password)) {
        printf("Result: %s\n", auth_message_pass);
        printf("Welcome! Secret data: 42\n");
    } else {
        printf("Result: %s\n", auth_message_fail);
        printf("Hint: Try patching the binary!\n");
    }

    printf("=== Done ===\n");
    return 0;
}
