#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>      /* dev_t, alloc_chrdev_region, file_operations */
#include <linux/cdev.h>    /* struct cdev, cdev_init, cdev_add, cdev_del */

#define DEV_NAME   "mychardev"   /* זה השם שיופיע ב-/proc/devices */
#define DEV_COUNT  1

MODULE_LICENSE("GPL");

static dev_t dev;                 
static struct cdev my_cdev;       


static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
};

static int __init my_mod_init(void)
{
    int ret;

    /* 1. הקצאה דינמית של מספר התקן: minor ראשון = 0, כמות = 1 */
    ret = alloc_chrdev_region(&dev, 0, DEV_COUNT, DEV_NAME);
    if (ret < 0) {
        pr_err("%s: alloc_chrdev_region failed (%d)\n", DEV_NAME, ret);
        return ret;
    }
    pr_info("%s: allocated major=%d minor=%d\n",
            DEV_NAME, MAJOR(dev), MINOR(dev));

    /* 2. אתחול ה-cdev הסטטי וקישור ל-file_operations */
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    /* 3. רישום ב-kernel */
    ret = cdev_add(&my_cdev, dev, DEV_COUNT);
    if (ret < 0) {
        pr_err("%s: cdev_add failed (%d)\n", DEV_NAME, ret);
        unregister_chrdev_region(dev, DEV_COUNT);  /* ניקוי אחרי כישלון */
        return ret;
    }
    return 0;
}

static void __exit my_mod_exit(void)
{
    /* סדר הפוך מה-init */
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, DEV_COUNT);
    pr_info("%s: unregistered\n", DEV_NAME);
}

module_init(my_mod_init);
module_exit(my_mod_exit);
