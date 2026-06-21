#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");

static DEFINE_MUTEX(my_mutex);
static dev_t dev;
static struct cdev my_cdev;
static char kernel_buf[256];
static ssize_t my_read(struct file*, char __user *, size_t, loff_t*);
static int my_release(struct inode*, struct file*);
static int my_open(struct inode*, struct file*);
static ssize_t my_write(struct file* , const char __user* , size_t, loff_t*);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.release = my_release,
	.open = my_open,
	.write = my_write,
};

static int my_open(struct inode *inode, struct file* file){
	return 0;
}

static ssize_t my_write(struct file* file, const char __user* buf , size_t len, loff_t* offset){
	if(len > sizeof(kernel_buf))
		return -EINVAL;
       	mutex_lock(&my_mutex);
	if(copy_from_user(kernel_buf,buf,len) != 0){
		mutex_unlock(&my_mutex);
		return -EFAULT;
	}
	mutex_unlock(&my_mutex);
	return len;
}

static ssize_t my_read(struct file* file, char __user* buf, size_t len, loff_t *offset){
	int msg_len;

	mutex_lock(&my_mutex);
	msg_len = strlen(kernel_buf);
	if(*offset >= msg_len){
		mutex_unlock(&my_mutex);
		return 0;
	}
	if(copy_to_user(buf,kernel_buf,msg_len)!= 0){
		mutex_unlock(&my_mutex);
		return -EFAULT;
	}
	*offset += msg_len;
	mutex_unlock(&my_mutex);
	return msg_len;
}

static int my_release(struct inode* inode, struct file* file){
	return 0;
}

static int __init indevice_init(void){
	int ret = alloc_chrdev_region(&dev, 0 , 1, "indevice");
	if(ret < 0){
		printk(KERN_ERR "alloc_chrdev_region failed: %d\n",ret);
		return ret;
	}
       	cdev_init(&my_cdev, &fops);
	ret = cdev_add(&my_cdev, dev , 1);
	if(ret < 0){
		unregister_chrdev_region(dev,1);
		printk(KERN_ERR "cdev_add failed: %d\n", ret);
		return ret;
	}
	return 0;
}

static void __exit indevice_exit(void){
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev,1);
}
module_init(indevice_init);
module_exit(indevice_exit);
