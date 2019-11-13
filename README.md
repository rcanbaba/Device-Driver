# Device Driver
Implemented a character device driver module that plays a the board game "Master Mind"

see project instruction report for details.


## Compile

cd Desktop/mastermind

sudo make
pwd
### remove
sudo rmmod mastermind

### Set Module Parameter
sudo insmod ./mastermind.ko mmin_number="1234"

### remove
sudo rm /dev/mastermind

### Set Major Number
sudo mknod /dev/mastermind c 250 0

sudo chmod 666 /dev/mastermind


### Write to device
echo "6475" > /dev/mastermind
### Read from device
cat /dev/mastermind

### Test
gcc test.c -o test
sudo ./test
