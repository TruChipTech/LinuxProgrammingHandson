/*
 * klist_demo.c — Kernel Linked List Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>

struct city {
    char name[32];
    int population;
    struct list_head list;
};

static LIST_HEAD(city_list);

static void add_city(const char *name, int population)
{
    struct city *c = kmalloc(sizeof(*c), GFP_KERNEL);
    if (!c) return;
    strscpy(c->name, name, sizeof(c->name));
    c->population = population;
    list_add_tail(&c->list, &city_list);
}

static void print_cities(void)
{
    struct city *c;
    int i = 0;

    pr_info("--- City List ---\n");
    list_for_each_entry(c, &city_list, list) {
        pr_info("  [%d] %s (pop: %d)\n", i++, c->name, c->population);
    }
    pr_info("--- Total: %d cities ---\n", i);
}

static void delete_city(const char *name)
{
    struct city *c, *tmp;

    list_for_each_entry_safe(c, tmp, &city_list, list) {
        if (strcmp(c->name, name) == 0) {
            pr_info("Deleting city: %s\n", c->name);
            list_del(&c->list);
            kfree(c);
            return;
        }
    }
    pr_info("City not found: %s\n", name);
}

static void free_all_cities(void)
{
    struct city *c, *tmp;

    list_for_each_entry_safe(c, tmp, &city_list, list) {
        list_del(&c->list);
        kfree(c);
    }
}

static int __init klist_demo_init(void)
{
    pr_info("=== Kernel Linked List Demo ===\n");

    /* Add cities */
    add_city("Bangalore", 12000000);
    add_city("Mumbai", 20000000);
    add_city("Delhi", 19000000);
    add_city("Chennai", 10000000);
    add_city("Hyderabad", 10500000);

    print_cities();

    /* Check empty */
    pr_info("List empty? %s\n", list_empty(&city_list) ? "YES" : "NO");

    /* Delete one */
    delete_city("Delhi");
    print_cities();

    /* Count using list_for_each */
    {
        struct list_head *pos;
        int count = 0;
        list_for_each(pos, &city_list)
            count++;
        pr_info("Count via list_for_each: %d\n", count);
    }

    pr_info("=== List Demo Complete ===\n");
    return 0;
}

static void __exit klist_demo_exit(void)
{
    free_all_cities();
    pr_info("klist_demo unloaded\n");
}

module_init(klist_demo_init);
module_exit(klist_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Linked List Demo");
