open8610

version 2.1

Copyright 2003,2004 Kenneth Lavrsen
Copyright 2005 Grzegorz Wisniewski, Sander Eerkes
Copyright 2006 Philip Rayner
Copyright 2007 Laurent Chauvin
Copyright 2007 Philip Rayner -windows compile
Copyright 2011 Emanuele Iannone (bug fixing, write support)

This program is published under the GNU General Public License version 2.0
or later. Please read the file 'COPYING' for more info.


With open8610 you can read and write to and from a WS-8610 weather station.
You may be able to do anything - maybe even harm the station or bring it into a mode it never comes out of again.
-whilst this is unlikely you have been warned..
It is a basic adaption of the open3600 packages, which are themselves a reworking of the open2310 package.
The open2300 packages had more sub programs and features, it is maintained, but it would not talk to the ws-8610.
Open3600 did talk to the ws-8610, but the memory map was incorrect, and so taking this starting point the map has been worked out and the code altered to give temperature and humidity data for the station and additional external sensors (up to three).

The ws-8610 does hold the current time and date, but doesn't store the current reading in a fixed location, so the address position of the last reading is calculated in the memory map and it is returned as the latest reading. It is obvious that there are values for min,max, average and dewpoint readings and their times held somewhere in memory, but perhaps this is not accessible through the data download.
Once the memory is full the data is stored on a loop and this program attempts to locate the new point throughout the loop. If everything works fine, no need to reset the memory over time. When reaching the max capacity, the program will re-read from start of memory. Only potential issue I couldn't correct is on the DST change day where most probably the reading will give one hour ahead or behing depending on the case. Simplest way to correct it is to do a memory reset on that given day ;-)  Not nice, but works...

See memreset8610.c for a software attempt to reset it.
All programs are set to download the data for the number of sensors as per indicated in the memory map.

It is your choice if you want to take the risk then use it, otherwise don't.
The author takes no responsibility for any damage the use of this program
may cause.

To compile I just ran 'make' while in the source code directory which deposits object code and executable files in the code directory.
I manually moved them to the progs directory.
NB. If the file you downloaded includes .exe files they should work with windows. Linux files tested in mepis (ubuntu dapper)

dump8610
Is a special tool which will dump a range of memory into a file
AND to the screen. This makes is possible to easily see what has changed in
an area larger than 10 bytes and even use a file comparison tool to spot the
changes. Note that the WS-8610 operates with addresses of 4-bit nibbles.
When you specify an odd number of addresses the program will always add one
as it always fetches bytes from the station and displays these bytes.
If you only want a file and no display output just run it as:
dump8610 filename start end > /dev/null
If you only want to display and not create a file run:
dump8610 /dev/null start end

history8610
Read out a selected range of the history records as raw data to
both screen and file. Output is human readable.

log8610
It reads all the current data from the station
and append the data to a log file with a time stamp. The file format is
human readable text but without any labels - only with single space between
each value and a newline for each record. This makes it easy to pick up the
data using e.g. Perl or PHP for presentation on the web.



rw8610.c / rw8610.h
This is the common function library. This has been extended in so that
now you can read actual weather data using these functions without having
to think about decoding the data from the weather station.

linux8610.c / linux8610.h
This is part of the common function library and contains all the platform
unique functions. These files contains the functions that are special for
Linux.

win8610.c / win8610.h
This is part of the common function library and contains all the platform
unique functions. These files contains the functions that are special for
Windows. The most important new part is the code that handles the serial port.
As long as a windows C compiler defines a variable called WIN32 the compiler
will choose to the win8610.c and .h files when compiling.
We have used the MingW32 compiler on Windows. This is a free open source
compiler that uses the gcc compiler and makefiles.
The Windows programs have been tested on Windows XP.

memory_map.txt is a very useful information file that tells all the
currently known positions of data inside the weather station.
The information in this file may not be accurate. It is gathered by Phil Rayner
by hours of experiments. None of the information has come from the
manufacturer of the station. If you find something new please send email
to the author (email address below).
Using the memory map and the rw8610 library is it pretty easy to create
your own Linux (or windows) driven interface for your weather station.


Installing:

Just run from the program folder using a command line (DOS prompt for windows)
You should proably only want to use the log8610 or log8610echo programs. You may want a cron job to do this every 5mins.
For windows this one looks good: http://www.kalab.com/freeware/pycron/pycron.htm

How to use the programs:
All the programs now use a config file "open8610.conf" to get information like
COM port (serial device name), preferred units of dimentions,
etc etc.
Note that you should copy the open8610.conf to your preferred location

dump8610
Write address to file:	dump8610 filename start_address end_address
The addresses are simply written in hex. E.g. 21C 3A1

history8610
Write records to file:	history8610 filename start_record end_record
The addresses are simply written in hex. E.g. 1B 3A

log8610
Write current data to log interpreted: log8610 filename config_filename
This is very suitable for a cron job since it makes no output to screen.

log8610echo
same as log8610 but will return the data to the command line. I use a command called by cron 1min after each sample:

./log8610echo ws8610log open8610.conf > <apache root>/ws8610.txt

-which writes this to a one line file which can be accessed from other machines using a web browser.
-you might have to adjust file permissions to get this to work depending on your security wishes and apache setup.
For me: another server running cacti reads this file with a perl script and then graphs it.
