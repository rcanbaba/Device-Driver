#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/file.h> 
#include <linux/cred.h> 
#include <linux/uidgid.h>
#include <linux/sched.h>

#include <asm/switch_to.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include "mastermind.h"

#define MASTERMIND_MAJOR 0
#define MASTERMIND_MINOR 0
#define MAX_GUESS 10

int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = MASTERMIND_MINOR;
int secret_number;
int max_guess = MAX_GUESS;

module_param(mastermind_major, int, S_IRUGO);
module_param(secret_number, int, S_IRUGO);
module_param(max_guess, int, S_IRUGO);

MODULE_AUTHOR("Can, Kubi, Emin");
MODULE_LICENSE("Dual BSD/GPL");

void fillguesscnt(char*, int);
struct mastermind_dev {
	char *secretNumber;
	int guessCount;
	char *history;
	struct semaphore sem;
	struct cdev cdev;
};

struct mastermind_dev *mastermind_device;

int mastermind_open(struct inode *inode, struct file *filp){
	
    struct mastermind_dev *dev;

    dev = container_of(inode->i_cdev, struct mastermind_dev, cdev);
    filp->private_data = dev;
    printk(KERN_INFO "Mastermind: Open function\n");
	
    return 0;
}


int mastermind_release(struct inode *inode, struct file *filp){
	printk(KERN_INFO "Mastermind: Release function\n");
    return 0;
}

ssize_t mastermind_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	
    struct mastermind_dev *dev = filp->private_data;
    ssize_t retval = 0;
	
    if (down_interruptible(&dev->sem)){
		return -ERESTARTSYS;
	}
	if (strlen(dev->history) < 1 ){
		retval = 0;
		goto out;
	}else{
		if (copy_to_user(buf, dev->history, (dev->guessCount)*16)) { //4096 olabilir
			retval = -EFAULT;
			goto out;
		}		
	}
	
	out:
	up(&dev->sem);
	return retval;
}

ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){

    struct mastermind_dev *dev = filp->private_data;
	ssize_t retval = -ENOMEM;
	int plus = 0;
    int minus = 0;
	int i;
	int j;
	char *temp_guess = kmalloc(count, GFP_KERNEL);
	char *guesscnt = kmalloc(4, GFP_KERNEL);
	char pluss = plus + '0';
	char minuss = minus + '0';
	char *tempbuf = kmalloc(16, GFP_KERNEL);
	int lengthOfHistory;
	
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;	
	
	if(copy_from_user(temp_guess, buf, count)){
        retval = -EFAULT;
		goto out;
    }
        

    dev->guessCount++;
   // char *secret = kmalloc(4, GFP_KERNEL);
   //  sprintf(secret, "%d", dev->secretNumber);

    for(i=0; i<count; i++){	//temp_guess için
		
		if(dev->secretNumber[i] == temp_guess[i]){	
			plus++;
			continue;
		}
		
		for(j=0; j<count; j++){		//secretNumber için	
			if ( i==j ){
				continue;
			}else if ( dev->secretNumber[j] == temp_guess[i]){
				minus++;
				break;
			}//else if
		}//inside for
	}//outside for

	
	fillguesscnt(guesscnt, dev->guessCount);
	//sablon: xxxx m+ n- abcd\n (16 bytes)
	strcpy(tempbuf, temp_guess);
	tempbuf[4] = ' ';
	tempbuf[5] = pluss;
	tempbuf[6] = '+';
	tempbuf[7] = ' ';
	tempbuf[8] = minuss;
	tempbuf[9] = '-';
	tempbuf[10] = ' ';
	strcat(tempbuf, guesscnt);
	tempbuf[15] = '\n';
	
	
	//log to history
	lengthOfHistory = strlen(dev->history);
	strcat(dev->history, tempbuf);
	dev->history[lengthOfHistory+16] = '\0';
	
	
	//free
	plus = 0; minus=0;
	tempbuf[0] = '\0';
	//secret[0] = '\0';
	//kfree(secret);
	kfree(guesscnt);
	kfree(temp_guess);
	kfree(tempbuf);
	
	out:
	return retval;
	
}//write

void fillguesscnt(char* g, int gc) {
	int temp = gc;
	int count = 0;
	int i=0;
	char a[4];
	
	while (temp!= 0){
		count++;
		temp /=10;
	}
	
	for(i=0; i<=3-count; i++){
		g[i] = '0';
	}	
	
	sprintf(a, "%d", gc);
	
	strcat(g, a);
}

int mastermind_trim(struct mastermind_dev *dev){
	
	printk(KERN_INFO "Mastermind: Trim function\n");
		//trim de satır satır mı silmemiz lazım??
	kfree(dev->history);
    return 0;
}

void mastermind_cleanup_module(void)
{
    dev_t devno = MKDEV(mastermind_major, mastermind_minor);

    if (mastermind_device) {
		mastermind_trim(mastermind_device);
		cdev_del(&mastermind_device->cdev);
	//	cdev_del(&mastermind_device[0].cdev);
	//	kfree(mastermind_device);
	}
	kfree(mastermind_device);
    unregister_chrdev_region(devno, 1);
    printk(KERN_WARNING "Mastermind: Cleanup finished \n");
}

long mastermind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	char *newGameNumber = kmalloc(5, GFP_KERNEL);	
	struct mastermind_dev *dev = filp->private_data;
		
	switch(cmd) {
		case MMIND_REMAINING:
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			printk("Remaining guesses: %d", max_guess - dev->guessCount);
			break;
		case MMIND_ENDGAME:
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			dev->guessCount = 0;
			dev->history[0] = '\0';
			break;
		case MMIND_NEWGAME:
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM; 		
			copy_from_user(newGameNumber, (char *)arg, 4);
			newGameNumber[4] = '\0';
			strcpy(dev->secretNumber, newGameNumber);
			dev->guessCount = 0;
			dev->history[0] = '\0';
			break;
		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	return retval;
}


struct file_operations mastermind_fops = {
	.owner = THIS_MODULE,
	.read = mastermind_read,
	.write = mastermind_write,
	.unlocked_ioctl = mastermind_ioctl,
	.open = mastermind_open,
	.release = mastermind_release,
};


int mastermind_init_module(void)
{
	int result;
	int err;
	dev_t devno = 0;
	struct mastermind_dev *dev;
	
	if (mastermind_major) {
		devno = MKDEV(mastermind_major, mastermind_minor);
		result = register_chrdev_region(devno, 1, "mastermind");
	} else {
		result = alloc_chrdev_region(&devno, mastermind_minor, 1, "mastermind");
		mastermind_major = MAJOR(devno);
	}
	if (result < 0) {
		printk(KERN_WARNING "mastermind: can't get major %d\n", mastermind_major);
		return result;
	}
	mastermind_device = kmalloc(sizeof(struct mastermind_dev), GFP_KERNEL);
	if (!mastermind_device) {
		result = -ENOMEM;
		goto fail;
	}
	memset(mastermind_device, 0, sizeof(struct mastermind_dev));
	
	/* Initialize device. */
	
	dev = mastermind_device;
	dev->history = kmalloc(4096, GFP_KERNEL);
	dev->secretNumber = kmalloc(5, GFP_KERNEL);
	dev->guessCount = 0;
	sema_init(&dev->sem,1);
	cdev_init(&dev->cdev, &mastermind_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mastermind_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	
	if (err)
		printk(KERN_NOTICE "Error %d adding mastermind", err);
	
	return 0; /* succeed */
	
	fail:
	mastermind_cleanup_module();
	return result;
	
}


module_init(mastermind_init_module);
module_exit(mastermind_cleanup_module);
