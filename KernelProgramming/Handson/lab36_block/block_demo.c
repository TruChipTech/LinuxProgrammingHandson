/*
 * block_demo.c — RAM-backed Block Device using blk-mq
 *
 * Creates a 4 MB RAM disk (/dev/blkdemo0).
 * Format with: mkfs.ext4 /dev/blkdemo0
 * Mount with:  mount /dev/blkdemo0 /mnt
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/vmalloc.h>
#include <linux/hdreg.h>

#define BLKDEMO_NAME     "blkdemo"
#define BLKDEMO_MINORS   1
#define SECTOR_SIZE      512
#define NSECTORS         8192       /* 4 MB = 8192 * 512 */

struct blkdemo_dev {
    u8 *data;                       /* vmalloc'd backing store */
    struct gendisk *gd;
    struct blk_mq_tag_set tag_set;
};

static struct blkdemo_dev *dev_data;

static blk_status_t blkdemo_queue_rq(struct blk_mq_hw_ctx *hctx,
                                      const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct blkdemo_dev *bdev = rq->q->queuedata;
    struct bio_vec bvec;
    struct req_iterator iter;
    sector_t sector = blk_rq_pos(rq);
    void *buf;
    size_t offset;

    blk_mq_start_request(rq);

    rq_for_each_segment(bvec, rq, iter) {
        offset = sector * SECTOR_SIZE;
        buf = kmap_local_page(bvec.bv_page) + bvec.bv_offset;

        if (offset + bvec.bv_len > (size_t)NSECTORS * SECTOR_SIZE) {
            kunmap_local(buf);
            blk_mq_end_request(rq, BLK_STS_IOERR);
            return BLK_STS_IOERR;
        }

        if (rq_data_dir(rq) == WRITE)
            memcpy(bdev->data + offset, buf, bvec.bv_len);
        else
            memcpy(buf, bdev->data + offset, bvec.bv_len);

        kunmap_local(buf);
        sector += bvec.bv_len / SECTOR_SIZE;
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops blkdemo_mq_ops = {
    .queue_rq = blkdemo_queue_rq,
};

static const struct block_device_operations blkdemo_fops = {
    .owner = THIS_MODULE,
};

static int __init blkdemo_init(void)
{
    int ret;

    dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data)
        return -ENOMEM;

    dev_data->data = vmalloc(NSECTORS * SECTOR_SIZE);
    if (!dev_data->data) {
        ret = -ENOMEM;
        goto err_free_dev;
    }
    memset(dev_data->data, 0, NSECTORS * SECTOR_SIZE);

    /* Setup tag set for blk-mq */
    dev_data->tag_set.ops = &blkdemo_mq_ops;
    dev_data->tag_set.nr_hw_queues = 1;
    dev_data->tag_set.queue_depth = 128;
    dev_data->tag_set.numa_node = NUMA_NO_NODE;
    dev_data->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;

    ret = blk_mq_alloc_tag_set(&dev_data->tag_set);
    if (ret)
        goto err_free_data;

    /* Allocate gendisk */
    dev_data->gd = blk_mq_alloc_disk(&dev_data->tag_set, NULL, dev_data);
    if (IS_ERR(dev_data->gd)) {
        ret = PTR_ERR(dev_data->gd);
        goto err_free_tag;
    }

    dev_data->gd->major = 0;  /* Dynamic major */
    dev_data->gd->first_minor = 0;
    dev_data->gd->minors = BLKDEMO_MINORS;
    dev_data->gd->fops = &blkdemo_fops;
    snprintf(dev_data->gd->disk_name, DISK_NAME_LEN, "%s0", BLKDEMO_NAME);
    set_capacity(dev_data->gd, NSECTORS);

    dev_data->gd->queue->queuedata = dev_data;

    ret = add_disk(dev_data->gd);
    if (ret)
        goto err_put_disk;

    pr_info("[blkdemo] Created /dev/%s (4 MB RAM disk)\n",
            dev_data->gd->disk_name);
    return 0;

err_put_disk:
    put_disk(dev_data->gd);
err_free_tag:
    blk_mq_free_tag_set(&dev_data->tag_set);
err_free_data:
    vfree(dev_data->data);
err_free_dev:
    kfree(dev_data);
    return ret;
}

static void __exit blkdemo_exit(void)
{
    del_gendisk(dev_data->gd);
    put_disk(dev_data->gd);
    blk_mq_free_tag_set(&dev_data->tag_set);
    vfree(dev_data->data);
    kfree(dev_data);
    pr_info("[blkdemo] Removed\n");
}

module_init(blkdemo_init);
module_exit(blkdemo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab Exercise");
MODULE_DESCRIPTION("RAM-backed Block Device using blk-mq");
