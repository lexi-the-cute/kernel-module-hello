// Tutorials
// https://www.geeksforgeeks.org/linux-kernel-module-programming-hello-world-program/
// https://tldp.org/LDP/lkmpg/2.6/html/x569.html
// https://tldp.org/LDP/khg/HyperNews/get/devices/basics.html
// https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html

// Documentation
// https://elixir.bootlin.com/linux/v3.4/source/drivers/base/class.c#L249
// https://www.kernel.org/doc/html/v4.12/driver-api/infrastructure.html
// https://www.kernel.org/doc/html/latest/core-api/mm-api.html

// Explanations
// https://unix.stackexchange.com/questions/550037/how-does-udev-uevent-work#551047
// https://www.kernel.org/doc/html/v5.4/driver-api/driver-model/class.html
// https://unix.stackexchange.com/questions/674962/why-are-copy-from-user-and-copy-to-user-needed-when-the-kernel-is-mappe
// https://stackoverflow.com/a/33996607

// Inspiration
// https://github.com/valadaptive/uwurandom


#include <linux/module.h>   /* Needed by all modules */
#include <linux/kernel.h>   /* Needed for KERN_INFO */
#include <linux/init.h>     /* Needed for the macros */
#include <linux/device.h>   /* Needed to create/destroy classes/devices */
#include <linux/fs.h>       /* Needed for all file operations */
#include <linux/slab.h>     /* Needed for kmalloc */

#include "hello.h"


#define MODULE_NAME "Hello World"   /* The Kernel Module's Name */
#define DEVICE_NAME "hello"   /* The Name Of The Default Character Device (Used in 2 Places) */

// These are required for the kernel module to load
MODULE_LICENSE("GPL");  /* Modules have to be GPL compatible to keep the kernel from being tainted */
MODULE_AUTHOR("Alexis' Art");
MODULE_DESCRIPTION("A sample kernel module");
MODULE_VERSION("0.1");

static int major;   /* Major number assigned to character device by system */
static int minor;
static struct class* device_class; /* Used for defining a type of device */
static dev_t device_number;   /* Datatype containing a major and minor number */

// This is used to tell the kernel how the driver will be accessed
static struct file_operations fops = {
    owner: THIS_MODULE,
    read: device_read,  /* Called when attempting to read an open device */
    write: device_write,  /* Called when attempting to write to an open device */
    open: device_open,  /* Called when trying to open a device */
    release: device_release  /* Called when trying to close a device */
};

// Called when attempting to read from an open device (e.g. cat /dev/mydevice)
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t * offset) {
    char message[] = "Hello World!\n";
    
    // Copy message[] from kernel space to user space
    if (copy_to_user(buffer, &message, sizeof message))
        return -EFAULT;

    // printk(KERN_INFO "%s: Message (%ld): %s\n", MODULE_NAME, sizeof message, message);  /* \n is duplicate since it already exists in message[] */

    // Return size of message
    return sizeof message;
}

// Called when attempting to write to an open device (e.g. echo "hi" > /dev/mydevice)
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t * offset) {
    /* 
     * kmalloc allocates physically continuous space, vmalloc allocates virtually continuous space.
     * vmalloc is recommended for when you need a large amount of buffer
     * kmalloc is recommended for when you only need less than a page of buffer
     * kmalloc is required when passing to/from hardware
     * 
     * GFP_KERNEL is Get Free Page Kernel
     */
    char *input = kzalloc(length, GFP_KERNEL);  /* For me, PAGE_SIZE is 4096 */
    // char input[length];  

    // Copy *buffer from user space to kernel space
    if (copy_from_user(input, buffer, length))
        return -EFAULT;

    // Forcibly null-terminate buffer
    input[length] = 0;

    printk(KERN_INFO "%s: Input (%ld): %s\n", MODULE_NAME, length, input);

    // Free Allocated Memory
    kfree(input);

    // Return size of input
    return length;
}

// Called when opening a device
static int device_open(struct inode *inode, struct file *file) {
    return 0;
}

// Called when closing a device
static int device_release(struct inode *inode, struct file *file) {
	return 0;
}

// Process event for device
static int dev_uevent(struct device* dev, struct kobj_uevent_env* env) {
    int result = add_uevent_var(env, "DEVMODE=%#o", 0666);

    if (result < 0)
        return result;

    return 0;
}

// This is the start function
static int __init initialize(void) {
    struct device* device_file;

    printk(KERN_INFO "Loading %s module...\n", MODULE_NAME);

    // Register Character Device, 0 Means Assign Major Number For Me
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register character device, %s, for module, %s. Error code is %d\n", DEVICE_NAME, MODULE_NAME, major);
        return major;
    }

    // ---

    minor = 0;  /* Minor is used internally to differentiate between devices, major is the one that'll let us see the device */
    device_number = MKDEV(major, minor);  /* Datatype containing a character device major and minor number */

    // Create a struct class pointer for device_create calls
    device_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(device_class)) {
        printk(KERN_ALERT "Failed to create class pointer, %s, for module, %s. Error code is %ld\n", DEVICE_NAME, MODULE_NAME, PTR_ERR(device_class));
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(device_class);
    }

    // Called when device is added/removed from this class
    device_class->dev_uevent = dev_uevent;

    // ---

    // Create Default Character Device File
    device_file = device_create(device_class, NULL, device_number, NULL, DEVICE_NAME);
    if (IS_ERR(device_file)) {
        class_destroy(device_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(device_file);
    }

    return 0;
}

// This is the cleanup function
static void __exit cleanup(void) {
    printk(KERN_INFO "Unloading %s module...\n", MODULE_NAME);

    device_destroy(device_class, device_number);
    class_destroy(device_class);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(initialize);
module_exit(cleanup);