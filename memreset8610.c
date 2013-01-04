/*  open8610 - memreset8610.c
 *  
 *  Version 1.1
 *  
 *  Control WS8610 weather station
 *  
 *  reworked from work Copyright 2003-2005, Kenneth Lavrsen
 *  Copyright 2006 Phil Rayner
 *  Copyright 2011 Emanuele Iannone
 *  This program is published under the GNU General Public license
 */

#include "rw8610.h"

/********************************************************************
 * print_usage prints a short user guide
 *
 * Input:   none
 * 
 * Output:  prints to stdout
 * 
 * Returns: exits program
 *
 ********************************************************************/
void print_usage(void)
{
	printf("\n");
	printf("memreset8610 - wipes the history of a WS-8610 weather station\n");
	printf("Good to call when memory is full to prevent memory looping.\n");
	printf("Version 0.1 (C)2006 Phil Rayner.\n");
	printf("This program is released under the GNU General Public License (GPL)\n\n");
	printf("Usage: ./memreset8610 enable config_filename\n");
	printf("Set 'enable' to 1 to reset memory, otherwise a dry run.\n");
	printf("It writes 0 to addresses holding pointers for no of readings held\n");
	printf("-its obviously DANGEROUS to the data on your ws8610\n");
	exit(0);
}
 
/********** MAIN PROGRAM ************************************************
 *
 * Resets the memory on the ws8610
 * The first argument is a safety swtich 1 for go otherwise stop
 * The second argument is the config file name with path.
 * If this parameter is omitted the program will look at the default paths
 * See the open8610.conf-dist file for info
 *
 ***********************************************************************/
int main(int argc, char *argv[])
{
	WEATHERSTATION ws;
	struct config_type config;
	unsigned char data[2];
	int enable;
	unsigned char records[128];

	if (argc < 2 || argc > 3)
	{
		print_usage();
	}			

	get_configuration(&config, argv[2]);
	ws = open_weatherstation(config.serial_device_name);

	enable = (int)strtol(argv[1],NULL,10);

	if (enable == 1)
	{
		printf("Wiping data history from the ws8610\n");
		data[0] = 0x80;
		data[1] = 0x02;
		write_data(ws, 0x0009, 2, data);
	}

	read_safe(ws, 0x0009, 2, records);

	printf("Data wiped if enable was used. Enable= %d\n", enable);
	printf("Number of valid records now is %d\n", history_length(records));

	close_weatherstation(ws);
	
	return (0);
}

