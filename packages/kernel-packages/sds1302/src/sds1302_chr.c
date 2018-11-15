#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/types.h>

#include "sds1302.h"

// the device will appear at /dev/sds1302
#define DEVICE_NAME     "sds1302"
#define CLASS_NAME      "sds1302"

#define MAGIC_CODE      0x42
#define DIR_OUT         0  // 0 for output in the register

#define RTC_RST_PORT    12
#define RTC_SCLK_PORT   13
#define RTC_DATA_PORT   14

#define	RTC_CMD_READ	0x81		// Read command
#define	RTC_CMD_WRITE	0x80		// Write command

#define RTC_ADDR_RAM0	0x20		// Address of RAM0
#define RTC_ADDR_TCR	0x08		// Address of trickle charge register
#define RTC_ADDR_CTRL   0x07            // address of control register
#define	RTC_ADDR_YEAR	0x06		// Address of year register
#define	RTC_ADDR_DAY	0x05		// Address of day of week register
#define	RTC_ADDR_MON	0x04		// Address of month register
#define	RTC_ADDR_DATE	0x03		// Address of day of month register
#define	RTC_ADDR_HOUR	0x02		// Address of hour register
#define	RTC_ADDR_MIN	0x01		// Address of minute register
#define	RTC_ADDR_SEC	0x00		// Address of second register

struct datetime {
  unsigned char year;
  unsigned char month;
  unsigned char mday;
  unsigned char hour;
  unsigned char min;
  unsigned char sec;
  unsigned char wday;
};

struct datetime g_datetime = { 0, 0, 0, 0, 0, 0, 0};

static int g_major;
static struct class* g_chr_class;
static struct device* g_chr_device;

static unsigned char ascii2bcd(unsigned char ascii_data) {
  unsigned char bcd_data = 0;
  bcd_data = (((ascii_data / 10) << 4) + ((ascii_data % 10)));
  return bcd_data;
}

static unsigned char bcd2ascii(unsigned char bcd_data) {
  unsigned char ascii_data = 0;
  ascii_data = (((bcd_data & 0xf0) >> 4) * 10 + (bcd_data & 0x0f));
  return ascii_data;
}

static inline void ds1302_reset(void) {
  // enable output
  gpio_direction_output(RTC_SCLK_PORT, DIR_OUT);
  gpio_direction_output(RTC_DATA_PORT, DIR_OUT);

  // output low
  gpio_set_value(RTC_SCLK_PORT, 0);
  gpio_set_value(RTC_DATA_PORT, 0);
}

static inline void ds1302_clock(void) {
  gpio_set_value(RTC_SCLK_PORT, 1); // clock high
  udelay(2);
  gpio_set_value(RTC_SCLK_PORT, 0); // clock low
  udelay(2);
}

static inline void ds1302_start(void) {
  gpio_set_value(RTC_RST_PORT, 1);
}

static inline void ds1302_stop(void) {
  gpio_set_value(RTC_RST_PORT, 0);
}

static inline void ds1302_txbit(int bit) {
  if (bit)
    gpio_set_value(RTC_DATA_PORT, 1);
  else
    gpio_set_value(RTC_DATA_PORT, 0);
}

static inline int ds1302_rxbit(void) {
  return gpio_get_value(RTC_DATA_PORT);
}

static inline void ds1302_set_tx(void) {
  gpio_direction_output(RTC_SCLK_PORT, DIR_OUT);
  gpio_direction_output(RTC_DATA_PORT, DIR_OUT);
}

static inline void ds1302_set_rx(void) {
  gpio_direction_output(RTC_SCLK_PORT, DIR_OUT);
  gpio_direction_input(RTC_DATA_PORT);
}

static void ds1302_sendbits(unsigned char val) {
  int i;

  ds1302_set_tx();

  for (i = 8; (i); i--, val >>= 1) {
    ds1302_txbit(val & 0x1);
    ds1302_clock();
  }
}

static unsigned int ds1302_recvbits(void) {
  unsigned int val;
  int i;

  ds1302_set_rx();

  for (i = 0, val = 0; (i < 8); i++) {
    val |= (ds1302_rxbit() << i);
    ds1302_clock();
  }

  return val;
}

