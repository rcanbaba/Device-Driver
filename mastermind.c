#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/cred.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>	/* kmalloc() */
#include <linux/fs.h>	/* everything... */
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

#include <asm/switch_to.h>	/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include "mastermind.h"

#define MASTERMIND_MAJOR 0
#define MASTERMIND_MINOR 0
#define MAX_GUESS 10
#define MASTERMIND_NUMBER "4325"

int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = MASTERMIND_MINOR;
//int secret_number;
int max_guess = MAX_GUESS;
char *mastermind_number = MASTERMIND_NUMBER;

module_param(mastermind_major, int, S_IRUGO);
module_param(mastermind_minor, int, S_IRUGO);
//module_param(secret_number, int, S_IRUGO);
module_param(max_guess, int, S_IRUGO);
module_param(mastermind_number, charp, S_IRUGO);

MODULE_AUTHOR("Can, Kubi, Emin");
MODULE_LICENSE("Dual BSD/GPL");

int mastermind_init_module(void);

struct mastermind_dev {
	char *mastermind_number;
	int guessCount;
	char *history;
	unsigned long size;
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
	printk(KERN_DEBUG "Read: count before all: %d\n",count);
	printk(KERN_DEBUG "Read: dev->guessCount before all: %d\n",dev->guessCount);
	if (down_interruptible(&dev->sem)){
		return -ERESTARTSYS;
	}
	
	if (*f_pos >= dev->size){
		printk(KERN_INFO "Read: info 1\n");
        up(&dev->sem);
        return retval;
    }

    if (*f_pos + count > dev->size){
		printk(KERN_INFO "Read: info 2\n");
        count = dev->size - *f_pos;
	}
	
	//printk(KERN_DEBUG "Read: dev->size: %l\n",dev->size);
	//printk(KERN_DEBUG "Read: *f_pos: %l\n",*f_pos);
        
	printk(KERN_DEBUG "Read: dev->history length: %d\n",strlen(dev->history));
	if (strlen(dev->history) < 1 ){
		retval = 0;
		goto out;
	}else{
	printk(KERN_DEBUG "Read: (else) dev->history: \n%s\n",dev->history);
		//dev->history[strlen(dev->history)-1] = '\0'; 
		if(count == strlen(dev->history)){
		if (copy_to_user(buf, dev->history, count)) { //4096 olabilir
			printk(KERN_INFO "Read: in copy_to_user \n");
			retval = -EFAULT;
			goto out;
		}
		printk(KERN_DEBUG "Read: (out) 1 buf: \n%s\n",buf);
        printk(KERN_DEBUG "Read: (out) 1 count: \n%d\n",count);
        *f_pos += count;
        up(&dev->sem);
        return count;
	}
	printk(KERN_DEBUG "Read: (else) buf: \n%s\n",buf);
	printk(KERN_DEBUG "Read: (else) count: %d\n",count);
	printk(KERN_DEBUG "Read: (else) dev->history: \n%s\n",dev->history);
	printk(KERN_DEBUG "Read: (else) strlen(dev->history): %d\n",strlen(dev->history));
	}
	if(strlen(dev->history) == count - 16 ){
		if (copy_to_user(buf, dev->history, count - 16)) {
			printk(KERN_DEBUG "Read: (in) 2 buf: \n%s\n",buf);
			up(&dev->sem);
			return -EFAULT;
		}
	printk(KERN_DEBUG "Read: (3else) buf: \n%s\n",buf);
	printk(KERN_DEBUG "Read: (3else) count: %d\n",count);
	printk(KERN_DEBUG "Read: (3else) dev->history: \n%s\n",dev->history);
	printk(KERN_DEBUG "Read: (3else) strlen(dev->history): %d\n",strlen(dev->history));
	}
	
