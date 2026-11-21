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


## 03 - Device File
Extends the registration driver to create the device file automatically:
- `class_create` -> `/sys/class/mychar_class`
- `device_create` -> `/dev/mychardev`
- Cleanup in reverse order with `device_destroy` / `class_destroy`

### Build & test
    cd 03_device_file
    make
    sudo insmod mychardev.ko
    ls -l /dev/mychardev
    sudo rmmod mychardev
