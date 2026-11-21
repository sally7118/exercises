# Embedded Linux Exercises

## 02 - Driver Registration
Character device driver registered with:
- Dynamic major allocation (`alloc_chrdev_region`)
- Statically allocated `struct cdev` (`cdev_init` + `cdev_add`)

### Build & test
    cd 02_driver_registration
    make
    sudo insmod mychardev.ko
    grep mychardev /proc/devices
    sudo rmmod mychardev
