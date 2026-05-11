/*
 * process_list.c — Process Management Explorer
 *
 * Walks the kernel task list, prints process info:
 *   PID, name, state, priority, scheduling policy, parent.
 * Creates a /proc/process_list entry for easy reading.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static const char *policy_name(unsigned int policy)
{
    switch (policy) {
    case SCHED_NORMAL:   return "NORMAL";
    case SCHED_FIFO:     return "FIFO";
    case SCHED_RR:       return "RR";
    case SCHED_BATCH:    return "BATCH";
    case SCHED_IDLE:     return "IDLE";
    case SCHED_DEADLINE: return "DEADLINE";
    default:             return "UNKNOWN";
    }
}

static int proc_show(struct seq_file *m, void *v)
{
    struct task_struct *task;
    int count = 0;

    seq_printf(m, "%-7s %-7s %-20s %-6s %-5s %-10s %-20s\n",
               "PID", "TGID", "COMM", "STATE", "PRIO", "POLICY", "PARENT");
    seq_puts(m, "----------------------------------------------------------------------"
                "----------\n");

    rcu_read_lock();
    for_each_process(task) {
        seq_printf(m, "%-7d %-7d %-20s %-6c %-5d %-10s %-20s\n",
                   task->pid,
                   task->tgid,
                   task->comm,
                   task_state_to_char(task),
                   task->prio,
                   policy_name(task->policy),
                   task->real_parent ? task->real_parent->comm : "(none)");
        count++;
    }
    rcu_read_unlock();

    seq_printf(m, "\nTotal processes: %d\n", count);
    return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open    = proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static struct proc_dir_entry *pentry;

static int __init process_list_init(void)
{
    pentry = proc_create("process_list", 0444, NULL, &proc_fops);
    if (!pentry)
        return -ENOMEM;

    pr_info("[process_list] /proc/process_list created\n");
    return 0;
}

static void __exit process_list_exit(void)
{
    proc_remove(pentry);
    pr_info("[process_list] Removed\n");
}

module_init(process_list_init);
module_exit(process_list_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Process List Explorer Module");
