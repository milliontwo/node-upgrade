#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/*************************************************************************************
* This is a simple command line tool to help upgrading the nodes on the milliontwo clock
* Built for the millionboot AVR I2C bootloader 
* http://github.com/milliontwo/millionboot
* (c) 2015 najiji
***************************************************************************************/
#define FIRMWARE_UPDATE_COMMAND 0xAA


extern int errno;

typedef enum {NOTHING, ENTER_UPGRADE, UPGRADE_SINGLE, UPGRADE_ALL} Mode;


/*
 * Opens the i2c bus and returns the file descriptor
 */
int open_i2c(char* filename) {
	int file;
	if((file = open(filename, O_RDWR)) < 0){
		fprintf(stderr, "[Error] Could not open %s\n", filename);
		exit(1);
	}
	printf("[OK] Opened I2C Interface on %s\n", filename);
	return file;
}


void send_enter_upgrade_command(int file) {
	/* set device id to general call */ 
	if (ioctl(file, I2C_SLAVE, 0x00) < 0) {
		fprintf(stderr, "[Error] Failed to set general call address\n");
		exit(1);
	}
	int command = FIRMWARE_UPDATE_COMMAND;
	/* Write firmware upgrade enable code */
	if (write(file, &command, 1) != 1) {
		fprintf(stderr, "[Error] Transaction failed with error %s\n", strerror(errno));
		exit(1);
	}
	printf("[OK] Sent firmware upgrade command. Nodes are now awaiting mew code\n");
}
	

int main(int argc, char **argv) {
	
	/* Default I2C character device */
	char* device_path = "/dev/i2c-1";

	/* Parse arguments */
	Mode mode = NOTHING;
	int c;
	while((c = getopt(argc, argv, "edf")) != -1) {
		switch(c) {
			case 'e':
				mode = ENTER_UPGRADE;	
				break;
			case 'f':
				device_path = optarg;
				break;
			
		}
	}

	/* Do work according to selected mode */
	int file;
	switch(mode) {
		case ENTER_UPGRADE:
			file = open_i2c(device_path);
			send_enter_upgrade_command(file);
			break;
	}
	


	close(file);
	return 0;
}
