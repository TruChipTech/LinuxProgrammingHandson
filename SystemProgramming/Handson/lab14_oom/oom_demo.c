/*
 * oom_demo.c — OOM Killer Observation and Control
 *
 * Build: gcc -Wall -g oom_demo.c -o oom_demo
 * Run:   ./oom_demo           (safe mode — just observe)
 *        ./oom_demo --eat     (allocate until OOM — USE IN VM ONLY!)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

/* ============================================================
 * Read OOM score for a PID
 * ============================================================ */
int read_oom_score(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/oom_score", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    int score;
    if (fscanf(fp, "%d", &score) != 1) score = -1;
    fclose(fp);
    return score;
}

int read_oom_adj(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -9999;
    int adj;
    if (fscanf(fp, "%d", &adj) != 1) adj = -9999;
    fclose(fp);
    return adj;
}

/* ============================================================
 * Demo 1: Show OOM scores for current and system processes
 * ============================================================ */
void demo_show_scores(void) {
    printf("--- Demo 1: OOM Scores ---\n\n");

    printf("  Current process (PID=%d):\n", getpid());
    printf("    oom_score:     %d\n", read_oom_score(getpid()));
    printf("    oom_score_adj: %d\n", read_oom_adj(getpid()));

    /* Show overcommit settings */
    printf("\n  System overcommit settings:\n");
    FILE *fp = fopen("/proc/sys/vm/overcommit_memory", "r");
    if (fp) {
        int val;
        if (fscanf(fp, "%d", &val) == 1) {
            const char *modes[] = {"heuristic", "always allow", "never (strict)"};
            printf("    overcommit_memory: %d (%s)\n", val,
                   val <= 2 ? modes[val] : "unknown");
        }
        fclose(fp);
    }

    fp = fopen("/proc/sys/vm/overcommit_ratio", "r");
    if (fp) {
        int val;
        if (fscanf(fp, "%d", &val) == 1)
            printf("    overcommit_ratio:  %d%%\n", val);
        fclose(fp);
    }

    /* Free memory */
    printf("\n  Memory info:\n");
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), fp) && count < 8) {
            printf("    %s", line);
            count++;
        }
        fclose(fp);
    }
}

/* ============================================================
 * Demo 2: Adjust OOM score
 * ============================================================ */
void demo_adjust_score(void) {
    printf("\n--- Demo 2: Adjusting OOM Score ---\n");

    printf("  Before: oom_score=%d, oom_score_adj=%d\n",
           read_oom_score(getpid()), read_oom_adj(getpid()));

    /* Try to adjust (may need root for negative values) */
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", getpid());
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "500");
        fclose(fp);
        printf("  Set oom_score_adj to 500 (more likely to be killed)\n");
        printf("  After: oom_score=%d, oom_score_adj=%d\n",
               read_oom_score(getpid()), read_oom_adj(getpid()));

        /* Reset */
        fp = fopen(path, "w");
        if (fp) { fprintf(fp, "0"); fclose(fp); }
    } else {
        perror("  Cannot adjust oom_score_adj");
    }

    /* Try OOM-immune (needs root) */
    fp = fopen(path, "w");
    if (fp) {
        if (fprintf(fp, "-1000") > 0) {
            printf("  Set oom_score_adj to -1000 (OOM-immune) — needs root\n");
        }
        fclose(fp);
        /* Reset */
        fp = fopen(path, "w");
        if (fp) { fprintf(fp, "0"); fclose(fp); }
    }
}

/* ============================================================
 * Demo 3: Check recent OOM kills
 * ============================================================ */
void demo_check_oom_kills(void) {
    printf("\n--- Demo 3: Recent OOM Events ---\n");

    FILE *fp = popen("dmesg 2>/dev/null | grep -i 'oom\\|killed process' | tail -5", "r");
    if (fp) {
        char line[512];
        int found = 0;
        while (fgets(line, sizeof(line), fp)) {
            printf("  %s", line);
            found = 1;
        }
        pclose(fp);
        if (!found) {
            printf("  No recent OOM kills found in dmesg.\n");
        }
    }
}

/* ============================================================
 * Demo 4: Memory eater (DANGEROUS — use in VM only!)
 * ============================================================ */
void demo_eat_memory(void) {
    printf("\n--- Demo 4: Memory Eater (VM only!) ---\n");
    printf("  Allocating memory in 10MB chunks...\n");
    printf("  Press Ctrl+C to stop.\n\n");

    size_t total = 0;
    size_t chunk = 10 * 1024 * 1024;  /* 10 MB */
    int count = 0;

    while (1) {
        void *mem = malloc(chunk);
        if (!mem) {
            printf("  malloc failed after %zu MB!\n", total / (1024 * 1024));
            break;
        }
        /* Touch every page to force physical allocation */
        memset(mem, 0x42, chunk);
        total += chunk;
        count++;

        if (count % 10 == 0) {
            printf("  Allocated: %4zu MB  (oom_score=%d)\n",
                   total / (1024 * 1024), read_oom_score(getpid()));
        }
        /* Don't free — we're deliberately hoarding memory */
    }
}

int main(int argc, char *argv[]) {
    printf("=== OOM Killer Demo ===\n\n");

    demo_show_scores();
    demo_adjust_score();
    demo_check_oom_kills();

    if (argc > 1 && strcmp(argv[1], "--eat") == 0) {
        printf("\n  ⚠ WARNING: This will consume all available memory!\n");
        printf("  Only run this in a VM or container.\n");
        printf("  Starting in 3 seconds...\n");
        sleep(3);
        demo_eat_memory();
    } else {
        printf("\n  To test OOM killer, run: %s --eat (IN A VM ONLY!)\n",
               argv[0]);
    }

    printf("\n=== Demo Complete ===\n");
    return 0;
}
