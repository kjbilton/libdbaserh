/*
 * libdbaserhi.h: Internal header file for libdbaserh 
 * 
 * adapted from libdbasei.h for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * libdbasei.h: Internal header file for libdbase
 * 
 * Copyright (c) 2011, 2012 Peder Kock <peder.kock@med.lu.se>
 * Lund University
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LIBDBASERHI_H__ /*JK*/
#define __LIBDBASERHI_H__ /*JK*/

/* libdbase public header */
#include "libdbaserh.h" /*JK*/

/* C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
  DEBUGGING
  - 0 for silent, otherwise debug messages will be printed to stdout 
*/
#define _DEBUG           0

/* 
   Bit order test 
*/	
#define IS_BIG_ENDIAN() ((*(char*) &big_endian_test) == 0)

/*
  dbase usb i/o settings 
*/
/*JK digiBaseRH uses two sets of EP */
#define EP0_IN    (1 | LIBUSB_ENDPOINT_IN)   /*JK, added EP for init functions*/
#define EP0_OUT   (1 | LIBUSB_ENDPOINT_OUT)  /*JK, added EP for init functions*/ 
#define EP_IN     (2 | LIBUSB_ENDPOINT_IN)   /*JK, regular functions -- status, spectra*/
#define EP_OUT    (8 | LIBUSB_ENDPOINT_OUT)  /*JK, regular functions -- status, spectra*/


#define S_TIMEOUT       125          /* Read timeout  (ms) */
#define L_TIMEOUT       1000         /* Write timeout (ms) */
	
/* 
   dbase CTRL byte (see libdbase.h, status_msg) bits
*/
#define MODE_MASK       0x01
#define ON_MASK         0x02
#define LTP_MASK        0x04
#define RTP_MASK        0x08
#define GS_MASK         0x10
#define ZS_MASK         0x20
#define HV_MASK         0x40
#define MODE_LM_MASK    0xfe
#define MODE_PHA_MASK   0x01
#define OFF_MASK        0xfd
#define LTP_OFF_MASK    0xfb
#define RTP_OFF_MASK    0xf7
#define LRTP_OFF_MASK   0xf3
#define GS_OFF_MASK     0xef
#define ZS_OFF_MASK     0xdf
#define HV_OFF_MASK     0xbf

/*
  dbase usb bulk-commands
*/
#define STATUS          0x01
#define START           0x06
#define START2          0x04
#define START3          0x11 /*JK, 1st byte of digiBaseRH 3rd start command */
#define SPECTRUM        0x80
	
/* Maximum path length */
#define MAX_PATH_LEN    512

/*
  List mode masks.
  32-bit word sent in list mode,
  can be either one of:
  1)
    bits 0-20  time
    bits 21-30 amplitude
    bit  31 timestamp flag (=0)
  or 2)
    bits 0-30 time word
    bit  31 timestamp flag (=1)
*/
#define TS_MASK         0x7fffffff
#define T_MASK          0x001fffff
#define A_MASK          0x7fe00000
/* dbase internal timer rollover in listmode */
#define MAX_T           1048575
	
/*
  Some constants
*/
/* Scale factor for correct High Voltage */
#define HV_FACTOR       1.25f
/* Scale factor for correct Fine Gain */
#define GAIN_FACTOR     4194304.0f /* 2^22 */
/* Min set fine gain value (0.40000) */
#define GAIN_SET_MIN    10485760
/* Conversion factor, ticks to seconds */
#define TICKSTOSEC      0.02
/* Max HV in Volts (V) */
#define MAX_HV          1200
/* This also seems sane */
#define MIN_HV          50

/*
  Helper macros
  Endianess: byte swap (copy and swap bytes in x)
*/
#define BYTESWAP(x) dbase_switch_endian((unsigned char *) &x, sizeof(x))
/* PW byte -> float */
#define GET_PW(x)        (0.75f + 0.0625f * ((x) - 12))
/* float -> PW byte */
#define GET_INV_PW(x)    (((x) - 0.75f) / 0.0625f + 12)
/* Account for gain offset when setting the dbase gain value */
#define GET_GAIN_VALUE(x) (GAIN_SET_MIN + (uint32_t)(((x) - 0.5) * GAIN_FACTOR))
#define GET_INV_GAIN_VALUE(x) ((x) / GAIN_FACTOR)

