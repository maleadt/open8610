####### Makefile for open8610 - manually created based on open2300 makefile
#
# # Default locations of config file are 
# 1. Path to config file including filename given as parameter
# 2. ./open8610.conf
# 3. /usr/local/etc/open8610.conf
# 4. /etc/open8610.conf
#
# You may want to adjust the 3 directories below

prefix = open8610
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

#########################################

CC  = gcc
OBJ = test8610.o rw8610.o linux8610.o
LOGOBJ = log8610.o rw8610.o linux8610.o
LOGEOBJ = log8610echo.o rw8610.o linux8610.o
MMRESOBJ = memreset8610.o rw8610.o linux8610.o
FETCHOBJ = fetch8610.o rw8610.o linux8610.o
WUOBJ = wu8610.o rw8610.o linux8610.o
CWOBJ = cw8610.o rw8610.o linux8610.o
DUMPOBJ = dump8610.o rw8610.o linux8610.o
HISTOBJ = history8610.o rw8610.o linux8610.o
HISTLOGOBJ = histlog8610.o rw8610.o linux8610.o
DUMPBINOBJ = bin8610.o rw8610.o linux8610.o
XMLOBJ = xml8610.o rw8610.o linux8610.o
PGSQLOBJ = pgsql8610.o rw8610.o linux8610.o
LIGHTOBJ = light8610.o rw8610.o linux8610.o
INTERVALOBJ = interval8610.o rw8610.o linux8610.o
MINMAXOBJ = minmax8610.o rw8610.o linux8610.o
TESTOBJ = test8610.o rw8610.o linux8610.o

VERSION = 0.10

CFLAGS = -Wall -O3 -DVERSION=$(VERSION)
CC_LDFLAGS = -lm
INSTALL = install

####### Build rules

all: dump8610 history8610 log8610 log8610echo memreset8610

test8610 : $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(CC_LDFLAGS)
	
open8610 : $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(CC_LDFLAGS)
	
dump8610 : $(DUMPOBJ)
	$(CC) $(CFLAGS) -o $@ $(DUMPOBJ) $(CC_LDFLAGS)
	
log8610 : $(LOGOBJ)
	$(CC) $(CFLAGS) -o $@ $(LOGOBJ) $(CC_LDFLAGS)

log8610echo : $(LOGEOBJ)
	$(CC) $(CFLAGS) -o $@ $(LOGEOBJ) $(CC_LDFLAGS)
	
memreset8610 : $(MMRESOBJ)
	$(CC) $(CFLAGS) -o $@ $(MMRESOBJ) $(CC_LDFLAGS)

fetch8610 : $(FETCHOBJ)
	$(CC) $(CFLAGS) -o $@ $(FETCHOBJ) $(CC_LDFLAGS)

wu8610 : $(WUOBJ)
	$(CC) $(CFLAGS) -o $@ $(WUOBJ) $(CC_LDFLAGS)
	
cw8610 : $(CWOBJ)
	$(CC) $(CFLAGS) -o $@ $(CWOBJ) $(CC_LDFLAGS)

history8610 : $(HISTOBJ)
	$(CC) $(CFLAGS) -o $@ $(HISTOBJ) $(CC_LDFLAGS)
	
histlog8610 : $(HISTLOGOBJ)
	$(CC) $(CFLAGS) -o $@ $(HISTLOGOBJ) $(CC_LDFLAGS)
	
bin8610 : $(DUMPBINOBJ)
	$(CC) $(CFLAGS) -o $@ $(DUMPBINOBJ) $(CC_LDFLAGS)

xml8610 : $(XMLOBJ)
	$(CC) $(CFLAGS) -o $@ $(XMLOBJ) $(CC_LDFLAGS)

mysql8610:
	$(CC) $(CFLAGS) -o mysql8610 mysql8610.c rw8610.c linux8610.c $(CC_LDFLAGS) -I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient

pgsql8610: $(PGSQLOBJ)
	$(CC) $(CFLAGS) -o $@ $(PGSQLOBJ) $(CC_LDFLAGS) -I/usr/include/pgsql -L/usr/lib/pgsql -lpq

light8610: $(LIGHTOBJ)
	$(CC) $(CFLAGS) -o $@ $(LIGHTOBJ) $(CC_LDFLAGS)
	
interval8610: $(INTERVALOBJ)
	$(CC) $(CFLAGS) -o $@ $(INTERVALOBJ) $(CC_LDFLAGS)
	
minmax8610: $(MINMAXOBJ)
	$(CC) $(CFLAGS) -o $@ $(MINMAXOBJ) $(CC_LDFLAGS) $(CC_WINFLAG)

install:
	mkdir -p $(bindir)
	$(INSTALL) open8610 $(bindir)
	$(INSTALL) dump8610 $(bindir)
	$(INSTALL) log8610 $(bindir)
	$(INSTALL) log8610echo $(bindir)
	$(INSTALL) fetch8610 $(bindir)
	$(INSTALL) memreset8610 $(bindir)
	$(INSTALL) wu8610 $(bindir)
	$(INSTALL) cw8610 $(bindir)
	$(INSTALL) histlog8610 $(bindir)
	$(INSTALL) xml8610 $(bindir)
	$(INSTALL) light8610 $(bindir)
	$(INSTALL) interval8610 $(bindir)
	$(INSTALL) minmax8610 $(bindir)
	
uninstall:
	rm -f $(bindir)/open8610 $(bindir)/dump8610 $(bindir)/log8610 $(bindir)/log8610echo $(bindir)/memreset8610 $(bindir)/fetch8610 $(bindir)/wu8610 $(bindir)/cw8610 $(bindir)/xml8610 $(bindir)/light8610 $(bindir)/interval8610 $(bindir)/minmax8610

clean:
	rm -f *~ *.o open8610 dump8610 log8610 log8610echo memreset8610 fetch8610 wu8610 cw8610 history8610 histlog8610 bin8610 xml8610 mysql8610 pgsql8610 light8610 interval8610 minmax8610 test8610

cleanexe:
	rm -f *~ *.o open8610.exe dump8610.exe log8610.exe log8610echo.exe memreset8610.exe fetch8610.exe wu8610.exe cw8610.exe history8610.exe histlog8610.exe bin8610.exe xml8610.exe mysql8610.exe pgsql8610.exe light8610.exe interval8610.exe minmax8610.exe test8610.exe
