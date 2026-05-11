/*
 * hashtable_demo.c — Kernel Hash Table Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/hashtable.h>
#include <linux/slab.h>

#define MY_HASH_BITS 4  /* 2^4 = 16 buckets */

struct student {
    int id;
    char name[32];
    int grade;
    struct hlist_node hash_node;
};

static DEFINE_HASHTABLE(student_table, MY_HASH_BITS);

static void add_student(int id, const char *name, int grade)
{
    struct student *s = kmalloc(sizeof(*s), GFP_KERNEL);
    if (!s) return;

    s->id = id;
    strscpy(s->name, name, sizeof(s->name));
    s->grade = grade;

    hash_add(student_table, &s->hash_node, id);
    pr_info("  Added: id=%d name=%s grade=%d\n", id, name, grade);
}

static struct student *find_student(int id)
{
    struct student *s;

    hash_for_each_possible(student_table, s, hash_node, id) {
        if (s->id == id)
            return s;
    }
    return NULL;
}

static void remove_student(int id)
{
    struct student *s = find_student(id);
    if (s) {
        pr_info("  Removing: id=%d name=%s\n", s->id, s->name);
        hash_del(&s->hash_node);
        kfree(s);
    }
}

static void print_all_students(void)
{
    struct student *s;
    int bucket;

    pr_info("--- All Students ---\n");
    hash_for_each(student_table, bucket, s, hash_node) {
        pr_info("  [bucket %d] id=%d name=%s grade=%d\n",
                bucket, s->id, s->name, s->grade);
    }
}

static void free_all_students(void)
{
    struct student *s;
    struct hlist_node *tmp;
    int bucket;

    hash_for_each_safe(student_table, bucket, tmp, s, hash_node) {
        hash_del(&s->hash_node);
        kfree(s);
    }
}

static int __init hashtable_demo_init(void)
{
    struct student *found;

    pr_info("=== Kernel Hash Table Demo ===\n");
    pr_info("Hash table: %d buckets (bits=%d)\n",
            1 << MY_HASH_BITS, MY_HASH_BITS);

    /* Add students */
    add_student(101, "Alice", 95);
    add_student(202, "Bob", 87);
    add_student(303, "Charlie", 91);
    add_student(404, "Diana", 78);
    add_student(117, "Eve", 99);  /* Same bucket as 101 (hash collision demo) */

    print_all_students();

    /* Lookup */
    found = find_student(303);
    if (found)
        pr_info("Found: id=%d name=%s grade=%d\n",
                found->id, found->name, found->grade);

    /* Remove */
    remove_student(202);
    print_all_students();

    pr_info("Empty? %s\n", hash_empty(student_table) ? "YES" : "NO");

    pr_info("=== Hash Table Demo Complete ===\n");
    return 0;
}

static void __exit hashtable_demo_exit(void)
{
    free_all_students();
    pr_info("hashtable_demo unloaded\n");
}

module_init(hashtable_demo_init);
module_exit(hashtable_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Hash Table Demo");
