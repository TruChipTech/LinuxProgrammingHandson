/*
 * kmalloc_demo.c — Kernel Memory Allocation Demo
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

static int __init kmalloc_demo_init(void)
{
    void *ptr;
    unsigned long vaddr;

    pr_info("=== Kernel Memory Allocation Demo ===\n");

    /* --- kmalloc --- */
    pr_info("--- kmalloc ---\n");
    ptr = kmalloc(256, GFP_KERNEL);
    if (ptr) {
        pr_info("  kmalloc(256): %p (phys: %pa)\n", ptr, &ptr);
        memset(ptr, 0xAA, 256);
        kfree(ptr);
        pr_info("  Freed 256 bytes\n");
    }

    /* --- kzalloc (zeroed) --- */
    pr_info("--- kzalloc ---\n");
    ptr = kzalloc(512, GFP_KERNEL);
    if (ptr) {
        unsigned char *bytes = ptr;
        pr_info("  kzalloc(512): first byte = 0x%02x (should be 0x00)\n",
                bytes[0]);
        kfree(ptr);
    }

    /* --- Large kmalloc --- */
    pr_info("--- Large kmalloc ---\n");
    ptr = kmalloc(64 * 1024, GFP_KERNEL);
    if (ptr) {
        pr_info("  kmalloc(64KB): success at %p\n", ptr);
        kfree(ptr);
    } else {
        pr_info("  kmalloc(64KB): FAILED (too large for kmalloc)\n");
    }

    /* --- vmalloc --- */
    pr_info("--- vmalloc ---\n");
    ptr = vmalloc(1024 * 1024);  /* 1 MB */
    if (ptr) {
        pr_info("  vmalloc(1MB): success at %p\n", ptr);
        memset(ptr, 0, 1024 * 1024);
        vfree(ptr);
        pr_info("  Freed 1MB vmalloc\n");
    }

    /* --- kvmalloc --- */
    pr_info("--- kvmalloc ---\n");
    ptr = kvmalloc(256 * 1024, GFP_KERNEL);
    if (ptr) {
        pr_info("  kvmalloc(256KB): success at %p\n", ptr);
        pr_info("  (kvmalloc tries kmalloc first, falls back to vmalloc)\n");
        kvfree(ptr);
    }

    /* --- Page allocation --- */
    pr_info("--- Page allocation ---\n");
    {
        struct page *pg = alloc_page(GFP_KERNEL);
        if (pg) {
            vaddr = (unsigned long)page_address(pg);
            pr_info("  alloc_page: page=%p, vaddr=0x%lx\n", pg, vaddr);
            __free_page(pg);
        }

        /* Allocate 4 contiguous pages (order=2, 2^2=4) */
        pg = alloc_pages(GFP_KERNEL, 2);
        if (pg) {
            pr_info("  alloc_pages(order=2): 4 pages at %p\n", page_address(pg));
            __free_pages(pg, 2);
        }
    }

    /* --- GFP_ATOMIC --- */
    pr_info("--- GFP_ATOMIC (no sleep) ---\n");
    ptr = kmalloc(128, GFP_ATOMIC);
    if (ptr) {
        pr_info("  GFP_ATOMIC kmalloc: success (used in interrupt context)\n");
        kfree(ptr);
    }

    pr_info("=== Memory Demo Complete ===\n");
    return 0;
}

static void __exit kmalloc_demo_exit(void)
{
    pr_info("kmalloc_demo unloaded\n");
}

module_init(kmalloc_demo_init);
module_exit(kmalloc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("Kernel Memory Allocation Demo");
