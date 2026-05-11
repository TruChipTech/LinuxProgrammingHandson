/*
 * ptrace_memspy.c — Read/write memory of another process via ptrace
 *
 * Build: gcc -Wall -g ptrace_memspy.c -o memspy
 * Usage: ./memspy <PID> <HEX_ADDRESS>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <PID> <HEX_ADDRESS>\n", argv[0]);
        fprintf(stderr, "Example: %s 1234 0x7ffd12345678\n", argv[0]);
        return 1;
    }

    pid_t target_pid = atoi(argv[1]);
    unsigned long addr = strtoul(argv[2], NULL, 16);

    printf("[memspy] Attaching to PID %d\n", target_pid);

    /* Attach to the target process */
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) < 0) {
        perror("PTRACE_ATTACH");
        return 1;
    }

    /* Wait for the target to stop */
    int status;
    waitpid(target_pid, &status, 0);
    printf("[memspy] Attached. Target stopped.\n");

    /* Read memory at the specified address */
    printf("[memspy] Reading memory at 0x%lx:\n", addr);
    printf("  Offset    Hex                                      ASCII\n");
    printf("  --------  ---------------------------------------  ----------------\n");

    for (int line = 0; line < 4; line++) {
        unsigned long line_addr = addr + line * 16;
        printf("  %08lx  ", line_addr);

        char ascii[17];
        memset(ascii, '.', 16);
        ascii[16] = '\0';

        for (int i = 0; i < 16; i += sizeof(long)) {
            errno = 0;
            long word = ptrace(PTRACE_PEEKDATA, target_pid,
                              (void *)(line_addr + i), NULL);
            if (errno != 0) {
                printf("?? ?? ?? ?? ?? ?? ?? ?? ");
                break;
            }

            unsigned char *bytes = (unsigned char *)&word;
            for (int b = 0; b < (int)sizeof(long) && (i + b) < 16; b++) {
                printf("%02x ", bytes[b]);
                if (bytes[b] >= 32 && bytes[b] < 127) {
                    ascii[i + b] = bytes[b];
                }
            }
        }
        printf(" %s\n", ascii);
    }

    /*
     * TODO Exercise: Implement PTRACE_POKEDATA to modify the target's memory.
     *
     * Example: Change the value at 'addr' to 0xDEADBEEF:
     *   ptrace(PTRACE_POKEDATA, target_pid, (void *)addr, (void *)0xDEADBEEF);
     *
     * Be careful: modifying arbitrary memory can crash the target!
     */

    /* Detach from the target */
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
    printf("\n[memspy] Detached from PID %d\n", target_pid);

    return 0;
}
