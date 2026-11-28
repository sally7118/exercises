#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/uaccess.h>   /* copy_to_user / copy_from_user */

#define BUF_SIZE 4096   /* 4K circular buffer */

static int nparam = 5;
module_param(nparam, int, 0644);

dev_t dev;
int units = 2;

/*
 * Private data of the driver.
 * cdev is embedded in the structure, so in open() we can get back
 * to the whole structure from inode->i_cdev using container_of

 */
static struct mod_priv {
  struct proc_dir_entry *mod_proc;
  struct cdev mod_cdev;
  struct class *mod_cl;
  struct device *mod_dev;
  /* statistics */
  int open_cnt;        /* how many times the device was opened */
  int close_cnt;      /* how many times the device was closed */
  /* circular buffer */
  char buf[BUF_SIZE]; /* the memory buffer itself                       */
  int head;           /* write index - next place we have not written   */
  int tail;           /* read index  - next place we have not read yet  */
  int used;           /* bytes written and not read yet (0..BUF_SIZE)    */
} mod_priv;


static int mod_open (struct inode *ino, struct file *filp)
{
  struct mod_priv *priv;

  /* 1. Setting of private data */
  priv = container_of (ino->i_cdev, struct mod_priv, mod_cdev);
  filp->private_data = priv;

  /* 2. Update of statistics */
  priv->open_cnt++;

  printk (KERN_INFO "triv_mod: open  (minor %d) - opens: %d, closes: %d\n",
          iminor (ino),
          priv->open_cnt,
          priv->close_cnt);

  return 0;
}

static int mod_release (struct inode *ino, struct file *filp)
{
  /* private_data was set in open - no need for container_of again */
  struct mod_priv *priv = filp->private_data;

  /* Update of statistics */
  priv->close_cnt++;

  printk (KERN_INFO "triv_mod: close (minor %d) - opens: %d, closes: %d\n",
          iminor (ino),
          priv->open_cnt,
          priv->close_cnt);

  return 0;
}

/* ---------------------------------------------------------------- */
/* read entry point - reads from tail  */
/* ---------------------------------------------------------------- */
static ssize_t mod_read (struct file *filp, char __user *ubuf,
                         size_t count, loff_t *offp)
{
  struct mod_priv *priv = filp->private_data;
  size_t first, second;

  /* nothing to read -> end of file */
  if (priv->used == 0)
    return 0;

  /* we can not read more than what was written */
  if (count > priv->used)
    count = priv->used;

  /* from tail up to the end of the buffer (at most) */
  first = min (count, (size_t)(BUF_SIZE - priv->tail));
  /*  what is left - from the start of the buffer (wrap around) */
  second = count - first;

  if (copy_to_user (ubuf, priv->buf + priv->tail, first))
    return -EFAULT;
  if (second && copy_to_user (ubuf + first, priv->buf, second))
    return -EFAULT;

  priv->tail = (priv->tail + count) % BUF_SIZE;   /* crossed the end -> back to 0 */
  priv->used -= count;
  *offp += count;

  printk (KERN_INFO "triv_mod: read  %zu bytes (head %d, tail %d, used %d)\n",
          count, priv->head, priv->tail, priv->used);

  return count;
}

/* ---------------------------------------------------------------- */
/* write entry point - writes at head, head advances                */
/* A write that goes past the end of the buffer -> -EFAULT          */
/* ---------------------------------------------------------------- */
static ssize_t mod_write (struct file *filp, const char __user *ubuf,
                          size_t count, loff_t *offp)
{
  struct mod_priv *priv = filp->private_data;

  /* writing past the end of the buffer is an error */
  if (priv->head + count > BUF_SIZE) {
    printk (KERN_INFO "triv_mod: write %zu bytes at head %d - past end of buffer\n",
            count, priv->head);
    return -EFAULT;
  }

  /* do not overwrite data that was not read yet */
  if (priv->used + count > BUF_SIZE)
    return -ENOSPC;

  if (copy_from_user (priv->buf + priv->head, ubuf, count))
    return -EFAULT;

  /* reached exactly the end -> next write starts at 0 */
  priv->head = (priv->head + count) % BUF_SIZE;
  priv->used += count;
  *offp += count;

  printk (KERN_INFO "triv_mod: write %zu bytes (head %d, tail %d, used %d)\n",
          count, priv->head, priv->tail, priv->used);

  return count;
}