static unsigned int ds1302_readbyte(unsigned char addr) {
  unsigned int val;

  ds1302_reset();

  ds1302_start();
  ds1302_sendbits(((addr & 0x3f) << 1) | RTC_CMD_READ);
  val = ds1302_recvbits();
  ds1302_stop();

  return val;
}

static void ds1302_writebyte(unsigned char addr, unsigned char val) {
  ds1302_reset();

  ds1302_start();
  ds1302_sendbits(((addr & 0x3f) << 1) | RTC_CMD_WRITE);
  ds1302_sendbits(val);
  ds1302_stop();
}


// ===================================================================
// === fops interfaces
static int sds1302_open(struct inode *inode , struct file *file) {
  S_DBG(KERN_INFO "Device has been opened\n");
  return 0;
}

/*
 * the device release function that is called whenever the device is
 * closed/released by the userspace program.
 */
static int sds1302_release(struct inode *inode , struct file *file) {
  S_DBG("Device successfully closed\n");
  return 0;
}

/*
 * This function is called whenever device is being read from user space
 */
static ssize_t sds1302_read(struct file *file, char __user *buf,
                            size_t count, loff_t *pos) {
  int error_count = 0;

  g_datetime.sec = bcd2ascii(ds1302_readbyte(RTC_ADDR_SEC));
  g_datetime.min = bcd2ascii(ds1302_readbyte(RTC_ADDR_MIN));
  g_datetime.hour = bcd2ascii(ds1302_readbyte(RTC_ADDR_HOUR));
  g_datetime.wday = bcd2ascii(ds1302_readbyte(RTC_ADDR_DAY)); // week day
  g_datetime.mday = bcd2ascii(ds1302_readbyte(RTC_ADDR_DATE)); // month day
  g_datetime.month = bcd2ascii(ds1302_readbyte(RTC_ADDR_MON));
  g_datetime.year = bcd2ascii(ds1302_readbyte(RTC_ADDR_YEAR));

#if 0
  S_DBG("%s: tm is secs=%d, mins=%d, hours=%d, mday=%d, mon=%d,"\
        " year=%d, wday=%d\n", __func__, g_datetime.sec,
        g_datetime.min, g_datetime.hour, g_datetime.mday,
        g_datetime.month + 1, g_datetime.year, g_datetime.mday);
#endif

  // copy_to_user has the format (* to, *from, size) and returns 0 on success
  error_count = copy_to_user(buf, (char*)&g_datetime, count);
  if (error_count) {
    S_ALERT("Failed to send %d characters to the user\n", error_count);
    return -EFAULT; // Failed -- return a bad address message (i.e. -14)
  }

  return count;
}

static ssize_t sds1302_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *pos) {

  struct datetime *tmp = (struct datetime*)buf;

  // disable write protect
  ds1302_writebyte(RTC_ADDR_CTRL, 0x00);
  // Stop RTC
  ds1302_writebyte(RTC_ADDR_SEC, ds1302_readbyte(RTC_ADDR_SEC) | 0x80);

  ds1302_writebyte(RTC_ADDR_SEC, ascii2bcd(tmp->sec));
  ds1302_writebyte(RTC_ADDR_MIN, ascii2bcd(tmp->min));
  ds1302_writebyte(RTC_ADDR_HOUR, ascii2bcd(tmp->hour));
  ds1302_writebyte(RTC_ADDR_DAY, ascii2bcd(tmp->wday));
  ds1302_writebyte(RTC_ADDR_DATE, ascii2bcd(tmp->mday));
  ds1302_writebyte(RTC_ADDR_MON, ascii2bcd(tmp->month));
  ds1302_writebyte(RTC_ADDR_YEAR, ascii2bcd(tmp->year % 100));

  // Start RTC
  ds1302_writebyte(RTC_ADDR_SEC, ds1302_readbyte(RTC_ADDR_SEC) & ~0x80);
  // enable write protect
  ds1302_writebyte(RTC_ADDR_CTRL, 0x80);

  return count;
}

