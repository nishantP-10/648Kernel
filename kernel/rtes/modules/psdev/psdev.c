/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "psdev"	/* Dev name as it appears in /proc/devices   */

static int Major;		/* Major number assigned to our device driver */
static int opens = 0;	/* Is device open?  
				 * Used to prevent multiple access to device */
static DEFINE_MUTEX(open_mutex);

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};
struct my_dev {
	char* data;
	size_t size;
	size_t count;
	size_t offset;
	struct semaphore sem;
	struct cdev cdev;
}my_dev;

static int psdev_init(void) {
	Major = register_chrdev(0, DEVICE_NAME, &fops);
	
	if (Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}
	
	printk(KERN_INFO "This device was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat the device file\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}

static void psdev_exit(void) {
	/* 
	 * Unregister the device 
	 */
	mutex_lock(&open_mutex);
	if (opens) {
		printk(KERN_ALERT "[ERROR]: Cannot unload module due to in use\n");
	} else {
		printk(KERN_ALERT "Device open is zero. Unload successfully.\n");
		unregister_chrdev(Major, DEVICE_NAME);
	}
	mutex_unlock(&open_mutex);
}


static int device_open(struct inode *inode, struct file *file) {
        
	struct task_struct *task_list;
	struct my_dev *dev;
	size_t real_size = 0;
	mutex_lock(&open_mutex);
	if (file->private_data != NULL) {
		return -EBUSY;
	}
	dev = kmalloc(sizeof(struct my_dev), GFP_KERNEL);
	opens++;
	//dev = container_of(inode->i_cdev, struct my_dev, cdev);
	file->private_data = dev;
	mutex_unlock(&open_mutex);
	try_module_get(THIS_MODULE);
	dev->size = 20;
	dev->offset = 0;
	sema_init(&dev->sem, 1);	
	dev->data = (char*) kmalloc(dev->size, GFP_KERNEL);
	real_size = sprintf(dev->data,"tid\tpid\tpr\tname\t\n");
        for_each_process(task_list) {
                int each_size = 40;
		if (task_list->rt_priority == 0 || task_list->rt_priority >= 100) {
			continue;
		}
                dev->size = dev->size + each_size;
                dev->data =  krealloc(dev->data, dev->size, GFP_KERNEL);
                real_size += sprintf(dev->data + real_size, "%d\t%d\t%d\t%s\n", task_list->pid, task_list->tgid, task_list->rt_priority, task_list->comm);
        }
	dev->size = real_size;
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *filp) {
	struct my_dev* dev;
	mutex_lock(&open_mutex);
	dev = filp->private_data;
	if (dev == NULL) {
		return -EFAULT;
	}
	opens--;
	kfree(dev->data);
	kfree(dev);
	filp->private_data = NULL;
	module_put(THIS_MODULE);
	mutex_unlock(&open_mutex);
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t * offset) {
	
	struct my_dev* dev = filp->private_data;
	down(&dev->sem);
//	printk(KERN_ALERT "[INFO]dev_offset == %d\n", dev->offset);
//	printk(KERN_ALERT "[INFO]offset == %d\n", (size_t)(*offset));
	if ((dev->offset) > dev->size) {
		up(&dev->sem);
		return 0;
	} else if ((dev->offset) + length > dev->size) {
		length = (dev->size) - (dev->offset);	
	}
	if (copy_to_user(buffer, (dev->data) + (dev->offset), length) != 0 ) {
		up(&dev->sem);
		return -EFAULT;
	}
	dev->offset += length;
	//(*offset) += length;
	up(&dev->sem);
	return length;
}


static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t * offset) {
	printk(KERN_ALERT "Writing is not supportive in this devcie\n");
	return -ENOTSUPP;
}
module_init(psdev_init);
module_exit(psdev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZIYUAN SONG");
