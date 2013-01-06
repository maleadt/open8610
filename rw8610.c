/*  open8610  - rw8610.c library functions
 *  This is a library of functions common to Linux and Windows
 *
 *  Version 0.11
 *
 *  Control WS8610 weather station
 *
 *  Copyright 2003-2005, Kenneth Lavrsen, Grzegorz Wisniewski, Sander Eerkes
 *  2006-7 Phil Rayner & Portions by Laurent Chauvin
 *  2011 Emanuele Iannone
 *
 *  This program is published under the GNU General Public license
 */

#include "rw8610.h"

//calibration value for nanodelay function
float spins_per_ns;

//configuration data, global variable for easy availability in many function
struct config_type config;

//some constants defined as array for easy use
static int history_record_length[] = {10,13,15};
static int max_history_record[] = {
    HISTORY_BUFFER_SIZE / 10,
    HISTORY_BUFFER_SIZE / 13,
    HISTORY_BUFFER_SIZE / 15
};


/********************************************************************/
/* temperature_conv
 * Converts temperature according to configuration settings
 *
 * Input: temp_celcius - temperature in Celcius
 *
 * Returns: Temperature (deg C or F)
 *
 ********************************************************************/
double temperature_conv(double temp_celcius)
{
    if (config.temperature_conv)
        return temp_celcius * 9 / 5 + 32;
    else
        return temp_celcius;
}


/********************************************************************/
/* temp2str
 * Converts temperature into a string according to format and validity
 *
 * Input: temp - temperature, frmt - format, str - result
 *
 * Returns: pointer to updated string
 *
 ********************************************************************/
char *temp2str(double temp, char *frmt, char *str)
{
    if (temp == 81.0)
        strcpy(str, "-");
    else
        sprintf(str, frmt, temp);

    return str;
}


/********************************************************************/
/* RH2str
 * Converts humidity into a string according to format and validity
 *
 * Input: RH - humidity, frmt - format, str - result
 *
 * Returns: pointer to updated string
 *
 ********************************************************************/
char *RH2str(int RH, char *frmt, char *str)
{
    if (RH == 110)
        strcpy(str, "-");
    else
        sprintf(str, frmt, RH);

    return str;
}


/********************************************************************/
/* current_timestamp
 * Read the currently stored timestamp
 *
 * Input: Handle to weatherstation
 *
 * Returns:  seconds since 1/1/70 or -1 if read error
 ********************************************************************/
time_t current_timestamp(WEATHERSTATION ws)
{
    unsigned char tempdata[6];
    struct tm t;

    if (read_safe(ws, 0, 6, tempdata) == -1) return -1;
    t.tm_isdst = -1;
    t.tm_sec  = 0;
    t.tm_min  = (tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF);
    t.tm_hour = (tempdata[1] >> 4) * 10 + (tempdata[1] & 0xF);
    t.tm_mday = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
    t.tm_mon  = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10 - 1;
    t.tm_year = (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10 + 100;

    return mktime(&t);
}


/********************************************************************/
/* outdoor_count
 * Return the count of additional outdoor unit (1 is the minimum)
 *
 * Input: Handle to weatherstation
 *
 * Returns: 0,1 or 2 (# of additional unit) or -1 (error)
 ********************************************************************/
int outdoor_count(WEATHERSTATION ws)
{
    unsigned char tempdata[1];

    if (read_safe(ws, 0x0C, 1, tempdata) == -1) return -1;

    return (tempdata[0] - 1);
}


/********************************************************************/
/* get_history_record_length
 * Return the size of a record depending on the count of add sensors
 *
 * Input: additional outdoor sensors count
 *
 * Returns: length of corresponding record
 ********************************************************************/
int get_history_record_length(int outdoor_count)
{
    return (history_record_length[outdoor_count]);
}


/********************************************************************/
/* hist_mins
 * Read minutes from the timestamp of a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns:  minutes
 ********************************************************************/
int hist_mins(unsigned char *data)
{
    int address=0x00; /* at the start of the stored byte */
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF));
}


/********************************************************************/
/* hist_hours
 * Read hours from the timestamp of a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns:  hours
 ********************************************************************/
int hist_hours(unsigned char *data)
{
    int address=0x01; /* 1 byte on from the start of the stored byte */
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF));
}


/********************************************************************/
/* history_length
 * Read the bytes which give the no of samples held in memory
 *
 * Input: data - pointer to data buffer
 *
 * Returns:  history length
 ********************************************************************/
int history_length(unsigned char *data)
{
    int address=0x00; /* dummy val */
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF) +(tempdata[1] >> 4) * 1000 + (tempdata[1] & 0xF) * 100);
}


/********************************************************************/
/* temperature_indoor
 * Read indoor temperature from a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns: Temperature (deg C if temperature_conv is 0)
 *                      (deg F if temperature_conv is 1)
 *
 ********************************************************************/