#define GET_TIME(x) ( x * TICKSTOSEC )
#define GET_TICKS(x) ( (uint32_t) (x / TICKSTOSEC) )

 /* 
    Internal usb I/O functions 
 */
 /*
   Read from digibase.
   Internal 8k buffer is used to avoid 
   overflow error.
 */
  int dbase_read(libusb_device_handle *dev, 
		 unsigned char *bytes, 
		 int len, 
		 int *read);
 /*
  JK   Read from digibase.
       Init EP (EP0_IN) is used.
 */
  int dbase_read_init(libusb_device_handle *dev, 
		 unsigned char *bytes, 
		 int len, 
		 int *read);
  /*
   JK  Writes a defined number of bytes to EP.
       Init EP (EP0_OUT) is used.
  */
  int dbase_write_init(libusb_device_handle *dev, 
		  unsigned char *bytes, 
		  int len, 
		  int *written);
  /*
    Writes a defined number of bytes to EP_OUT
  */
  int dbase_write(libusb_device_handle *dev, 
		  unsigned char *bytes, 
		  int len, 
		  int *written);
  /*
   JK  Write one byte to EP (wrapper).
       Init EP (EP0_OUT) is used.
  */
  int dbase_write_one_init(libusb_device_handle *dev, 
		      unsigned char byte, 
		      int *written);
  /*
    Write one byte to EP_OUT (wrapper)
  */
  int dbase_write_one(libusb_device_handle *dev, 
		      unsigned char byte, 
		      int *written);
  /*
   JK  Reads and checks the response of commands sent to dbase init EP.
       Init EP (EP0_IN) is used.
  */
  int dbase_checkready_init(libusb_device_handle *dev);

  /*
    Reads and checks the response of commands sent to dbase
  */
  int dbase_checkready(libusb_device_handle *dev);

  /*
    Sends an empty (zeros) spectra to dbase,
    the mcb internal spectrum is then cleared.
  */
  int dbase_send_clear_spectrum(libusb_device_handle *dev);

  /*
    Write status to dbase.
    all changes in status, like mode, hv, etc 
    are made using this function
  */
  int dbase_write_status(libusb_device_handle *dev, 
			 status_msg * stat);

  /*
    Read status from dbase.
    - should always return 80 bytes containing
      the status of mcb device in *stat.
      for decription see public header.
  */
  int dbase_get_status(libusb_device_handle *dev, 
		       status_msg *stat);

  /*
    Read one spectrum from dbase.
    - in pha mode this returns 1024 int32's
    - in list mode this returns an unknown number of
      of events (128k FIFO)
  */
  int dbase_get_spectrum(libusb_device_handle *dev, 
			 int32_t *chans, 
			 int32_t *last);
  
  /*
    Print's one spectrum to file stream
  */
  void dbase_print_spectrum_file(const int32_t *spec, 
				 int len, 
				 FILE *fh);

  /*
    Print's one binary spectrum to file stream
  */
  void dbase_print_file_spectrum_binary(const int32_t *arr,
					int len, 
					FILE *fh);

  /*
     JK 
     Initialization functions:
     - check if device is already initialized
     - if not, continue with dbase_init2()
     - if yes, continue with dbase_init3()
  */
  int dbase_init(libusb_device_handle *dev);

  /* 
     JK 
     - write packets (probably firmware)
     - write proper status messages
     - clear/reset counters and spectrum
     - read first status message
   */
  int dbase_init2(libusb_device_handle *dev);

  /* 
     JK 
     - finalize the init process
  */
  int dbase_init3(libusb_device_handle *dev);


  /* 
      Get firmware from digiBaseRH.rbf
  */
  unsigned char* dbase_get_firmware_pack(const char* file, 
					 int n, int *len);
  
  /* 
     Find dbase on usb-bus. 
     Return first one or, if given_serial is not -1,
     a specific one.
  */
  libusb_device_handle *dbase_locate(int* serial, 
				     int given_serial,
				     libusb_context * cntx);

  /* 
     Extract serialnumber for a libusb device (dbase)
  */
  int dbase_get_serial(libusb_device *dev,
		       libusb_device_handle *handle,
		       struct libusb_device_descriptor desc,
		       int *serial);

  /* Basic detector pointer NULL checks */
  int check_detector(const detector *det, const char *str); 
  

  /* 
     Parse status text file
  */
  int parse_status_lines(status_msg * status, FILE * fh);

  /* 
     Function for libusb error handling 
  */ 
  void err_str(const char *func, 
	       enum libusb_error err);
  
  /* 
     Big endian conversion function
     Pack status struct for b.e. machines 
  */
  void dbase_byte_swap_status_struct(status_msg *stat);

  /* 
     Check sanity before saving status 
     A copy of stat ready to save is returned in *mod
     if check_status_sanity returns <0 status is corrupt
  */
  int check_status_sanity(status_msg stat, status_msg *mod);

  /* 
     Switch endianess of bytes in b 
  */
  void dbase_switch_endian(unsigned char *b, int n);

  /* 
   JK  Print n bytes of msg in a nice form.
       Auxiliary function for DEBUG purpose.
  */
  void dbase_print_msg(unsigned char *string, int n);

  

#ifdef __cplusplus
}
#endif /* end of c++ */

#endif /* end of __LIBDBASERHI_H__ */
