#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/timer.h>


/* comment this out for building for a RPi 1 */
#define RASPBERRY_PI2_OR_PI3


#define IO_ADDRESS(x)		(((x) & 0x00ffffff) + (((x) >> 4) & 0x0f000000) + 0xf0000000)
#define __io_address(n)		IOMEM(IO_ADDRESS(n))
#ifdef RASPBERRY_PI2_OR_PI3
  #define BCM2708_PERI_BASE	0x3F000000
#else
  #define BCM2708_PERI_BASE	0x20000000
#endif
#define GPIO_BASE		(BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

// GPIO port for transmitter
static int GPIO_TX = 4;

#define DEVICE_NAME "uartchar"
#define CLASS_NAME "uart"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAMLB");
MODULE_DESCRIPTION("Uart Device Driver");
MODULE_VERSION("0.1");

static int majorNumber;                  ///< Stores the device number -- determined automatically
static struct class *uartClass = NULL;   ///< The device-driver class struct pointer
static struct device *uartDevice = NULL; ///< The device-driver device struct pointer

//hrtimer for transmitter
static struct hrtimer hrtimer_tx;

static int BAUDRATE = 4800;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

//declare struct of register
struct GPIO_REGISTERS
{
	uint32_t GPFSEL[6];
	uint32_t Reserved1;
	uint32_t GPSET[2];
	uint32_t Reserved2;
	uint32_t GPCLR[2];
	uint32_t Reserved3;
	uint32_t GPLEV[2];
} *pGPIO_REGISTER;


static struct file_operations fops = {
        .open = dev_open,
        .read = dev_read,
        .write = dev_write,
        .release = dev_release,
};

// declare function
static enum hrtimer_restart transferData(struct hrtimer * unused);
static void GPIOOutputValueSet(int gpio, bool value);
static void GPIOFunction(int gpio, int function);

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

    //Khoi tao timer
    hrtimer_init(&hrtimer_tx, CLOCK_REALTIME, HRTIMER_MODE_REL);
    hrtimer_tx.function = transferData;
    return 0;
}

static void __exit uart_exit(void)
{
    //cancel timer
    hrtimer_cancel(&hrtimer_tx);

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
    hrtimer_start(&hrtimer_tx,  ktime_set(2, 0), HRTIMER_MODE_REL);
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


/* transfer data function */
static enum hrtimer_restart transferData(struct hrtimer * unused)
{
	
	printk(KERN_INFO "UARTChar: Transfer Data function is invoked\n");

    //forward timer
	hrtimer_forward_now(&hrtimer_tx, ktime_set(2, (1000000/BAUDRATE)*1000 ));
	
	return HRTIMER_RESTART;
}

/* set gpio function */
static void GPIOFunction(int gpio, int function)
{	
	pGPIO_REGISTER->GPFSEL[gpio / 10] = (pGPIO_REGISTER->GPFSEL[gpio / 10] & ~(0b111 << ((gpio % 10) * 3))) | ((function << ((gpio % 10) * 3)) & (0b111 << ((gpio % 10) * 3)));
}

/* set output value for GPIO */
static void GPIOOutputValueSet(int gpio, bool value)
{
	if (value)
		pGPIO_REGISTER->GPSET[gpio / 32] = (1 << (gpio % 32));
	else
		pGPIO_REGISTER->GPCLR[gpio / 32] = (1 << (gpio % 32));
}

module_init(uart_init);
module_exit(uart_exit);



/* 
Su dung timer
1. Khoi tao timer trong ham init
hrtimer_init(&hrtimer_tx, CLOCK_REALTIME, HRTIMER_MODE_REL);
2. Set function cho timer
hrtimer_tx.function = transferData;
3. Start timer
hrtimer_start(&hrtimer_tx,  ktime_set(0, 0), HRTIMER_MODE_REL);
 */