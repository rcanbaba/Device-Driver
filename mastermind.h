#ifndef __MASTERMIND_H
#define __MASTERMIND_H

#include <linux/ioctl.h>

#define MMIND_IOC_MAGIC  'k'
#define MMIND_REMAINING    _IO(MMIND_IOC_MAGIC, 0)
#define MMIND_ENDGAME 	_IO(MMIND_IOC_MAGIC,  1)
#define MMIND_NEWGAME _IOW(MMIND_IOC_MAGIC,  2, char*)

#endif