static const struct file_operations fops = {
  .owner   = THIS_MODULE,
  .open    = mod_open,
  .release = mod_release,
  .read    = mod_read,
  .write   = mod_write,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
static int mod_read_proc (char *page, char **start, off_t off,
                                int count, int *eof, void *data) {
  unsigned char *content, *c;
  int len = 0;
  off_t begin = 0;

  content = (char *)(page + off);
  len = sprintf (content, "param is %d\nopens: %d\ncloses: %d\n", nparam,
                 mod_priv.open_cnt,
                 mod_priv.close_cnt);
  c = (char *)content+len;
  if (off > begin + len)
    return 0;
  *start = page + (off - begin);
  *eof = 1;

  return (count < begin + len) ? count : begin + len - off;
}
#endif

static int __init trivmod_init (void)
{
  int err;

  printk (KERN_INFO "triv_mod loaded\n");

  /* reset statistics */
  mod_priv.open_cnt = 0;
  mod_priv.close_cnt = 0;
  /* empty circular buffer */
  mod_priv.head = 0;
  mod_priv.tail = 0;
  mod_priv.used = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  mod_priv.mod_proc = create_proc_entry ("driver/mod_proc", 0, 0);
  if (!mod_priv.mod_proc)
    return -EFAULT;
  mod_priv.mod_proc->read_proc = mod_read_proc;
#endif
  err = alloc_chrdev_region (&dev, 0, units, "CourseDev");
  if (err < 0)
    goto dvr_err;
  cdev_init (&mod_priv.mod_cdev, &fops);
  mod_priv.mod_cdev.owner = THIS_MODULE;
  err = cdev_add (&mod_priv.mod_cdev, dev, units);
  if (err < 0)
    goto cdev_err;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0))
  mod_priv.mod_cl = class_create (THIS_MODULE, "CourseDev");
#else
  mod_priv.mod_cl = class_create ("CourseDev");  /* owner arg removed in 6.4 */
#endif
  if (IS_ERR_OR_NULL (mod_priv.mod_cl)) {
    err = mod_priv.mod_cl ? PTR_ERR (mod_priv.mod_cl) : -ENOMEM;
    goto clc_err;
  }
  mod_priv.mod_dev = device_create (mod_priv.mod_cl, NULL, dev, NULL, "myDev");
  if (IS_ERR_OR_NULL (mod_priv.mod_dev)) {
    err = mod_priv.mod_dev ? PTR_ERR (mod_priv.mod_dev) : -ENOMEM;
    goto dvc_err;
  }

  return 0;
dvc_err:
  class_destroy (mod_priv.mod_cl);
clc_err:
  cdev_del (&mod_priv.mod_cdev);
cdev_err:
  unregister_chrdev_region (dev, units);
dvr_err:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  remove_proc_entry ("driver/mod_proc", 0);
#endif

  return err;
}

static void __exit trivmod_exit (void)
{
  device_destroy (mod_priv.mod_cl, dev);
  class_destroy (mod_priv.mod_cl);
  cdev_del (&mod_priv.mod_cdev);
  unregister_chrdev_region (dev, units);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  remove_proc_entry ("driver/mod_proc", 0);
#endif
  printk (KERN_INFO "triv_mod unloaded - total opens: %d, total closes: %d\n",
          mod_priv.open_cnt,
          mod_priv.close_cnt);
  return;
}

MODULE_LICENSE("GPL");
MODULE_VERSION("0.3");
MODULE_AUTHOR("Instructor");
MODULE_PARM_DESC(nparam, "A numeric demonstration parameter");
MODULE_DESCRIPTION("A trivial exercise module - open/release/read/write with a circular buffer");

module_init(trivmod_init);
module_exit(trivmod_exit);
