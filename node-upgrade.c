#include <stdio.h>
#include <string.h>
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
extern char *optarg;

typedef enum {NOTHING, ENTER_UPGRADE, UPGRADE_SINGLE, UPGRADE_ALL} Mode;

/*
 * Error Codes
 */
#define NOT_INTEL_HEX 101
#define MALFORMATED 102
#define CHECKSUM_FAILED 103

#define PAGE_FULL 201
#define WRONG_PAGE 202
#define ILLEGAL_OPERATION 203


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

/*
 * Sends the command (general call)for entering firmware upgrade during bootloader wait
 */
void send_enter_upgrade_command(int device_handle) {
	/* set device id to general call */ 
	if (ioctl(device_handle, I2C_SLAVE, 0x00) < 0) {
		fprintf(stderr, "[Error] Failed to set general call address\n");
		exit(1);
	}
	int command = FIRMWARE_UPDATE_COMMAND;
	/* Write firmware upgrade enable code */
	if (write(device_handle, &command, 1) != 1) {
		fprintf(stderr, "[Error] Transaction failed with error %s\n", strerror(errno));
		exit(1);
	}
	printf("[OK] Sent firmware upgrade command. Nodes are now awaiting hex\n");
}
	
/*
 * Sends the provided data
 */
void send_data(int device_handle, uint8_t address, char* data, int size) {
	if(!address || address > 127) {
		fprintf(stderr, "[Error] Invalid address %i\n", address);
		exit(1);

	}
	/* Set address */
	if (ioctl(device_handle, I2C_SLAVE, address) < 0) {
		fprintf(stderr, "[Error] Failed to set address %i\n", address);
		exit(1);
	}
	/* Write the data */
	if (write(device_handle, data, size) != size) {
		fprintf(stderr, "[Error] Transaction failed with error %s\n", strerror(errno));
		exit(1);
	}
	printf("[OK] Data sent succesfully \n");
}

/*
 * Opens the file specified
 */
FILE* open_file(char* filename){
	FILE* file;
	if(!(file = fopen(filename, "r" ))){
		fprintf(stderr, "Could not open the file %s\n", filename);
		exit(ENOENT);
	}
	return file;
}

/*
 * Uploads the given hex file to the node specified
 */
void upload_hex(FILE* file_handle, int device_handle, uint8_t address) {
	int i = 0;
	char line[128];
	while (fgets(line, sizeof(line), file_handle)){
		if(strlen(line) <= 2) { /* Skip (almost) empty lines */
			continue;
		}
		send: printf("[Info] Sending line %i \n", i);
		send_data(device_handle, address, line, strlen(line));
		printf("[Info] Trying to get response\n");
		/* Wait for target to reenable i2c (do the flashing) */
		int result;
		while((result = i2c_smbus_read_byte(device_handle)) == -1);
		if(result) {
			switch(result){
				case NOT_INTEL_HEX:
					fprintf(stderr, "[Error] The data is not in Intel Hex Format\n");
					exit(1);
				case MALFORMATED:
					fprintf(stderr, "[Error] The data is Malformated\n");
					exit(1);
				case CHECKSUM_FAILED:
					fprintf(stderr, "[Warning] CHECKSUM FAILED! Retrying\n");
					goto send;
				case PAGE_FULL:
					fprintf(stderr, "[Error] The provided data does not fit Layout (Page Full)\n");
					exit(1);
				case WRONG_PAGE:
					fprintf(stderr, "[Error] The data was not provided in right order\n");
					exit(1);
				case ILLEGAL_OPERATION:
					fprintf(stderr, "[Error] Other than 00 or 01 record type\n");
					exit(1);
			}
		}
		i++;
	}
}


int main(int argc, char **argv) {
	
	/* Default I2C character device */
	char* device_path = "/dev/i2c-1";
	uint8_t address = 0x00;

	/* Parse arguments */
	Mode mode = NOTHING;
	char* hexfilename;
	int c;
	while((c = getopt(argc, argv, "a:ed:f:")) != -1) {
		switch(c) {
			case 'e':
				mode = ENTER_UPGRADE;	
				break;
			case 'd':
				device_path = optarg;
				break;
			case 'a':
				address = atoi(optarg);
				mode = UPGRADE_SINGLE;
				break;
			case 'f':
				hexfilename = optarg;
				break;
		}
	}

	/* Do work according to selected mode */
	int device_handle;
	FILE* file;
	switch(mode) {
		case ENTER_UPGRADE:
			device_handle = open_i2c(device_path);
			send_enter_upgrade_command(device_handle);
			break;

		case UPGRADE_SINGLE:
			device_handle = open_i2c(device_path);
			file = open_file(hexfilename);
			upload_hex(file, device_handle, address);
			break;
	}
	if(file){
		fclose(file);
	}

	close(device_handle);
	return 0;
}
