/*
 * analyze.c
 * 
 * read/write Firmware Data of bq40z60
 * 
 * uses the pigpio library
 * 
 * Copyright 2016  rricharz
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
 
#define MAXCOUNT   50		// maximum attempts on i2c bus due to limited reliablity
#define MDELAY	50000       // wait 50 msec between i2c commands

#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include <string.h>

char data[40];				// global variables
int i2cHandle;

///////////////////////////////////////////////////
int mac_read(int command, const int bytes_expected)
///////////////////////////////////////////////////
// returns the number of data bytes
{
	int i, res;
	
	for (i = 0; i < 39; i++)              // clear data array
	  data[i] = 0;
	
	data[0] = command;
	data[1] = 0;	
	
	res = i2cWriteBlockData(i2cHandle, 0x44, data, 2);
	usleep(MDELAY);
	if (res < 0) {
		// printf("i2c write command failed\n");
		return 0;
	}
	
	for (i = 0; i < 39; i++)              // clear data array
	  data[i] = 0;
	  
	res = i2cReadI2CBlockData(i2cHandle, 0x44, data, bytes_expected + 3);
	if (res < 0) {
		// printf("i2c read block failed\n");
		return 0;
	}
	if (data[0] != (bytes_expected + 2)) {
		// printf("Number of bytes returned (%d) wrong (expected = %d)\n", data[0], bytes_expected + 2);
		return 0;
	}
	if (data[1] != command) {
		// printf("Data returned does not include command\n");
		return 0;
	}
	return (data[0]);
}

/////////////////////////////////////////////////
int mac_readDump(int command, int bytes_expected)
/////////////////////////////////////////////////
// returns the number of data bytes
{
	int i, res;

	for (i = 0; i < 39; i++)              // clear data array
	  data[i] = 0;
	
	data[0] = command & 0xFF;
	data[1] = command >> 8;	
	
	res = i2cWriteBlockData(i2cHandle, 0x44, data, 2);
	if (res < 0) {
		// printf("i2c write command failed\n");
		return 0;
	}
	usleep(MDELAY);
	
	for (i = 0; i < 39; i++)              // clear data array
	  data[i] = 0;
	  
	res = i2cReadI2CBlockData(i2cHandle, 0x44, data, bytes_expected);
	if (res < 0) {
		// printf("i2c read block failed\n");
		return 0;
	}
	if (data[0] != (bytes_expected + 2)) {
		// printf("Number of bytes returned (%d) wrong (expected = %d)\n", data[0], bytes_expected + 2);
		return 0;
	}
	if ((data[1] != (command & 0xFF)) && (data[2] != (command >> 8))) {
		// printf("Data returned does not include command\n");
		return 0;
	}
	
	usleep(MDELAY);
	return (data[0]);
}

/////////////////////
int operationStatus()
/////////////////////
// returns the sec status
{
	int n, sec;
	// printf("\nReading operation status\n");
	int count = 0;
	do {
		n = mac_read(0x54, 4);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
	// if (data[0] < 32) {
	// for (i = 0; i <= data[0]; i++)
	//   printf("data[%d] = 0x%X\n", i,  data[i]); 
	// }
	sec = data[4] & 0x3;
	usleep(MDELAY);
	return (sec);
}

/////////////////////////
void display_status(void)
/////////////////////////
{
	if (data[1] != 0x54) {
		printf("Failure: cannot read operation status\n");
		return;
	}
	int sec = data[4] & 0x3;
	if (sec == 0x3)
		printf("sec = 0x3                        : SEALED\n");
	else if (sec == 0x2)
		printf("sec = 0x2                        : UNSEALED\n");
	else if (sec == 0x1)
		printf("sec = 0x1                        : FULL ACCESS\n");
	else
		printf("sec = 0x0                        : RESERVED\n");
		
	printf("Sleep mode condition met         : %d\n", (data[4] & 0x80) > 0);
	printf("Charging disabled                : %d\n", (data[4] & 0x40) > 0);
	printf("Discharging disabled             : %d\n", (data[4] & 0x20) > 0);
	printf("Permanent failure mode           : %d\n", (data[4] & 0x10) > 0);
	printf("Safety mode                      : %d\n", (data[4] & 0x08) > 0);
	printf("Shutdown triggered               : %d\n", (data[4] & 0x04) > 0);
	
	// printf("BTP_INT                          : %d\n", (data[3] & 0x80) > 0);
	printf("AC Voltage below thresh.         : %d\n", (data[3] & 0x40) > 0);
	printf("Fuse status                      : %d\n", (data[3] & 0x20) > 0);
	printf("AC FET status                    : %d\n", (data[3] & 0x10) > 0);
	printf("Pre-charge FET status            : %d\n", (data[3] & 0x08) > 0);
	printf("Charge FET status                : %d\n", (data[3] & 0x04) > 0);
	printf("Discharge FET status             : %d\n", (data[3] & 0x02) > 0);
	printf("System present                   : %d\n", (data[3] & 0x01) > 0);
	
	printf("Emergency shutdown               : %d\n", (data[6] & 0x20) > 0);
	// printf("Cell balancing                   : %d\n", (data[6] & 0x10) > 0);
	// printf("SLPCC                            : %d\n", (data[6] & 0x08) > 0);
	// printf("SLP                              : %d\n", (data[6] & 0x04) > 0);
	// printf("SMBLCAL                          : %d\n", (data[6] & 0x02) > 0);
	printf("Init after full reset            : %d\n", (data[6] & 0x01) > 0);
}

////////////////////
void printHex(int h)
////////////////////
{
	printf("%X", h >> 4);
	printf("%X", h & 0xF);
	
}

//////////////////////////////////////////////
void readWord(char *name, char *unit, int reg)
//////////////////////////////////////////////
{
	int count, res;
	count = 0;
	while (((res = i2cReadWordData(i2cHandle, reg)) < 0) && (count++ < MAXCOUNT)) usleep(MDELAY);
	printf("[0x%X] %s: ", reg, name);
	if (count >= MAXCOUNT)
		printf("Failure: cannot read register\n");
	else {
		if (reg == 0x0A) {	// convert to signed integer
			if (res > 32767)
				res -= 65536;
		}			
		if (strcmp(unit, "C") == 0)
			printf("%0.1f %s\n", ((double)res / 10.0) - 273.0, unit);
		else if (strcmp(unit, "hex") == 0) {
			printf("0x");
			printHex(res >> 8);
			printHex(res & 0xFF);
			printf("\n");
		}
		else
			printf("%d %s\n", res, unit);
	}
}


///////////////////////////////
int main(int argc, char **argv)
///////////////////////////////
{
	int count;
	
	if (gpioInitialise() < 0) {
		printf("pigpio iniialization failed\n");
		return 1;
	}
	
	i2cHandle = i2cOpen(1, 0x0b, 0);
	if (i2cHandle < 0) {
		printf("i2c open failed\n");
		gpioTerminate();
		return 1;
	}
	
	printf("\n*** pi-top battery pack analyze tool ***\n");
	printf("\nStatus [register 0x54]:\n");
	operationStatus();
	display_status();
				
	printf("\nStatus registers:\n");
	readWord("Current                    ","mA", 0x0A);
	readWord("Relative state of charge   ","%", 0x0D);
	readWord("Max err of state of charge ","%", 0x0C);
	readWord("Absolute state of charge   ","%", 0x0E);
	readWord("Design Voltage            ","mV", 0x19);
	readWord("Cell voltage 1            ","mV", 0x3F);
	readWord("Cell voltage 2            ","mV", 0x3E);
	readWord("Cell voltage 3            ","mV", 0x3D);
	readWord("Cell voltage 4            ","mV", 0x3C);
	readWord("Total voltage              ","mV", 0x09);
	readWord("Temperature                ","C", 0x08);
	readWord("Cycle count               ","cycles", 0x17);
	readWord("State of health           ","%", 0x4F);
	readWord("Remaining capacity         ","mAh", 0x0F);
	readWord("Full capacity             ","mAh", 0x10);
	readWord("Design capacity           ","mAh", 0x18);
		
	int i, n;
	count = 0;
	int bytes_to_read = 11;
	int command = 0x0002;		// Firmware version
	printf("\nFirmware [register 0x%x]:\n", command);
	do {
			n = mac_read(command, bytes_to_read);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
	if (count < MAXCOUNT) {
		for (i = 3; i <= data[0]; i++)
			printf(" 0x%X", data[i]);
		printf("\n");
		// only TI knows why this data is returned in big endian
		printf("Device number = "); printHex(data[3]); printHex(data[4]); printf("\n");
		printf("Version = %x.%x\n", data[5], data[6]);
		printf("Build number = %d\n", data[8]);
	}
	else
		printf("Failure: cannot read data from battery pack\n");
				
	int bitset = 0;	
	bytes_to_read = 4;
	command = 0x0050;		// Safety alert
	count = 0;
	printf("\nSafety alert [register 0x%x]:\n", command);
	do {
		n = mac_read(command, bytes_to_read);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
	if (count < MAXCOUNT) {
		for (i = 3; i <= data[0]; i++) {
			printf(" 0x%X", data[i]);
			bitset = bitset | data[i];
		}
		if (bitset)
			printf("**** Alert bit is set\n");
		printf("\n");		
	}
	else
		printf("Failure: cannot read data from battery pack\n");	

	bitset = 0;			
	bytes_to_read = 4;
	command = 0x0051;		// Safety status
	count = 0;
	printf("\nSafety status [register 0x%x]:\n", command);
	do {
		n = mac_read(command, bytes_to_read);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
		if (count < MAXCOUNT) {
		for (i = 3; i <= data[0]; i++) {
			printf(" 0x%X", data[i]);
			bitset = bitset | data[i]; 
		}
		if (bitset)
			printf("**** Status bit is set\n");
		printf("\n");		
	}
	else
			printf("Failure: cannot read data from battery pack\n");
		
	bitset = 0;			
	bytes_to_read = 4;
	command = 0x0052;		// Permanent failure alert
	count = 0;
	printf("\nPermanent failure alert [register 0x%x]:\n", command);
	do {
		n = mac_read(command, bytes_to_read);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
	if (count < MAXCOUNT) {
		for (i = 3; i <= data[0]; i++) {
			printf(" 0x%X", data[i]); 
			bitset = bitset | data[i];
		}
		if (bitset)
		printf("**** Alert bit is set\n");
		printf("\n");		
	}
	else
		printf("Failure: cannot read data from battery pack\n");
		
	bitset = 0;			
	bytes_to_read = 4;
	command = 0x0053;		// Permanent failure status
	count = 0;
	printf("\nPermanent failure status [register 0x%x]:\n", command);
	do {
		n = mac_read(command, bytes_to_read);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
	if (count < MAXCOUNT) {
		for (i = 3; i <= data[0]; i++) {
			printf(" 0x%X", data[i]);
			bitset = bitset | data[i];
		}
		if (bitset)
			printf("**** Status bit is set\n");
		printf("\n");		
	}
	else
		printf("Failure: cannot read data from battery pack\n");
			
	count = 0;
	printf("\nCharging status [register 0x55]:\n");
	do {
		n = mac_read(0x55, 4);
		usleep(MDELAY);
	}			
	while ((n == 0) && (count++ < MAXCOUNT));
		
	if (count < MAXCOUNT) {
		printf("Charge Termination        : %d\n", (data[3] & 0x80) > 0);
		printf("Maintenance charge        : %d\n", (data[3] & 0x40) > 0);
		printf("Charge suspend            : %d\n", (data[3] & 0x20) > 0);
		printf("Charge inhibit            : %d\n", (data[3] & 0x10) > 0);
		printf("High cell vlt charge cond.: %d\n", (data[3] & 0x08) > 0);
		printf("Med cell vlt charge cond. : %d\n", (data[3] & 0x04) > 0);
		printf("Low cell vlt charge cond. : %d\n", (data[3] & 0x02) > 0);
		printf("Pre cell vlt charge cond. : %d\n", (data[3] & 0x01) > 0);
			
	}
	else
		printf("Failure: cannot read data from battery pack\n");		

	gpioTerminate();
	return 0;

}

