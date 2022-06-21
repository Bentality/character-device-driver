// Modified from https://blog.sourcerer.io/writing-a-simple-linux-kernel-module-d9dc3762c234
// Under GPL License

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CSIS Kernel Module");

#define DEVICE_NAME "CSISKernelModule"
#define EXAMPLE_MSG "Welcome to CSIS-443!\n"
#define MSG_BUFFER_LEN 21

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static int major_num;
static int device_open_count = 0;
static char msg_buffer[MSG_BUFFER_LEN];
static char *msg_ptr;

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
 .read = device_read,
 .write = device_write,
 .open = device_open,
 .release = device_release
};

/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {
 int bytes_read = 0;
 /* If we’re at the end, loop back to the beginning */
 if (*msg_ptr == 0) {
 msg_ptr = msg_buffer;
 }
 /* Put data in the buffer */
 while (len && *msg_ptr) {
 /* Buffer is in user data, not kernel, so you can’t just reference
 * with a pointer. The function put_user handles this for us */
 put_user(*(msg_ptr++), buffer++);
 len--;
 bytes_read++;
 }
 return bytes_read;
}

/* Called when a process tries to write to our device */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
 /* This is a read-only device */
 int count = 0;
 memset (msg_buffer, 0, MSG_BUFFER_LEN);

 while(len > 0){
  copy_from_user(msg_buffer, buffer, MSG_BUFFER_LEN-1);
  count ++;
  len--;
  msg_buffer[MSG_BUFFER_LEN-1] = 0x00;
 }

 printk(KERN_INFO "Writing to CSISKernelModule Device. Recieved: %s", msg_buffer);

 return count;

}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file) {
 /* If device is open, return busy */
 if (device_open_count) {
 return -EBUSY;
 }
 device_open_count++;
 try_module_get(THIS_MODULE);
 return 0;
}

/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file) {
 /* Decrement the open counter and usage count. Without this, the module would not unload. */
 device_open_count--;
 module_put(THIS_MODULE);
 return 0;
}

static int __init CSISKernelModule_init(void) {

 /* Fill buffer with our message */
 strncpy(msg_buffer, EXAMPLE_MSG, MSG_BUFFER_LEN);
 /* Set the msg_ptr to the buffer */
 msg_ptr = msg_buffer;

 /* Try to register character device */
 major_num = register_chrdev(0, "CSISKernelModule", &file_ops);
 if (major_num < 0) {
    printk(KERN_ALERT "Could not register device: %d\n", major_num);
    return major_num;
 } else {
    printk(KERN_INFO "CSISKernelModule module loaded with device major number %d\n", major_num);
    return 0;
 }
}

static void __exit CSISKernelModule_exit(void) {
 unregister_chrdev(major_num, DEVICE_NAME);
 printk(KERN_INFO "CSISKernelModule Exiting\n");
}


module_init(CSISKernelModule_init);
module_exit(CSISKernelModule_exit);
