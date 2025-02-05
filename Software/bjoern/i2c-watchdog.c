/*
 * simple i2c wittipy watchdog
 * (C) 2025 Bjoern Riemer <bjoern.c3@nixda.biz>
 *
 * based on Simple I2C example
 * (https://github.com/shenki/linux-i2c-example/blob/master/i2c_example.c)
 *
 * Copyright 2017 Joel Stanley <joel@jms.id.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h> //usleep
#include <err.h>
#include <errno.h>

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
                                     int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}


static inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
	                     I2C_SMBUS_BYTE_DATA,&data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command, __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
				I2C_SMBUS_BYTE_DATA, &data);
}


int main(int argc, char **argv)
{
	uint8_t data, addr = 0x08, fw_id, fw_version;
	char *path = "/dev/i2c-0";
	char *shutdown_command = "/usr/sbin/poweroff";
	unsigned int check_intervall_ms = 500;
	int fd, rc, opt;


	opterr = 0;
	while ((opt = getopt (argc, argv, "i:a:c:b:")) != -1){
		switch (opt) {
		case 'c':
			shutdown_command = optarg;
			break;
		case 'b':
			path = optarg;
			break;
		case 'a':
			//errx(EXIT_FAILURE,"%c not implemented...\n",opt);
			addr = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			check_intervall_ms = atoi(optarg);
			if (!check_intervall_ms){
				errx(EXIT_FAILURE, "check intervall invalid");
			}
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-i msecs(default %i)] [-a i2c-address (default 0x%02x)] [-b %s] [-c shutdown-command ]\n", argv[0], check_intervall_ms, addr, path);
			exit(EXIT_FAILURE);
		}
	}


	fd = open(path, O_RDWR);
	if (fd < 0)
		err(errno, "Tried to open '%s'", path);

	rc = ioctl(fd, I2C_SLAVE, addr);
	if (rc < 0)
		err(errno, "Tried to set device address '0x%02x'", addr);

	if (-1 == (data = i2c_smbus_read_byte_data(fd, 0))) // 0 firmware id: 0x37 (Witty Pi 4 L3V7)
		err(errno, "Read error '%s' addr '0x%02x'\n", path, addr);
	fw_id = data;
	if (-1 == (data = i2c_smbus_read_byte_data(fd, 12))) // 12  // the firmware revision
		err(errno, "Read error '%s' addr '0x%02x'\n", path, addr);
	fw_version = data;

	if (fw_id == 0x37 && fw_version > 0x80){
		printf("WittyPi with watchdog rev 0x%02x found\n", fw_version);

		printf("sending SysUp...\n");
		if (i2c_smbus_write_byte_data(fd, 14, 1)<0)
			err(errno, "Write error '%s' addr '0x%02x' reg %d\n", path, addr, 14);

		//TODO switch on watchdog
		//TODO catch ctrl+c disable watchdog on exit
		while(1){
			if (-1 == (data = i2c_smbus_read_byte_data(fd, 14))) // 14  // flags & trigger & watchdog | read bit: 0:SYSisUP 1:WDTon? 2:turningOff? 3: 4: test-toggle
				err(errno, "Read error '%s' addr '0x%02x'\n", path, addr);

			if (data & (1<<4)){ //test mode
				printf("data 0x%02x -> bit 4 set !!\n", data);
				char test_shutdown_command[1024];
				snprintf(test_shutdown_command, sizeof(test_shutdown_command), "echo %s",shutdown_command);
				printf("exec '%s'\n", test_shutdown_command);
				system(test_shutdown_command);
			}

			if (data & 1<<2) { // check for TurningOff
				printf("turningOff bit set in WittyPi reg 14! Shutting down System...\n");
				printf("exec '%s'\n", shutdown_command);
				system(shutdown_command);
				break;
			}

			usleep(check_intervall_ms * 1000);
		}
		//data = i2c_smbus_read_byte_data(fd, reg);
		//printf("%s: device 0x%02x at address 0x%02x: 0x%02x\n",	path, addr, reg, data);
	}else{
		err(1,"fw_id[0x%02x] != 0x37 or fw_version[0x%02x] < 0x80", fw_id, fw_version);
	}

	return 0;
}