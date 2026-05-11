/*
 * stack_overflow.c — Stack buffer overflow demo for ASan
 *
 * Build: gcc -g -fsanitize=address -fno-omit-frame-pointer -o stack_overflow stack_overflow.c
 */

#include <stdio.h>
#include <string.h>

void vulnerable_function(const char *input)
{
    char buffer[16];
    int secret = 0xDEADBEEF;

    printf("Before copy: secret = 0x%X\n", secret);
    printf("Input length: %zu, buffer size: %zu\n", strlen(input), sizeof(buffer));

    /* BUG: strcpy doesn't check bounds — overflows into 'secret' and beyond */
    strcpy(buffer, input);

    printf("After copy: buffer = '%s'\n", buffer);
    printf("After copy: secret = 0x%X\n", secret);

    if (secret != (int)0xDEADBEEF)
        printf("!!! SECRET WAS CORRUPTED by overflow !!!\n");
}

int main(void)
{
    printf("=== Stack Buffer Overflow Demo ===\n\n");

    printf("[1] Safe input (fits in buffer):\n");
    vulnerable_function("Hello");
    printf("\n");

    printf("[2] Overflow input (too large for buffer):\n");
    vulnerable_function("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

    return 0;
}
