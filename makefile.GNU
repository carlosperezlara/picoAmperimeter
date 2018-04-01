all:
	gcc -std=gnu99 -I/usr/include/libusb-1.0 -c pico.c 
	gcc -L/usr/lib64/ pico.o -lusb-1.0 -o pico
	rm pico.o
