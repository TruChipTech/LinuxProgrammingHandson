/*
 * target_program.c — Target for ptrace memory inspection
 *
 * Build: gcc -Wall -g target_program.c -o target_prog
 * Run:   ./target_prog
 *
 * It prints its PID and the address of a secret variable,
 * then waits so you can attach with memspy.
 */

#include <stdio.h>
#include <unistd.h>

int main(void) {
    int secret_value = 0x41424344;   /* "ABCD" in hex */
    char secret_string[] = "HIDDEN_SECRET_DATA_12345";

    printf("=== Ptrace Target Program ===\n");
    printf("PID:            %d\n", getpid());
    printf("secret_value:   addr=%p  value=0x%X\n",
           (void *)&secret_value, secret_value);
    printf("secret_string:  addr=%p  value=\"%s\"\n",
           (void *)secret_string, secret_string);
    printf("\nWaiting... Use memspy to read my memory.\n");
    printf("Example: ./memspy %d %p\n", getpid(), (void *)&secret_value);
    printf("Press Ctrl+C to exit.\n\n");

    while (1) {
        sleep(5);
        printf("  Still alive (PID %d, secret=0x%X, str='%s')\n",
               getpid(), secret_value, secret_string);
    }

    return 0;
}
