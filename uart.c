#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#define  DEVICE_NAME "uartchar"
#define  CLASS_NAME  "uart"    

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAMLB");
MODULE_DESCRIPTION("Uart Device Driver");
MODULE_VERSION("0.1");

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  uartClass  = NULL; ///< The device-driver class struct pointer
static struct device* uartDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};


static int __init uart_init(void){
   printk(KERN_INFO "UARTChar: Initializing the UARTChar LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "UARTChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "UARTChar: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   uartClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(uartClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(uartClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "UARTChar: device class registered correctly\n");

   // Register the device driver
   uartDevice = device_create(uartClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(uartDevice)){               // Clean up if there is an error
      class_destroy(uartClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(uartDevice);
   }
   printk(KERN_INFO "UARTChar: device class created correctly\n"); // Made it! device was initialized
   return 0;
}


static void __exit uart_exit(void){
   device_destroy(uartClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(uartClass);                          // unregister the device class
   class_destroy(uartClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "UARTChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "UARTChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}


static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "UARTChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "UARTChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "UARTChar: Received %zu characters from the user\n", len);
   return len;
}


static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "UARTChar: Device successfully closed\n");
   return 0;
}

module_init(uart_init);
module_exit(uart_exit);