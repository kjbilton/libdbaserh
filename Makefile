#
# GNU project C compiler Makefile for libdbase
#
# Default install path is:
# 	libs in 	/usr/local/lib
#	header in 	/usr/local/include
#	files in 	/usr/local/share/libdbaserh
#
###################################################################
# USER SETTINGS:
#
# Change this to the name of libusb-1.X (without the 'lib' part)
# on your installation 
# (libusb-1.0 was installed under this name on ubuntu 10.10 x86_64)
# If #include <libusb-1.0/libusb.h> doesn't work 
# on your system you need to manually change the include line
# in src/libdbaserh.h on line #26 to libusb-1.0's public header
LIBUSBNAME = usb-1.0
# Change this to change library install path,
# /lib and /include will be appended to this path.
INSTALL = /usr/local
# Specify the location of digiBaseRH.rbf file, 
# required by libdbase. Default /usr/local/share/libdbaserh.
#PACKAGE_PATH = /usr/local/share/libdbaserh
PACKAGE_PATH = ./fw/
#
# END USER SETTINGS.
###################################################################

# Current library name and version
MAJOR = 0
MINOR = 1
NAME = libdbaserh.so.$(MAJOR).$(MINOR)
# Compiler options
CC = gcc
CFLAGS = -g -Wall -O2 -DPACK_PATH=\"$(PACKAGE_PATH)\"
LDLIBS = -l$(LIBUSBNAME) libdbaserh.a
LDFLAGS = -L.
BINDIR = .
VPATH = src
SRC = libdbaserh.c libdbaserh.h libdbaserhi.h
iSRC = libdbaserhi.c libdbaserh.h libdbaserhi.h 
#EXS = example1 example2 example3

###################################################################
# Top targets
###################################################################
all : lib dbaserh 
#examples
lib : static
static : libdbaserh.a
dbaserh : dbaserh.o
#examples : $(EXS)

shared : $(NAME)
###################################################################
# Small test programs
###################################################################
#example1.o: example1.c $(SRC)
#example2.o: example2.c $(SRC)
#example3.o: example3.c $(SRC)

###################################################################
# Control program (default: link static)
###################################################################
#dbase_shared: dbase.o
#	$(CC) $< -l$(LIBUSBNAME) -ldbase -o $(BINDIR)/dbase
#dbase.o: dbase.c dbase.h $(SRC)

###################################################################
libdbaserh.a: libdbaserh.o libdbaserhi.o # link static
	ar rs $@ libdbaserh.o libdbaserhi.o

libdbaserh.o: $(SRC)      # build static lib part 1
libdbaserhi.o: $(iSRC)    # build static lib part 2

$(NAME): libdbaserhs.o libdbaserhis.o   # link shared
	$(CC) -shared -fPIC -o $@ libdbaserhs.o libdbaserhis.o

libdbaserhs.o: $(SRC)     # build shared lib
	$(CC) $(CFLAGS) -c -fPIC $< -o $@
libdbaserhis.o: $(iSRC)
	$(CC) $(CFLAGS) -c -fPIC $< -o $@

install: lib shared
	mkdir -p $(INSTALL)/include
	install -m 644 -o root -g root src/libdbaserh.h $(INSTALL)/include/libdbaserh.h
	mkdir -p $(INSTALL)/lib
	install -m 755 -o root -g root $(NAME) $(INSTALL)/lib/$(NAME)
	install -m 644 -o root -g root libdbaserh.a $(INSTALL)/lib/libdbaserh.a
	mkdir -p $(PACKAGE_PATH)
	rm -f $(INSTALL)/lib/libdbaserh.so
	ln -sf $(NAME) $(INSTALL)/lib/libdbaserh.so
	ln -sf $(NAME) $(INSTALL)/lib/libdbaserh.so.0
	ldconfig -n $(INSTALL)/lib
clean:
	rm -f *.o *.a example* dbaserh $(NAME)
