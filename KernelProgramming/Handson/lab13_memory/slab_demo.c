/*
 * slab_demo.c — Slab Allocator (Custom Cache) Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#define NUM_OBJECTS 100

struct my_object {
    int id;
    char name[28];
    unsigned long timestamp;
};

static struct kmem_cache *my_cache;
static struct my_object *objects[NUM_OBJECTS];

static int __init slab_demo_init(void)
{
    int i;

    pr_info("=== Slab Allocator Demo ===\n");

    /* Create a custom slab cache */
    my_cache = kmem_cache_create("my_cache",
                                 sizeof(struct my_object),
                                 0,         /* alignment */
                                 SLAB_HWCACHE_ALIGN,
                                 NULL);     /* constructor */
    if (!my_cache) {
        pr_err("Failed to create slab cache\n");
        return -ENOMEM;
    }

    pr_info("Created slab cache: object_size=%zu\n", sizeof(struct my_object));

    /* Allocate objects from the cache */
    for (i = 0; i < NUM_OBJECTS; i++) {
        objects[i] = kmem_cache_alloc(my_cache, GFP_KERNEL);
        if (!objects[i]) {
            pr_err("Allocation %d failed\n", i);
            break;
        }
        objects[i]->id = i;
        snprintf(objects[i]->name, sizeof(objects[i]->name), "obj_%03d", i);
        objects[i]->timestamp = jiffies;
    }

    pr_info("Allocated %d objects from slab cache\n", i);
    pr_info("First: id=%d name=%s\n", objects[0]->id, objects[0]->name);
    pr_info("Last:  id=%d name=%s\n", objects[i-1]->id, objects[i-1]->name);

    /* Free half */
    for (i = 0; i < NUM_OBJECTS / 2; i++) {
        kmem_cache_free(my_cache, objects[i]);
        objects[i] = NULL;
    }
    pr_info("Freed %d objects (cache retains slabs for reuse)\n", NUM_OBJECTS / 2);

    /* Reallocate — should be fast (reuses freed slabs) */
    for (i = 0; i < NUM_OBJECTS / 2; i++) {
        objects[i] = kmem_cache_alloc(my_cache, GFP_KERNEL);
        objects[i]->id = i + 1000;
    }
    pr_info("Re-allocated %d objects (fast — slab reuse)\n", NUM_OBJECTS / 2);

    pr_info("=== Slab Demo Complete ===\n");
    return 0;
}

static void __exit slab_demo_exit(void)
{
    int i;

    for (i = 0; i < NUM_OBJECTS; i++) {
        if (objects[i])
            kmem_cache_free(my_cache, objects[i]);
    }

    kmem_cache_destroy(my_cache);
    pr_info("slab_demo unloaded — cache destroyed\n");
}

module_init(slab_demo_init);
module_exit(slab_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Slab Allocator Demo");
