#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


#define DEVICE_NAME "uartchar"
#define CLASS_NAME "uart"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAMLB");
MODULE_DESCRIPTION("Uart Device Driver");
MODULE_VERSION("0.1");

static int majorNumber;                  ///< Stores the device number -- determined automatically
static struct class *uartClass = NULL;   ///< The device-driver class struct pointer
static struct device *uartDevice = NULL; ///< The device-driver device struct pointer

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
    {
        .open = dev_open,
        .read = dev_read,
        .write = dev_write,
        .release = dev_release,
};

static int __init uart_init(void)
{
    printk(KERN_INFO "UARTChar: Initializing the UARTChar LKM\n");

    // Try to dynamically allocate a major number for the device -- more difficult but worth it
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "UARTChar failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "UARTChar: registered correctly with major number %d\n", majorNumber);

    // Register the device class
    uartClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(uartClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(uartClass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "UARTChar: device class registered correctly\n");

    // Register the device driver
    // DEVICE_NAME: name of device file will be appeared in /dev/ folder
    uartDevice = device_create(uartClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uartDevice))
    {
        class_destroy(uartClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(uartDevice);
    }
    printk(KERN_INFO "UARTChar: device class created correctly\n");
    return 0;
}

static void __exit uart_exit(void)
{
    device_destroy(uartClass, MKDEV(majorNumber, 0)); // remove the device
    class_unregister(uartClass);                      // unregister the device class
    class_destroy(uartClass);                         // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);      // unregister the major number
    printk(KERN_INFO "UARTChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "UARTChar: Device has been opened\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    printk(KERN_INFO "UARTChar: Read device file\n");
    return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    printk(KERN_INFO "UARTChar: Write to device file\n");
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "UARTChar: Device successfully closed\n");
    return 0;
}

module_init(uart_init);
module_exit(uart_exit);