static struct file_operations sds1302_fops = {
  .owner = THIS_MODULE,
  .read = sds1302_read,
  .write = sds1302_write,
  .open = sds1302_open,
  .release = sds1302_release,
};

static void sds1302_ram_test(void) {
  ds1302_writebyte(RTC_ADDR_SEC, 0x00);

  // Write a magic value to the DS1302 RAM, and see if it sticks.
  ds1302_writebyte(RTC_ADDR_RAM0, MAGIC_CODE);
  if (ds1302_readbyte(RTC_ADDR_RAM0) != MAGIC_CODE) {
    S_ALERT("write to DS1302 RAM failed.");
  }
  //S_DBG("read 0x%x\n", ds1302_readbyte(RTC_ADDR_RAM0));
}

static int __init sds1302_init(void) {
  S_DBG("install the DS1302 module\n");

  g_major = register_chrdev(0, DEVICE_NAME, &sds1302_fops);
  if (g_major < 0) {
    S_ALERT("failed to register a major number\n");
    return -1;
  }
  S_DBG("registered correctly with major number: %d\n", g_major);

  // Register the device class
  g_chr_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(g_chr_class)) { // Check for error and clean up if there is
    unregister_chrdev(g_major, DEVICE_NAME);
    S_ALERT("Failed to register device class\n");
    return -1;
  }
  S_DBG("device class registered correctly\n");

  // Register the device driver
  g_chr_device = device_create(g_chr_class, NULL,
                               MKDEV(g_major, 0), NULL, DEVICE_NAME);
  if (IS_ERR(g_chr_device)) { // Clean up if there is an error
    class_destroy(g_chr_class);
    unregister_chrdev(g_major, DEVICE_NAME);
    S_ALERT("Failed to create the device\n");
    return -1;
  }
  S_DBG("device class created correctly\n");

  if (gpio_request(RTC_SCLK_PORT, "rtc-sclk")) {
    S_ALERT("request pin %d err\n", RTC_SCLK_PORT);
    return -1;
  }

  if (gpio_request(RTC_DATA_PORT, "rtc-data")) {
    S_ALERT("request pin %d err\n", RTC_DATA_PORT);
    return -1;
  }

  if (gpio_request(RTC_RST_PORT, "rtc-rst")) {
    S_ALERT("request pin %d err\n", RTC_RST_PORT);
    return -1;
  }
  S_DBG("request PIN: %d, %d, %d, ok\n",
        RTC_SCLK_PORT, RTC_DATA_PORT, RTC_RST_PORT);

  // set output
  gpio_direction_output(RTC_DATA_PORT, DIR_OUT);
  gpio_direction_output(RTC_SCLK_PORT, DIR_OUT);
  gpio_direction_output(RTC_RST_PORT, DIR_OUT);

  sds1302_ram_test();

#if 0
  gpio_set_value(RTC_SCLK_PORT, 0);
  gpio_set_value(RTC_DATA_PORT, 0);
  gpio_set_value(RTC_RST_PORT, 0);
#endif

  return 0;
}

static void __exit sds1302_exit(void) {
#if 0
  // for test
  gpio_set_value(RTC_SCLK_PORT, 1);
  gpio_set_value(RTC_DATA_PORT, 1);
  gpio_set_value(RTC_RST_PORT, 1);
#endif

  gpio_free(RTC_SCLK_PORT);
  gpio_free(RTC_DATA_PORT);
  gpio_free(RTC_RST_PORT);

  // remove the device
  device_destroy(g_chr_class, MKDEV(g_major, 0));
  // unregister the device class
  class_unregister(g_chr_class);
  // remove the device class
  class_destroy(g_chr_class);
  // unregister the major number
  unregister_chrdev(g_major, DEVICE_NAME);

  S_DBG("remove the DS1302 module\n");
}

module_init(sds1302_init);
module_exit(sds1302_exit);

MODULE_AUTHOR("Neil Chen <jingsi.chen@qq.com>");
MODULE_DESCRIPTION("S DS1302 char driver");
MODULE_LICENSE("GPL");
