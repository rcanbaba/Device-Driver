# sistemDenemeler
## DERLERKEN

cd Desktop/mastermind

sudo make
sudo rmmod mastermind
sudo insmod ./mastermind.ko mmin_number="1234"
sudo rm /dev/mastermind
sudo mknod /dev/mastermind c 250 0
sudo chmod 666 /dev/mastermind

echo "6475" > /dev/mastermind
cat /dev/mastermind

gcc test.c -o test
sudo ./test
