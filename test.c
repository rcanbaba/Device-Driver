
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "mastermind_ioctl.h"

int main()
{
	int fd, retval = 0;
	char mmindNumber[3];
	
	fd = open("/dev/mastermind", 0);
	if (fd == -1) {
		printf("Device cannot be accessed!\n");
	}
	
	printf("1 : REMAINING\n");
	printf("2 : END GAME\n");
	printf("3 : NEW GAME\n");
	printf("Make Your Choice:\n");
	int choice;
	scanf("%d", &choice);
	
	switch(choice) {
		case 1:
			retval = ioctl(fd, MMIND_REMAINING);
			printf("remaining guesses: %d\n",retval);
			//printf("sonra %s",retval);
			break;
		case 2:
			retval = ioctl(fd, MMIND_ENDGAME);
			break;
		case 3:
            printf("Mastermind Number:\n");
            scanf("%s", mmindNumber);
            retval = ioctl(fd, MMIND_NEWGAME, mmindNumber);
            printf("retval-new game: %d\n",retval);
			break;
		default:
			printf("Sorry I could not understand you\n");
	}
	
	if (retval == -EPERM) {
		printf("You need to be root!\n");
	}
	
	close(fd);
	return 0;
}