double temperature_indoor(unsigned char *data)
{
    int address=0x05; /* 5 bytes on from the start of the stored byte */
    unsigned char *tempdata = data + address;

    return temperature_conv(((tempdata[1] & 0xF) * 10 +
                             (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);
}


/********************************************************************/
/* temperature_indoor_minmax
 * Read indoor min/max temperatures with timestamps
 *
 * Input: Handle to weatherstation
 *        temperature_conv flag (integer) controlling
 *            convertion to deg F
 *
 * Output: Temperatures temp_min and temp_max
 *                (deg C if temperature_conv is 0)
 *                (deg F if temperature_conv is 1)
 *         Timestamps for temp_min and temp_max in pointers to
 *                timestamp structures for time_min and time_max
 *
 ********************************************************************/
void temperature_indoor_minmax(unsigned char *data,
                               double *temp_min,
                               double *temp_max,
                               struct timestamp *time_min,
                               struct timestamp *time_max)
{
    int address_min=0x28;
    int address_max=0x2B;
    int address_mintime=0x2C;
    int address_maxtime=0x31;

    unsigned char *tempdata = data + address_min;

    *temp_min = temperature_conv(((tempdata[1] >> 4) * 10 + (tempdata[1] & 0xF) +
                                  (tempdata[0] >> 4) / 10.0 ) - 30.0);

    tempdata = data + address_max;
    *temp_max = temperature_conv(((tempdata[1] & 0xF) * 10 +
                                  (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);

    tempdata = data + address_mintime;
    if (time_min != NULL)
    {
        time_min->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_min->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_min->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_min->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_min->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    tempdata = data + address_maxtime;
    if (time_max != NULL)
    {
        time_max->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_max->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_max->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_max->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_max->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    return;
}


/********************************************************************/
/* temperature_outdoor
 * Read outdoor temperature from a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns: Temperature (deg C if temperature_conv is 0)
 *                      (deg F if temperature_conv is 1)
 *
 ********************************************************************/
double temperature_outdoor(unsigned char *data)
{
    int address=0x06; /* 6 bytes on from start byte of each history recrd */
    unsigned char *tempdata = data + address;

    return temperature_conv(((tempdata[1] & 0xF) +
                             (tempdata[1] >> 4) * 10 + (tempdata[0] >> 4) / 10.0) - 30.0);
}


/********************************************************************/
/* temperature_outdoor2
 * Read second outdoor temperature from a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns: Temperature (deg C if temperature_conv is 0)
 *                      (deg F if temperature_conv is 1)
 *
 ********************************************************************/
double temperature_outdoor2(unsigned char *data)
{
    int address=0x0A; /* 10 bytes on from start byte of each history recrd */
    unsigned char *tempdata = data + address;

    return temperature_conv(((tempdata[1] & 0xF) * 10 +
                             (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);
}


/********************************************************************/
/* temperature_outdoor3
 * Read third outdoor temperature from a record
 *
 * Input: data - pointer to data buffer
 *
 * Returns: Temperature (deg C if temperature_conv is 0)
 *                      (deg F if temperature_conv is 1)
 *
 ********************************************************************/
double temperature_outdoor3(unsigned char *data)
{
    int address=0x0C; /* 12 bytes on from start byte of each history recrd */
    unsigned char *tempdata = data + address;

    return temperature_conv(((tempdata[1] & 0xF) +
                             (tempdata[1] >> 4) * 10 + (tempdata[0] >> 4) / 10.0) - 30.0);
}


/********************************************************************
 * temperature_outdoor_minmax
 * Read outdoor min/max temperatures with timestamps
 *
 * Input: Handle to weatherstation
 *        temperature_conv flag (integer) controlling
 *            convertion to deg F
 *
 * Output: Temperatures temp_min and temp_max
 *                (deg C if temperature_conv is 0)
 *                (deg F if temperature_conv is 1)
 *         Timestamps for temp_min and temp_max in pointers to
 *                timestamp structures for time_min and time_max
 *
 ********************************************************************/
void temperature_outdoor_minmax(unsigned char *data,
                                double *temp_min,
                                double *temp_max,
                                struct timestamp *time_min,
                                struct timestamp *time_max)
{
    int address_min=0x3F;
    int address_max=0x42;
    int address_mintime=0x43;
    int address_maxtime=0x48;

    unsigned char *tempdata = data + address_min;

    *temp_min = temperature_conv(((tempdata[1] >> 4) * 10 + (tempdata[1] & 0xF) +
                                  (tempdata[0] >> 4) / 10.0 ) - 30.0);

    tempdata = data + address_max;
    *temp_max = temperature_conv(((tempdata[1] & 0xF) * 10 +
                                  (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);

    tempdata = data + address_mintime;
    if (time_min != NULL)
    {
        time_min->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_min->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_min->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_min->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_min->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    tempdata = data + address_maxtime;
    if (time_max != NULL)
    {
        time_max->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_max->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_max->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_max->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_max->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    return;
}

/********************************************************************
 * dewpoint
 * Read dewpoint, current value only
 *
 * Input: data - pointer to data buffer
 *
 * Returns: Dewpoint
 *
 *
 ********************************************************************/
double dewpoint(unsigned char *data)
{
    int address=0x6B;
    unsigned char *tempdata = data + address;

    return temperature_conv(((tempdata[1] & 0xF) * 10 +
                             (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);
}


/********************************************************************
 * dewpoint_minmax
 * Read outdoor min/max dewpoint with timestamps
 *
 * Input: Handle to weatherstation
 *        temperature_conv flag (integer) controlling
 *            convertion to deg F
 *
 * Output: Dewpoints dp_min and dp_max
 *                (deg C if temperature_conv is 0),
 *                (deg F if temperature_conv is 1)
 *         Timestamps for dp_min and dp_max in pointers to
 *                timestamp structures for time_min and time_max
 *
 ********************************************************************/
void dewpoint_minmax(unsigned char *data,
                     double *dp_min,
                     double *dp_max,
                     struct timestamp *time_min,
                     struct timestamp *time_max)
{
    int address_min=0x6D;
    int address_max=0x70;
    int address_mintime=0x71;
    int address_maxtime=0x76;

    unsigned char *tempdata = data + address_min;

    *dp_min = temperature_conv(((tempdata[1] >> 4) * 10 + (tempdata[1] & 0xF) +
                                (tempdata[0] >> 4) / 10.0 ) - 30.0);

    tempdata = data + address_max;
    *dp_max = temperature_conv(((tempdata[1] & 0xF) * 10 +
                                (tempdata[0] >> 4) + (tempdata[0] & 0xF) / 10.0) - 30.0);

    tempdata = data + address_mintime;
    if (time_min != NULL)
    {
        time_min->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_min->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_min->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_min->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_min->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    tempdata = data + address_maxtime;
    if (time_max != NULL)
    {
        time_max->minute = (tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10;
        time_max->hour = (tempdata[1] >> 4) + (tempdata[2] & 0xF) * 10;
        time_max->day = (tempdata[2] >> 4) + (tempdata[3] & 0xF) * 10;
        time_max->month = (tempdata[3] >> 4) + (tempdata[4] & 0xF) * 10;
        time_max->year = 2000 + (tempdata[4] >> 4) + (tempdata[5] & 0xF) * 10;
    }

    return;
}

/********************************************************************
 * humidity_indoor
 * Read indoor relative humidity, current value only
 *
 * Input: Handle to weatherstation
 * Returns: relative humidity in percent (integer)
 *
 ********************************************************************/
int humidity_indoor(unsigned char *data)
{
    int address=0x08;
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF));
}


/********************************************************************
 * humidity_indoor_minmax
 * Read min/max values with timestamps
 *
 * Input: Handle to weatherstation
 * Output: Relative humidity in % hum_min and hum_max (integers)
 *         Timestamps for hum_min and hum_max in pointers to
 *                timestamp structures for time_min and time_max
 * Returns: releative humidity current value in % (integer)
 *
 ********************************************************************/
void humidity_indoor_minmax(unsigned char *data,
                            int *hum_min,
                            int *hum_max,
                            struct timestamp *time_min,
                            struct timestamp *time_max)
{
    int address_min = 0x82;
    int address_max = 0x83;
    int address_mintime = 0x84;
    int address_maxtime = 0x89;
    unsigned char *tempdata = data + address_min;

    if (hum_min != NULL)
        *hum_min = (tempdata[0] >> 4) * 10  + (tempdata[0] & 0xF);

    tempdata = data + address_max;
    if (hum_max != NULL)
        *hum_max = (tempdata[0] >> 4) * 10  + (tempdata[0] & 0xF);

    tempdata = data + address_mintime;
    if (time_min != NULL)
    {
        time_min->minute = ((tempdata[0] >> 4) * 10) + (tempdata[0] & 0xF);
        time_min->hour = ((tempdata[1] >> 4) * 10) + (tempdata[1] & 0xF);
        time_min->day = ((tempdata[2] >> 4) * 10) + (tempdata[2] & 0xF);
        time_min->month = ((tempdata[3] >> 4) * 10) + (tempdata[3] & 0xF);
        time_min->year = 2000 + ((tempdata[4] >> 4) * 10) + (tempdata[4] & 0xF);
    }

    tempdata = data + address_maxtime;
    if (time_max != NULL)
    {
        time_max->minute = ((tempdata[0] >> 4) * 10) + (tempdata[0] & 0xF);
        time_max->hour = ((tempdata[1] >> 4) * 10) + (tempdata[1] & 0xF);
        time_max->day = ((tempdata[2] >> 4) * 10) + (tempdata[2] & 0xF);
        time_max->month = ((tempdata[3] >> 4) * 10) + (tempdata[3] & 0xF);
        time_max->year = 2000 + ((tempdata[4] >> 4) * 10) + (tempdata[4] & 0xF);
    }

    return;
}

/********************************************************************
 * humidity_outdoor
 * Read relative humidity, current value only
 *
 * Input: data - pointer to data buffer
 * Returns: relative humidity in percent (integer)
 *
 ********************************************************************/
int humidity_outdoor(unsigned char *data)
{
    int address=0x09;
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF));
}


/********************************************************************
 * humidity_outdoor2
 * Read relative humidity, current value only
 *
 * Input: data - pointer to data buffer
 * Returns: relative humidity in percent (integer)
 *
 ********************************************************************/
int humidity_outdoor2(unsigned char *data)
{
    int address=0x0B;
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) + (tempdata[1] & 0xF) * 10);
}


/********************************************************************
 * humidity_outdoor3
 * Read relative humidity, current value only
 *
 * Input: data - pointer to data buffer
 * Returns: relative humidity in percent (integer)
 *
 ********************************************************************/
int humidity_outdoor3(unsigned char *data)
{
    int address=0x0E;
    unsigned char *tempdata = data + address;

    return ((tempdata[0] >> 4) * 10 + (tempdata[0] & 0xF));
}


/********************************************************************
 * humidity_outdoor_minmax
 * Read min/max values with timestamps
 *
 * Input: Handle to weatherstation
 * Output: Relative humidity in % hum_min and hum_max (integers)
 *         Timestamps for hum_min and hum_max in pointers to
 *                timestamp structures for time_min and time_max
 *
 * Returns: releative humidity current value in % (integer)
 *
 ********************************************************************/
void humidity_outdoor_minmax(unsigned char *data,
                             int *hum_min,
                             int *hum_max,
                             struct timestamp *time_min,
                             struct timestamp *time_max)
{
    int address_min = 0x91;
    int address_max = 0x92;
    int address_mintime = 0x93;
    int address_maxtime = 0x98;
    unsigned char *tempdata = data + address_min;

    if (hum_min != NULL)
        *hum_min = (tempdata[0] >> 4) * 10  + (tempdata[0] & 0xF);

    tempdata = data + address_max;
    if (hum_max != NULL)
        *hum_max = (tempdata[0] >> 4) * 10  + (tempdata[0] & 0xF);

    tempdata = data + address_mintime;
    if (time_min != NULL)
    {
        time_min->minute = ((tempdata[0] >> 4) * 10) + (tempdata[0] & 0xF);
        time_min->hour = ((tempdata[1] >> 4) * 10) + (tempdata[1] & 0xF);
        time_min->day = ((tempdata[2] >> 4) * 10) + (tempdata[2] & 0xF);
        time_min->month = ((tempdata[3] >> 4) * 10) + (tempdata[3] & 0xF);
        time_min->year = 2000 + ((tempdata[4] >> 4) * 10) + (tempdata[4] & 0xF);
    }

    tempdata = data + address_maxtime;
    if (time_max != NULL)
    {
        time_max->minute = ((tempdata[0] >> 4) * 10) + (tempdata[0] & 0xF);
        time_max->hour = ((tempdata[1] >> 4) * 10) + (tempdata[1] & 0xF);
        time_max->day = ((tempdata[2] >> 4) * 10) + (tempdata[2] & 0xF);
        time_max->month = ((tempdata[3] >> 4) * 10) + (tempdata[3] & 0xF);
        time_max->year = 2000 + ((tempdata[4] >> 4) * 10) + (tempdata[4] & 0xF);
    }

    return;
}

/********************************************************************
 * read_history_record
 * Read the history information like time
 * of last record, pointer to last record.
 *
 * Input:  Handle to weatherstation
 *         record - record index number to be read [0x00-0xAE]
 *         hr - pointer to history_record structure
 *         outdoor_count - count of additional external sensor
 *
 * Output: history record
 *
 ********************************************************************/
int read_history_record(WEATHERSTATION ws, int record_no, struct history_record *hr, int outdoor_count) {
    unsigned char record[16];
    struct tm t;

    while (record_no >= max_history_record[outdoor_count]) record_no -= max_history_record[outdoor_count];

    if (read_safe(ws, HISTORY_BUFFER_ADR + history_record_length[outdoor_count] * record_no,
                  history_record_length[outdoor_count], record) != history_record_length[outdoor_count])
        read_error_exit();
    else {
        int i;
        char str[256];

        sprintf(str, "%d additional sensor(s), record length is %d, max record count is %d", outdoor_count,
                history_record_length[outdoor_count], max_history_record[outdoor_count]);
        print_log(2, str);
        sprintf(str, "Reading record %d at 0x%x: ", record_no,
                HISTORY_BUFFER_ADR + history_record_length[outdoor_count] * record_no);
        for (i = 0; i < history_record_length[outdoor_count]; i++)
            sprintf(str, "%s%02X ", str, record[i]);
        print_log(2, str);
    }

    t.tm_isdst = -1;
    t.tm_sec  = 0;
    t.tm_min  = (record[0] >> 4) * 10 + (record[0] & 0xF);
    t.tm_hour = (record[1] >> 4) * 10 + (record[1] & 0xF);
    t.tm_mday = (record[2] >> 4) * 10 + (record[2] & 0xF);
    t.tm_mon  = (record[3] >> 4) * 10 + (record[3] & 0xF) - 1;
    t.tm_year = (record[4] >> 4) * 10 + (record[4] & 0xF) + 100;
    hr->time_stamp = mktime(&t);

    hr->Temp[0] = temperature_indoor(record);
    hr->Temp[1] = temperature_outdoor(record);
    hr->Temp[2] = temperature_outdoor2(record);
    hr->Temp[3] = temperature_outdoor3(record);

    hr->RH[0] = humidity_indoor(record);
    hr->RH[1] = humidity_outdoor(record);
    hr->RH[2] = humidity_outdoor2(record);
    hr->RH[3] = humidity_outdoor3(record);

    return 0;
}


/********************************************************************
 * read_last_history_record
 * Read the last history record
 *
 * Input:  Handle to weatherstation
 *         hr - pointer to history_record structure
 *         outdoor_count - count of additional external sensor
 *
 * Output: last history record
 *
 ********************************************************************/
int read_last_history_record(WEATHERSTATION ws, struct history_record *hr, int outdoor_count) {
    time_t current_t;
    int last_record_no;
    unsigned char n_record;
    char str[256];

    read_history_record(ws, 0, hr, outdoor_count);
    current_t = current_timestamp(ws);
    last_record_no = (current_t - hr->time_stamp) / 300;

    sprintf(str, "Diff: %d records", last_record_no);
    print_log(2, str);
    sprintf(str, "Date record0: %s", ctime(&(hr->time_stamp)));
    str[strlen(str) - 1] = 0;
    print_log(2, str);
    sprintf(str, "Date current: %s", ctime(&current_t));
    str[strlen(str) - 1] = 0;
    print_log(2, str);

    // Try to see if record (n+1) is valid
    if (read_safe(ws, HISTORY_BUFFER_ADR + history_record_length[outdoor_count] * (last_record_no +1), 1, &n_record) != 1)
        read_error_exit();

    sprintf(str, "Next record start with 0x%02X", (int) n_record);

    // If valid then read one record ahead
    if (n_record != 0xFF) {
        sprintf(str, "%s, seems valid, skipping to it", str);
        last_record_no++;
    } else sprintf(str, "%s, stick with current record", str);
    print_log(2, str);

    return read_history_record(ws, last_record_no, hr, outdoor_count);
}


/********************************************************************
 * read_error_exit
 * exit location for all calls to read_safe for error exit.
 * includes error reporting.
 *
 ********************************************************************/
void read_error_exit(void)
{
    perror("read_safe() error");
    exit(0);
}


/********************************************************************
 * write_error_exit
 * exit location for all calls to write_safe for error exit.
 * includes error reporting.
 *
 ********************************************************************/
void write_error_exit(void)
{
    perror("write_safe() error");
    exit(0);
}


/********************************************************************
 * get_configuration()
 *
 * read setup parameters from ws8610.conf
 * It searches in this sequence:
 * 1. Path to config file including filename given as parameter
 * 2. ./open8610.conf
 * 3. /usr/local/etc/open8610.conf
 * 4. /etc/open8610.conf
 *
 * See file open8610.conf-dist for the format and option names/values
 *
 * input:    config file name with full path - pointer to string
 *
 * output:   struct config populated with valid settings either
 *           from config file or defaults
 *
 * returns:  0 = OK
 *          -1 = no config file or file open error
 *
 ********************************************************************/
int get_configuration(struct config_type *config, char *path)
{
    FILE *fptr;
    char inputline[1000] = "";
    char token[100] = "";
    char val[100] = "";
    char val2[100] = "";

    // First we set everything to defaults - faster than many if statements
    strcpy(config->serial_device_name, DEFAULT_SERIAL_DEVICE);  // Name of serial device
    strcpy(config->citizen_weather_id, "CW0000");               // Citizen Weather ID
    strcpy(config->citizen_weather_latitude, "5540.12N");       // latitude default Glostrup, DK
    strcpy(config->citizen_weather_longitude, "01224.60E");     // longitude default, Glostrup, DK
    strcpy(config->aprs_host[0].name, "aprswest.net");         // host1 name
    config->aprs_host[0].port = 23;                            // host1 port
    strcpy(config->aprs_host[1].name, "indiana.aprs2.net");    // host2 name
    config->aprs_host[1].port = 23;                            // host2 port
    config->num_hosts = 2;                                     // default number of defined hosts
    strcpy(config->weather_underground_id, "WUID");             // Weather Underground ID
    strcpy(config->weather_underground_password, "WUPassword"); // Weather Underground Password
    strcpy(config->timezone, "1");                              // Timezone, default CET
    config->wind_speed_conv_factor = 1.0;                   // Speed dimention, m/s is default
    config->temperature_conv = 0;                           // Temperature in Celcius
    config->rain_conv_factor = 1.0;                         // Rain in mm
    config->pressure_conv_factor = 1.0;                     // Pressure in hPa (same as millibar)
    strcpy(config->mysql_host, "localhost");            // localhost, IP or domainname of server
    strcpy(config->mysql_user, "open8610");             // MySQL database user name
    strcpy(config->mysql_passwd, "mysql8610");          // Password for MySQL database user
    strcpy(config->mysql_database, "open8610");         // Name of MySQL database
    config->mysql_port = 0;                             // MySQL port. 0 means default port/socket
    strcpy(config->pgsql_connect, "hostaddr='127.0.0.1'dbname='open8610'user='postgres'"); // connection string
    strcpy(config->pgsql_table, "weather");             // PgSQL table name
    strcpy(config->pgsql_station, "open8610");          // Unique station id
    config->log_level = 0;

    // open the config file

    fptr = NULL;
    if (path != NULL)
        fptr = fopen(path, "r");       //first try the parameter given
    if (fptr == NULL)                  //then try default search
    {
        if ((fptr = fopen("open8610.conf", "r")) == NULL)
        {
            if ((fptr = fopen("/usr/local/etc/open8610.conf", "r")) == NULL)
            {
                if ((fptr = fopen("/etc/open8610.conf", "r")) == NULL)
                {
                    //Give up and use defaults
                    return(-1);
                }
            }
        }
    }

    while (fscanf(fptr, "%[^\n]\n", inputline) != EOF)
    {
        sscanf(inputline, "%[^= \t]%*[ \t=]%s%*[, \t]%s%*[^\n]", token, val, val2);

        if (token[0] == '#')	// comment
            continue;

        if ((strcmp(token,"SERIAL_DEVICE")==0) && (strlen(val) != 0))
        {
            strcpy(config->serial_device_name,val);
            continue;
        }

        if ((strcmp(token,"CITIZEN_WEATHER_ID")==0) && (strlen(val) != 0))
        {
            strcpy(config->citizen_weather_id, val);
            continue;
        }

        if ((strcmp(token,"CITIZEN_WEATHER_LATITUDE")==0) && (strlen(val)!=0))
        {
            strcpy(config->citizen_weather_latitude, val);
            continue;
        }

        if ((strcmp(token,"CITIZEN_WEATHER_LONGITUDE")==0) && (strlen(val)!=0))
        {
            strcpy(config->citizen_weather_longitude, val);
            continue;
        }

        if ((strcmp(token,"APRS_SERVER")==0) && (strlen(val)!=0) && (strlen(val2)!=0))
        {
            if ( config->num_hosts >= MAX_APRS_HOSTS)
                continue;           // ignore host definitions over the defined max
            strcpy(config->aprs_host[config->num_hosts].name, val);
            config->aprs_host[config->num_hosts].port = atoi(val2);
            config->num_hosts++;    // increment for next
            continue;
        }

        if ((strcmp(token,"WEATHER_UNDERGROUND_ID")==0) && (strlen(val)!=0))
        {
            strcpy(config->weather_underground_id, val);
            continue;
        }

        if ((strcmp(token,"WEATHER_UNDERGROUND_PASSWORD")==0)&&(strlen(val)!=0))
        {
            strcpy(config->weather_underground_password, val);
            continue;
        }

        if ((strcmp(token,"TIMEZONE")==0) && (strlen(val) != 0))
        {
            strcpy(config->timezone, val);
            continue;
        }

        if ((strcmp(token,"WIND_SPEED") == 0) && (strlen(val) != 0))
        {
            if (strcmp(val, "m/s") == 0)
                config->wind_speed_conv_factor = METERS_PER_SECOND;
            else if (strcmp(val, "km/h") == 0)
                config->wind_speed_conv_factor = KILOMETERS_PER_HOUR;
            else if (strcmp(val, "MPH") == 0)
                config->wind_speed_conv_factor = MILES_PER_HOUR;
            continue; //else default remains
        }

        if ((strcmp(token,"TEMPERATURE") == 0) && (strlen(val) != 0))
        {
            if (strcmp(val, "C") == 0)
                config->temperature_conv = CELCIUS;
            else if (strcmp(val, "F") == 0)
                config->temperature_conv = FAHRENHEIT;
            continue; //else default remains
        }

        if ((strcmp(token,"RAIN") == 0) && (strlen(val) != 0))
        {
            if (strcmp(val, "mm") == 0)
                config->rain_conv_factor = MILLIMETERS;
            else if (strcmp(val, "IN") == 0)
                config->rain_conv_factor = INCHES;
            continue; //else default remains
        }

        if ((strcmp(token,"PRESSURE") == 0) && (strlen(val) != 0))
        {
            if ( (strcmp(val, "hPa") == 0) || (strcmp(val, "mb") == 0))
                config->pressure_conv_factor = HECTOPASCAL;
            else if (strcmp(val, "INHG") == 0)
                config->pressure_conv_factor = INCHES_HG;
            continue; //else default remains
        }

        if ((strcmp(token,"MYSQL_HOST") == 0) && (strlen(val) != 0))
        {
            strcpy(config->mysql_host, val);
            continue;
        }

        if ( (strcmp(token,"MYSQL_USERNAME") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->mysql_user, val);
            continue;
        }

        if ( (strcmp(token,"MYSQL_PASSWORD") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->mysql_passwd, val);
            continue;
        }

        if ( (strcmp(token,"MYSQL_DATABASE") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->mysql_database, val);
            continue;
        }

        if ( (strcmp(token,"MYSQL_PORT") == 0) && (strlen(val) != 0) )
        {
            config->mysql_port = atoi(val);
            continue;
        }

        if ( (strcmp(token,"PGSQL_CONNECT") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->pgsql_connect, val);
            continue;
        }

        if ( (strcmp(token,"PGSQL_TABLE") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->pgsql_table, val);
            continue;
        }

        if ( (strcmp(token,"PGSQL_STATION") == 0) && (strlen(val) != 0) )
        {
            strcpy(config->pgsql_station, val);
            continue;
        }

        if ((strcmp(token,"LOG_LEVEL")==0) && (strlen(val)!=0))
        {
            config->log_level = atoi(val);
            continue;
        }
    }

    return (0);
}

/********************************************************************
 * read_data reads data from the WS8610 based on a given address,
 * number of data read, and a an already open serial port
 *
 * Inputs:  ws - device number of the already open serial port
 *          number - number of bytes to read
 *
 * Output:  readdata - pointer to an array of chars containing
 *                     the just read data, not zero terminated
 *
 * Returns: number of bytes read, -1 if failed
 *
 ********************************************************************/
int read_data(WEATHERSTATION ws, int number, unsigned char *readdata)
{
    unsigned char command = 0xa1;
    int i;

    if (write_byte(ws, command, 1))
    {
        for (i = 0; i < number; i++)
        {
            readdata[i] = read_byte(ws);
            if (i + 1 < number)
                read_next_byte_seq(ws);
            //printf("%i\n",readdata[i]);
        }

        read_last_byte_seq(ws);

        return i;
    } else
        return -1;
}

/********************************************************************
 * write_data writes data to the WS2300.
 * It can both write nibbles and set/unset bits
 *
 * Inputs:      ws - device number of the already open serial port
 *              address (interger - 16 bit)
 *              number - number of nibbles to be written/changed
 *                       must 1 for bit modes (SETBIT and UNSETBIT)
 *                       max 80 for nibble mode (WRITENIB)
 *              writedata - pointer to an array of chars containing
 *                          data to write, not zero terminated
 *                          data must be in hex - one digit per byte
 *                          If bit mode value must be 0-3 and only
 *                          the first byte can be used.
 *
 * Output:      commanddata - pointer to an array of chars containing
 *                            the commands that were sent to the station
 *
 * Returns:     number of bytes written, -1 if failed
 *
 ********************************************************************/
int write_data(WEATHERSTATION ws, short address, int number, unsigned char *writedata)
{
    unsigned char command = 0xa0;
    int i = 1;
    int c, status;

    write_byte(ws, command, 1);
    write_byte(ws, address/256, 1);
    write_byte(ws, address%256, 1);

    if (writedata!=NULL) {
        for (i = 0; i < number; i++) write_byte(ws, writedata[i], 1);
        set_RTS(ws,1);
        nanodelay(DELAY_CONST);
        set_DTR(ws,0);
        nanodelay(DELAY_CONST);
        set_RTS(ws,0);
        nanodelay(DELAY_CONST);

        for (c = 0; c < 3; c++) write_byte(ws, command, 0);
        set_DTR(ws,0);
        nanodelay(DELAY_CONST);
        status = get_CTS(ws);
        if (status == 0) i = -1;
        nanodelay(DELAY_CONST);
        set_DTR(ws,1);
        nanodelay(DELAY_CONST);
    }
    else {
        set_DTR(ws,0);
        nanodelay(DELAY_CONST);
        set_RTS(ws,0);
        nanodelay(DELAY_CONST);
        set_RTS(ws,1);
        nanodelay(DELAY_CONST);
        set_DTR(ws,1);
        nanodelay(DELAY_CONST);
        set_RTS(ws,0);
        nanodelay(DELAY_CONST);
    }

//return -1 for errors
    return i;
}


/********************************************************************
 * read_safe Read data, retry until success or maxretries
 * Reads data from the WS8610 based on a given address,
 * number of data read, and a an already open serial port
 * Uses the read_data function and has same interface
 *
 * Inputs:  ws - device number of the already open serial port
 *          address (interger - 16 bit)
 *          number - number of bytes to read
 *
 * Output:  readdata - pointer to an array of chars containing
 *                     the just read data, not zero terminated
 *
 * Returns: number of bytes read, -1 if failed
 *
 ********************************************************************/
int read_safe(WEATHERSTATION ws, short address, int number, unsigned char *readdata)
{
    int i,j;
    unsigned char readdata2[32768];

    print_log(1,"read_safe");

    for (j = 0; j < MAXRETRIES; j++)
    {
        write_data(ws, address, 0, NULL);
        read_data(ws, number, readdata);

        write_data(ws, address, 0, NULL);
        read_data(ws, number, readdata2);

        if (memcmp(readdata,readdata2,number) == 0)
        {
            //check if only 0's for reading memory range greater then 10 bytes
            print_log(2,"read_safe - two readings identical");
            i = 0;
            if (number > 10)
            {
                for (; readdata[i] == 0 && i < number; i++);
            }

            if (i != number)
                break;
            else
                print_log(2,"read_safe - only zeros");
        } else
            print_log(2,"read_safe - two readings not identical");
    }

    // If we have tried MAXRETRIES times to read we expect not to
    // have valid data
    if (j == MAXRETRIES)
    {
        return -1;
    }

    return number;
}

void read_next_byte_seq(WEATHERSTATION ws)
{
    print_log(3,"read_next_byte_seq");
    write_bit(ws,0);
    set_RTS(ws,0);
    nanodelay(DELAY_CONST);
}

void read_last_byte_seq(WEATHERSTATION ws)
{
    print_log(3,"read_last_byte_seq");
    set_RTS(ws,1);
    nanodelay(DELAY_CONST);
    set_DTR(ws,0);
    nanodelay(DELAY_CONST);
    set_RTS(ws,0);
    nanodelay(DELAY_CONST);
    set_RTS(ws,1);
    nanodelay(DELAY_CONST);
    set_DTR(ws,1);
    nanodelay(DELAY_CONST);
    set_RTS(ws,0);
    nanodelay(DELAY_CONST);
}

/********************************************************************
 * read_bit
 * Reads one bit from the COM
 *
 * Inputs:  serdevice - opened file handle
 *
 * Returns: bit read from the COM
 *
 ********************************************************************/

int read_bit(WEATHERSTATION ws)
{
    int status;
    char str[20];

    print_log(4, "Read bit...");
    set_DTR(ws,0);
    nanodelay(DELAY_CONST);
    status = get_CTS(ws);
    nanodelay(DELAY_CONST);
    set_DTR(ws,1);
    nanodelay(DELAY_CONST);
    sprintf(str, "bit = %i",!status);
    print_log(4,str);

    return !status;
}

/********************************************************************
 * write_bit
 * Writes one bit to the COM
 *
 * Inputs:  serdevice - opened file handle
 *          bit - bit to write
 *
 * Returns: nothing
 *
 ********************************************************************/
void write_bit(WEATHERSTATION ws,short bit)
{
    char str[20];
    int val = 0;

    if (bit) val = 1;
    sprintf(str, "Write bit %i", val);
    print_log(4,str);
    set_RTS(ws,!bit);
    nanodelay(DELAY_CONST);
    set_DTR(ws,0);
    nanodelay(DELAY_CONST);
    set_DTR(ws,1);
}



/********************************************************************
 * read_byte
 * Reads one byte from the COM
 *
 * Inputs:  serdevice - opened file handle
 *
 * Returns: byte read from the COM
 *
 ********************************************************************/
unsigned char read_byte(WEATHERSTATION serdevice)
{
    unsigned char byte = 0;
    int i;
    char str[20];

    print_log(3, "Read byte...");
    for (i = 0; i < 8; i++)
    {
        byte *= 2;
        byte += read_bit(serdevice);
    }
    sprintf(str, "byte = %X", byte);
    print_log(3, str);

    return byte;
}

/********************************************************************
 * write_byte
 * Writes one byte to the COM
 *
 * Inputs:  serdevice - opened file handle
 *          byte - byte to write
 *          check_value 1 to check value written
 *
 * Returns: nothing
 *
 ********************************************************************/
int write_byte(WEATHERSTATION ws, unsigned char byte, int check_value)
{
    int status = 1;
    int i;
    char str[20];

    sprintf(str, "Write byte %X", byte);
    print_log(3,str);

    for (i = 0; i < 8; i++)
    {
        write_bit(ws, byte & 0x80);
        byte <<= 1;
        byte &= 0xff;
    }

    set_RTS(ws,0);
    nanodelay(DELAY_CONST);
    if (check_value == 1) {
        status = get_CTS(ws);
        //TODO: checking value of status, error routine
        nanodelay(DELAY_CONST);
        set_DTR(ws,0);
        nanodelay(DELAY_CONST);
        set_DTR(ws,1);
        nanodelay(DELAY_CONST);
    }
    if (status)
        return 1;
    else
        return 0;
}

/********************************************************************
 * calculate_dewpoint
 * Calculates dewpoint value
 * REF http://www.faqs.org/faqs/meteorology/temp-dewpoint/
 *
 * Inputs:  temperature  in Celcius
 *          humidity
 *
 * Returns: dewpoint
 *
 ********************************************************************/
double calculate_dewpoint(double temperature, double humidity)
{
    double A, B, C;
    double dewpoint;

    A = 17.2694;
    B = (temperature > 0) ? 237.3 : 265.5;
    C = (A * temperature)/(B + temperature) + log((double)humidity/100);
    dewpoint = B * C / (A - C);

    return dewpoint;
}


void print_log(int log_level, char* str)
{
    if (log_level <= config.log_level)
        fprintf(stderr,"%s\n",str);
}