	out:
	printk(KERN_INFO "Read: out:\n");
	*f_pos = *f_pos + (dev->guessCount)*16;
	up(&dev->sem);
	return count;
}

ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){

	struct mastermind_dev *dev = filp->private_data;
	//ssize_t retval = -ENOMEM;
	int plus = 0;
	int minus = 0;
	int i;
	int j;
	int u = 14, ctr = 0;
	int g_Count = 0;
	char *temp_guess = kmalloc(count, GFP_KERNEL);
	//temp_guess = kmalloc(sizeof(char) * (count), GFP_KERNEL);
	//char *guesscnt = kmalloc(4, GFP_KERNEL);
	char pluss;
	char minuss;
	char *tempbuf = kmalloc(16, GFP_KERNEL);
	int lengthOfHistory;
	if( dev->guessCount < max_guess ){
		dev->guessCount++;
	}else{
		goto out;
	}	
	
	printk(KERN_DEBUG "Write: count: %d\n",count);
	if (down_interruptible(&dev->sem)){
		printk(KERN_INFO "WRITE: ERESTARTSYS bozuk\n");
		return -ERESTARTSYS;
	}
	if(copy_from_user(temp_guess, buf, count)){
		printk(KERN_INFO "WRITE: copy_from_user Bozuk!\n");
		up(&dev->sem);
		return -EFAULT;
	}
	printk(KERN_DEBUG "Write: temp_guess: %s\n",temp_guess);

	
	// char *secret = kmalloc(4, GFP_KERNEL);
	// sprintf(secret, "%d", dev->secretNumber);
	//printk(KERN_INFO "Mastermind: write before for\n");
	
	for(i=0; i<count; i++){ //temp_guess için
		if(mastermind_number[i] == temp_guess[i]){
			plus++;
			continue;
		}
		for(j=0; j<count; j++){ //secretNumber için
			if ( i==j ){
				continue;
			}else if (mastermind_number[j] == temp_guess[i]){
				minus++;
				break;
			}//else if
		}//inside for
	}//outside for
	printk(KERN_DEBUG "Write: deev->mastermind_number: %s\n",dev->mastermind_number);
	printk(KERN_DEBUG "Write: mastermind_number: %s\n",mastermind_number);
	printk(KERN_DEBUG "Write: plus: %d\n",plus);
	printk(KERN_DEBUG "Write: minus: %d\n",minus);
	//printk(KERN_INFO "Mastermind: write after for\n");
	//printk(KERN_DEBUG "Write: guesscnt: %s\n",guesscnt);
	printk(KERN_DEBUG "Write: dev->guessCount: %d\n",dev->guessCount);
	//fillguesscnt(guesscnt, dev->guessCount);
	//kstrtol(guesscnt,10,dev->guessCount);
	//printk(KERN_DEBUG "Write: guesscnt after: %s\n",guesscnt);
	//printk(KERN_DEBUG "Write: temp_guess after: %s\n",temp_guess);
	//sablon: xxxx m+ n- abcd\n (16 bytes)
	strcpy(tempbuf, temp_guess);
	//printk(KERN_DEBUG "Write: tempbuf after: %s\n",tempbuf);
	pluss = plus + '0';
	minuss = minus + '0';
	tempbuf[4] = ' ';
	tempbuf[5] = pluss;
	tempbuf[6] = '+';
	tempbuf[7] = ' ';
	tempbuf[8] = minuss;
	tempbuf[9] = '-';
	tempbuf[10] = ' ';

	g_Count = dev->guessCount;
    while(g_Count > 0 || u > 10){
        ctr = g_Count % 10;
        if (g_Count == 0){ 
            tempbuf[u] = '0';
        }
        else {
            tempbuf[u] = ctr + '0';
        } 
        g_Count = g_Count / 10;
        u--;
    }
	
	//strcat(tempbuf, guesscnt);
	tempbuf[15] = '\n';
	printk(KERN_DEBUG "Write: tempbuf after: %s\n",tempbuf);
	//printk(KERN_INFO "Mastermind: write before history\n");
	//log to history
	
	lengthOfHistory = strlen(dev->history);
	printk(KERN_DEBUG "Write: lengthOfHistory: %d\n",lengthOfHistory);
	printk(KERN_DEBUG "Write: 1 dev->history: %s\n",dev->history);
	strcat(dev->history, tempbuf);
	printk(KERN_DEBUG "Write: 2 dev->history: %s\n",dev->history);
	dev->history[lengthOfHistory+16] = '\0'; // BU 15 mi olacak DENE
	printk(KERN_DEBUG "Write: 3 dev->history: %s\n",dev->history);
	//printk(KERN_INFO "Mastermind: write after history\n");
	
	dev->size += 16;
	out:
	//free
	plus = 0; minus=0;
	tempbuf[0] = '\0';
	//secret[0] = '\0';
	//kfree(secret);
	kfree(temp_guess);
	kfree(tempbuf);
	//printk(KERN_INFO "Mastermind: write free run\n");
	
	up(&dev->sem);
	return count;

}//write
/*
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
*/
int mastermind_trim(struct mastermind_dev *dev){

	printk(KERN_INFO "Mastermind: Trim function\n");
	//trim de satır satır mı silmemiz lazım??
	dev->history[0] = '\0';
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
	//char *newGameNumber = kmalloc(5, GFP_KERNEL);
	struct mastermind_dev *dev = filp->private_data;
	//char __user *masterNo = (void *)0x0;
	int err = 0;
	
	if (_IOC_TYPE(cmd) != MMIND_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MMIND_IOC_MAXNR) return -ENOTTY;
	
	if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));

    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

    if (err) return -EFAULT;
    
	switch(cmd) {
		case MMIND_REMAINING:
			if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
			printk(KERN_DEBUG "Ioctl: remaining: max_guess: %d\n",dev->guessCount);
			printk(KERN_DEBUG "Ioctl: remaining: max_guess: %d\n",max_guess);
			if(dev->guessCount > 0){
				return max_guess - dev->guessCount;
			}else{
				return 0;
			}
			
			break;
		case MMIND_ENDGAME:
			if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
			dev->guessCount = 0;
			dev->history[0] = '\0';
			printk(KERN_DEBUG "Ioctl: endgame: max_guess: %d\n",dev->guessCount);
			printk(KERN_DEBUG "Ioctl: endgame: %s\n",dev->history);
			//mastermind_cleanup_module();
			break;
		case MMIND_NEWGAME:
			if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
			strncpy_from_user(mastermind_number, (const char __user *)arg, 4);
			printk(KERN_DEBUG "Ioctl: newgame: mastermind_number: %s\n",mastermind_number);
			mastermind_init_module();
			printk(KERN_DEBUG "Ioctl: newgame: dev->mastermind_number: %s\n",dev->mastermind_number);
			//dev->guessCount = 0;
			//dev->history[0] = '\0';
			break;
		default: /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}
	return retval;
}
loff_t mmind_llseek(struct file *filp, loff_t off, int whence)
{
    struct mastermind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations mastermind_fops = {
	.owner = THIS_MODULE,
	.llseek = mmind_llseek,
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
	printk(KERN_INFO "Mastermind: INIT started \n");
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
	dev->mastermind_number = kmalloc(5, GFP_KERNEL);
	dev->mastermind_number = mastermind_number;
	dev->guessCount = 0;
	dev->size = 0;
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
