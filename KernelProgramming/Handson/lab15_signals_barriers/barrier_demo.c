/*
 * barrier_demo.c — Memory Barrier Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>

/*
 * Demonstrates compiler and CPU memory barriers.
 *
 * Without barriers, CPU or compiler may reorder:
 *   flag = 1;
 *   message = 42;
 * to:
 *   message = 42;
 *   flag = 1;
 *
 * Reader might see flag=1 but stale message on SMP systems.
 */

static int message = 0;
static int flag = 0;

static struct task_struct *writer_thread;
static struct task_struct *reader_thread;

static int writer_fn(void *data)
{
    msleep(500);  /* Let reader start first */

    pr_info("[barrier-writer] Setting message=42\n");
    message = 42;

    /*
     * smp_wmb() — write memory barrier
     * Ensures 'message' is visible before 'flag' to other CPUs.
     */
    smp_wmb();

    pr_info("[barrier-writer] Setting flag=1\n");
    flag = 1;

    while (!kthread_should_stop())
        msleep(100);
    return 0;
}

static int reader_fn(void *data)
{
    int loops = 0;

    pr_info("[barrier-reader] Waiting for flag...\n");

    while (flag == 0 && loops < 50) {
        /*
         * smp_rmb() — read memory barrier
         * Ensures we see the latest 'flag' value.
         * Also prevents speculative reads of 'message' before 'flag'.
         */
        smp_rmb();
        msleep(100);
        loops++;
    }

    if (flag == 1) {
        smp_rmb();  /* Ensure message is read after flag */
        pr_info("[barrier-reader] flag=%d, message=%d (should be 42)\n",
                flag, message);
    } else {
        pr_info("[barrier-reader] Timed out waiting for flag\n");
    }

    while (!kthread_should_stop())
        msleep(100);
    return 0;
}

static int __init barrier_demo_init(void)
{
    pr_info("=== Memory Barrier Demo ===\n");
    pr_info("Barrier types:\n");
    pr_info("  barrier()    — compiler-only barrier\n");
    pr_info("  smp_mb()     — full memory barrier (read+write)\n");
    pr_info("  smp_rmb()    — read memory barrier\n");
    pr_info("  smp_wmb()    — write memory barrier\n");
    pr_info("  READ_ONCE()  — single-access compiler barrier\n");
    pr_info("  WRITE_ONCE() — single-access compiler barrier\n");

    reader_thread = kthread_run(reader_fn, NULL, "barrier_reader");
    if (IS_ERR(reader_thread))
        return PTR_ERR(reader_thread);

    writer_thread = kthread_run(writer_fn, NULL, "barrier_writer");
    if (IS_ERR(writer_thread)) {
        kthread_stop(reader_thread);
        return PTR_ERR(writer_thread);
    }

    return 0;
}

static void __exit barrier_demo_exit(void)
{
    kthread_stop(writer_thread);
    kthread_stop(reader_thread);
    pr_info("[barrier_demo] Module unloaded\n");
}

module_init(barrier_demo_init);
module_exit(barrier_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Memory Barrier Demo");
