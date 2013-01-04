/*  open8610 - log8610echo.c
 *  
 *  Version 1.10
 *  
 *  Control WS8610 weather station
 *  
 *  Copyright 2003-2005, Kenneth Lavrsen, Grzegorz Wisniewski, Sander Eerkes
 *  2006-7 Phil Rayner, Laurent Chauvin
 *
 *  This program is published under the GNU General Public license
 */

#include "rw8610.h"

/********************************************************************
 * print_usage prints a short user guide
 *
 * Input:   none
 * 
 * Output:  prints to stdout (unlike log8610 which only goes to file)
 * 
 * Returns: exits program
 *
 ********************************************************************/
void print_usage(void)
{
	printf("\n");
	printf("log8610echo - Read and interpret data from WS-8610 weather station,\n");
	printf("write it to a log file and to stdout. Perfect for a cron driven task.\n");
	printf("(C)2003 Kenneth Lavrsen.\n");
	printf("(C)2005 Grzegorz Wisniewski,Sander Eerkes. (Version alfa)\n");
	printf("(C)2006-7 Phil Rayner.\n");
	printf("(C)2007 Laurent Chauvin.\n");
	printf("This program is released under the GNU General Public License (GPL)\n\n");
	printf("Usage:\n");
	printf("Save current data to logfile:  log8610echo filename config_filename\n");
	exit(0);
}
 
/********** MAIN PROGRAM ************************************************
 *
 * This program reads current weather data from a WS8610
 * and writes the data to a log file. There's a little difficulty finding it
 * since only history is saved to memory and no value yet appears to have 
 * current reading data
 *
 * Just run the program without parameters for usage.
 *
 * It takes two parameters. The first is the log filename with path
 * The second is the config file name with path
 * If this parameter is omitted the program will look at the default paths
 * See the open8610.conf file for info
 *
 ***********************************************************************/
int main(int argc, char *argv[])
{
	WEATHERSTATION ws;
	FILE *fileptr;
	unsigned char logline[1024] = "";
	char str[50];
	char datestring[100];        //used to hold the date stamp for the log file
	time_t basictime;
	struct history_record hr;
	int o_count;

	get_configuration(&config, argv[2]);

   /* Get log filename. */

	if (argc < 2 || argc > 3)
	{
		print_usage();
	}			

	fileptr = fopen(argv[1], "a+");
	if (fileptr == NULL)
	{
		printf("Cannot open file %s\n",argv[1]);
		exit(-1);
	}
  
	ws = open_weatherstation(config.serial_device_name);

	if ((o_count = outdoor_count(ws)) == -1)
        {
                printf("Cannot get count of outdoor sensors\n");
		fclose(fileptr);
		close_weatherstation(ws);
                exit(-1);
        }

	read_last_history_record(ws, &hr, o_count);

	close_weatherstation(ws);


	/* READ TEMPERATURE INDOOR */
	sprintf(logline,"%sTi: %s | ", logline, temp2str(hr.Temp[0], "%.1f", str));
	
        /* READ RELATIVE HUMIDITY INDOOR */
        sprintf(logline,"%sHi: %s | ", logline, RH2str(hr.RH[0], "%d", str));

	/* READ TEMPERATURE OUTDOOR */
	sprintf(logline,"%sTo1: %s | ", logline, temp2str(hr.Temp[1], "%.1f", str));
	
	/* READ RELATIVE HUMIDITY OUTDOOR */
	sprintf(logline,"%sHo1: %s | ", logline, RH2str(hr.RH[1], "%d", str));

        /* READ SECOND TEMPERATURE OUTDOOR */
        sprintf(logline,"%sTo2: %s | ", logline, temp2str(hr.Temp[2], "%.1f", str));

        /* READ SECOND RELATIVE HUMIDITY OUTDOOR */
        sprintf(logline,"%sHo2: %s | ", logline, RH2str(hr.RH[2], "%d", str));

        /* READ THIRD TEMPERATURE OUTDOOR */
        sprintf(logline,"%sTo3: %s | ", logline, temp2str(hr.Temp[3], "%.1f", str));

        /* READ THIRD RELATIVE HUMIDITY OUTDOOR */
        sprintf(logline,"%sHo3: %s | ", logline, RH2str(hr.RH[3], "%d", str));

	/* GET TIME STAMP OF LAST HISTORY RECORD */
	sprintf(logline,"%s%s", logline, ctime(&hr.time_stamp));

	/* GET DATE AND TIME FOR LOG FILE, PLACE BEFORE ALL DATA IN LOG LINE */
	time(&basictime);
	strftime(datestring,sizeof(datestring),"%d/%m %H:%M:%S |",
	         localtime(&basictime));

	// Print out and leave
	printf("%s %s",datestring, logline); /*dumps it out to the command line */
	fprintf(fileptr,"%s %s",datestring, logline);
	
	fclose(fileptr);

	exit(0);
}
