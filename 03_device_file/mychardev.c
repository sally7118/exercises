#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>   /* class_create, device_create, ... */

#define DEV_NAME    "mychardev"      /* שם ב-/proc/devices וגם שם הקובץ ב-/dev */
#define CLASS_NAME  "mychar_class"   /* שם התיקייה ב-/sys/class/ */
#define DEV_COUNT   1

MODULE_LICENSE("GPL");

static dev_t dev;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
};

static int __init my_mod_init(void)
{
    int ret;

    /* --- מהתרגיל הקודם --- */
    ret = alloc_chrdev_region(&dev, 0, DEV_COUNT, DEV_NAME);
    if (ret < 0) {
        pr_err("%s: alloc_chrdev_region failed (%d)\n", DEV_NAME, ret);
        return ret;
    }

    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev, DEV_COUNT);
    if (ret < 0) {
        pr_err("%s: cdev_add failed (%d)\n", DEV_NAME, ret);
        goto err_region;
    }

    /* --- חדש: יצירת class ---  (בקרנל 6.4+ אין פרמטר THIS_MODULE) */
    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        pr_err("%s: class_create failed (%d)\n", DEV_NAME, ret);
        goto err_cdev;
    }

    /* --- חדש: יצירת device (נוצר /dev/mychardev) --- */
    my_device = device_create(my_class, NULL, dev, NULL, DEV_NAME);
    if (IS_ERR(my_device)) {
        ret = PTR_ERR(my_device);
        pr_err("%s: device_create failed (%d)\n", DEV_NAME, ret);
        goto err_class;
    }

    pr_info("%s: loaded, major=%d minor=%d, /dev/%s created\n",
            DEV_NAME, MAJOR(dev), MINOR(dev), DEV_NAME);
    return 0;

err_class:
    class_destroy(my_class);
err_cdev:
    cdev_del(&my_cdev);
err_region:
    unregister_chrdev_region(dev, DEV_COUNT);
    return ret;
}

static void __exit my_mod_exit(void)
{
    /* סדר הפוך לגמרי מה-init */
    device_destroy(my_class, dev);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, DEV_COUNT);
    pr_info("%s: unloaded\n", DEV_NAME);
}

module_init(my_mod_init);
module_exit(my_mod_exit);
