/*
 * libdbaserh.h: Public header for libdbaserh 
 * 
 * Communicate with ORTEC's digiBaseRH MCB over USB
 * 
 * adapted from libdbase.h for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * libdbase.h: Public header for libdbase
 * 
 * Communicate with ORTEC's digiBASE MCB over USB
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

#ifndef __LIBDBASERH_H__ /*JK*/
#define __LIBDBASERH_H__ /*JK*/

/* libusb 1.X public header */
#include <libusb-1.0/libusb.h>

/* C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
  digibase identifiers
*/
#define VENDOR_ID       0x0a2d       /* Ortec */
/*#define PROD_ID         0x000f*/   /* digiBASE */
#define PROD_ID         0x001f       /*JK 0x001f digiBaseRH version ID*/
#define DBASE_LEN       1023         /* digibase channels-1 */

/* 
   Status message:
   - Contains information about the digibase's settings.
   - Used when writing and reading settings to/from dbase.
   - Todo: some fields are unknown...
   (80 bytes)
*/
/* #pragma pack(2) */
typedef struct {
  /* 0x0000 - 0x000f */
  uint8_t CTRL;             /* Control byte */
  uint8_t RDY;              /* Ready byte? (bit0: counting, bit3: ready */
  uint8_t PW;               /* Pulse Width (shaping time) */
  uint8_t AHV;              /* Actual High Voltage low byte */
  uint8_t u2;               /* ux = unknown x;  TODO: probably ahv high byte (?) */ 
  uint8_t ISC;              /* InSight Control byte (not used) */
  uint8_t AIO;              /* Aux0 IO bytes (not used) */
  uint8_t u3;               /* Aux1? */
  /* 2012-02-20: 
     u4 replaced with 2-byte timer */
  /* Bits 8-23 is a 16 Hz Timer */
  uint32_t TMR;             /* Countdown (for what?), starts when spectrum is requested */
  uint32_t AFGN;            /* Actual fine gain */ 

  /* 0x0010 - 0x001f*/
  uint32_t FGN;             /* Fine Gain */
  uint16_t LLD;             /* Lower level discriminator */
  uint16_t u6;
  uint32_t LTP;             /* Live Time Preset */
  uint32_t LT;              /* Live Time */

  /* 0x0020 - 0x002f */
  uint32_t RTP;             /* Real Time Preset */
  uint32_t RT;              /* Real Time */
  uint16_t LEN;             /* MCA Channels */
  uint16_t HVT;             /* High Voltage target */
  uint32_t u7;

  /* 0x0030 - 0x003f */
  uint32_t u8;
  uint32_t u9;
  uint16_t GSU;             /* Gain stabilization Upper channel */
  uint16_t GSC;             /* Gain stabilization Center channel */
  uint16_t GSL;             /* Gain stabilization Lower channel */
  uint16_t u10;

  /* 0x0040 - 0x004f */
  uint16_t u11;
  uint16_t ZSU;             /* Zero stabilization Upper channel */
  uint16_t ZSC;             /* Zero stabilization Center channel */
  uint16_t ZSL;             /* Zero stabilization Lower channel */
  uint8_t MSZ;              /* Memory Size (kb?) */
  uint8_t u12;             
  uint8_t u13; 
  uint8_t u14;              
  uint8_t CNT;              /* Clear counters byte (bit 1) */
  uint8_t u15; 
  uint16_t u16;
} status_msg;
/* #pragma pack() */

/*
  Detector (digibase MCA/B) struct
*/
typedef struct {
  int serial;                     /* digibase's Serial number */
  libusb_device_handle *dev;      /* underlying libusb device handle */
  status_msg status;              /* status struct */
  int32_t spec[DBASE_LEN+1];      /* spectrum */
  int32_t last_spec[DBASE_LEN+1]; /* diff spectrum (difference since last readout) */
} detector;

/*
  List-mode pulse struct.
  each event (pulse) contains amplitude and time information
  (8 bytes)
*/
typedef struct {
  uint32_t amp;             /* Pulse amplitude (a.u.)  */
  uint32_t time;            /* Pulse arrival time (us) */
} pulse;

/*
  STATUS MACROS
  can be used to check status of dbase
*/
#define IS_PHA_MODE(x)  (((x)->status.CTRL & 0x01) > 0)
#define IS_ON(x)        (((x)->status.CTRL & 0x02) > 0)
#define IS_LTP_ON(x)    (((x)->status.CTRL & 0x04) > 0)
#define IS_RTP_ON(x)    (((x)->status.CTRL & 0x08) > 0)
#define IS_GS_ON(x)     (((x)->status.CTRL & 0x10) > 0)
#define IS_ZS_ON(x)     (((x)->status.CTRL & 0x20) > 0)
#define IS_HV_ON(x)     (((x)->status.CTRL & 0x40) > 0)

/* This bit is set only when startup is complete */
//#define IS_READY(x)     (((x)->status.RDY & 0x08) > 0)

/* shift out time-bits from TMR word */
#define GET_TIMER(x)    ((((x)->status.TMR & 0xff0000) >> 8) + (((x)->status.TMR & 0xff00) >> 8))
#define GET_TIMER_S(x)   (GET_TIMER(x) >> 4)

  /* libdbase_functions:

     Most functions return an int, indicating the status of the
     function call. Negative values indicate that an error occurred,
     while positive or zero value indicate ok.

     - USB I/O functions return libusb errors, and should also print
       any error that occurred to stderr.
     - Other functions are written in the same way, printing to
       stderr and returning neg. <errno.h> values on failure.
   */
  

  /* 
     CTRL funtions: enable/disable settings 

     Start detector,
     it will continue to measure util libdbase_stop() is called or
     real/live preset time has elapsed
  */
int libdbase_start(detector *det);
  /* Stop detector */ 
int libdbase_stop(detector *det);        

  /* 
     Enable gain stabilization 
     defined in libdbase_set_gs_chans()
  */  
int libdbase_gs_on(detector *det);      
  /* Disable gain stabilization */
int libdbase_gs_off(detector *det);    

  /*
    Enable zero stabilization
    defined in libdbase_set_zs_chans()
  */
int libdbase_zs_on(detector *det);      
  /* Disable zero stabilization */
int libdbase_zs_off(detector *det);                        

  /* 
     Enable high voltage 
     defined by libdbase_set_hv()
  */
int libdbase_hv_on(detector *det);
  /* Turn of high voltage */ 
int libdbase_hv_off(detector *det);                        

  /* 
     Settings functions: set/get values and properties 
     
     Set target high voltage (50-1200 V)
  */
int libdbase_set_hv(detector *det, ushort hv);
  /* Set (and enable) real/live time presets */
int libdbase_set_rtp(detector *det, double real_time_preset);
int libdbase_set_ltp(detector *det, double live_time_preset);
  /* Disable real/live time presets */
int libdbase_set_rtp_off(detector *det);
int libdbase_set_ltp_off(detector *det);
  /* Set gain stabilization channels (ROI) */
int libdbase_set_gs_chans(detector *det, ushort center, ushort width);
  /* Set zero stabilization channels (ROI) */
int libdbase_set_zs_chans(detector *det, ushort center, ushort width);
  /* Set pulse width (0.75-2.0 us) */
int libdbase_set_pw(detector *det, float pulse_width);
  /* Set fine gain (0.4-1.2) */
int libdbase_set_fgn(detector *det, float fg);

  /* Update spectrum buffer, read spectrum from dbase */
int libdbase_get_spectrum(detector *det);
  /* Update status buffer, read status from dbase */ 
int libdbase_get_status(detector *det);
  /* Wrapper for low-level dbase_write_status() */
int libdbase_set_status(detector *det);

  /*
    This is the list mode function.
    call libdbase_set_list_mode() before calling this one.

    There are two return possibilities:
    1) if buf is NULL, read is set to the number
       of events (pulses) registered since last function call
    2) if buf is non-NULL, the buffer is filled with 'read'
       number of events, each with amplitude and time.

    For both return types 'time' is used to track digibase's internal
    timer overflow. At first call 'time' should point to an uint32_t
    set to 0.
   */
int libdbase_read_lm_packets(detector *det,   /* the detector pointer */
			pulse *buf,      /* pulse buffer (can be NULL) */
			int len,         /* length of pulse buffer */
			int *read,       /* returns the read nbr of events */
			uint32_t *time); /* timer pointer to track internal overflow */

  /* 
     Misc functions: helpers, prints etc 
  
     Clear (and disable) live/real time presets */
int libdbase_clear_presets(detector *det);
  /* Clear MCB spectrum */
int libdbase_clear_spectrum(detector *det);
  /* Clear live/real time counters */
int libdbase_clear_counters(detector *det);
  /* Clear presets, spectrum and time counters */
int libdbase_clear_all(detector *det);

  /* 
     Print formatted status buffer (to stdout).
     These two functions actually print the
     det->status buffer, so call libdbase_get_status()
     to make sure the buffer is up to date.
  */ 
void libdbase_print_status(const detector *det);     
  /* Print status (to stream) */ 
void libdbase_print_file_status(const status_msg * status, const int serial, FILE *fh);        

  /* 
     All print_X_spectrum functions, except the two binary,
     prints all channels separated by space: " ".
     They actually print the det->spec or det->last_spec buffers
     so, these functions should be preceded with a call to
     libdbase_get_spectrum()
   */
  /* Print diff spectrum (to stdout) */ 
void libdbase_print_spectrum(const detector *det);
  /* Print diff spectrum (to stdout) */
void libdbase_print_diff_spectrum(const detector *det);
  /* Print spectrum (to stream) */
void libdbase_print_file_spectrum(const detector *det, FILE *fh);       
  /* Print spectrum (to stream) */
void libdbase_print_diff_file_spectrum(const detector *det, FILE *fh);                       
  /* Print binary spectrum (to stream) */
void libdbase_print_file_spectrum_binary(const detector *det, FILE *fh);
  /* Print binary differential spectrum (to stream) */
void libdbase_print_diff_file_spectrum_binary(const detector *det, FILE *fh);

  /* 
     Sum region and optionally, if [fh != NULL],
     print region of interest
  */
void libdbase_print_roi(detector *det, uint start_ch, uint end_ch, uint *sum, FILE *fh);

  /* 
     Mode functions 

     Set detector in list mode 
     
     When you're done reading pulses in list
     mode you should call the libdbase_set_pha_mode()
     to reset the detector in default mode.
  */
int libdbase_set_list_mode(detector *det);                          
  /* Set detector in pha mode (default mode) */ 
int libdbase_set_pha_mode(detector *det);                             

/*
    Open and Close functions 
*/

/*
  Initialize all digibases found on the system
  - this function calls libdbase_init() on each
    detector* found on usb
  - returns NULL terminated detector* array and
    number of dbases in *found
*/

int libdbase_get_list(detector *** list, int *found);

/* 
   Close connection to each detector and free list 
   - this function calls libdbase_close() on each 
     detector* in list.
*/
int libdbase_free_list(detector *** list);

/* Find, open (usb) and create detector handle 
   - returns NULL if no digibase was found or an
     fatal error occurred.
     if serial is -1, a pointer to the first dbase is returned
     otherwise a pointer to the dbase with the given serial
     number is returned
*/
detector *libdbase_init(int dbase_serial);

/*
  Close (usb) and release resources 
  - returns libusb_release status
*/
int libdbase_close(detector *det);

/*
  Iterate through libusb devices and 
  extract serial numbers of all (k) digibases found.
  - returns (up to len) serial numbers in serials[0..max(k-1,len-1)]
  - returns nbr of found dbases (k) or < 0 on error
*/
int libdbase_list_serials(int *serials, int len);

/*
  Save and Load status functions:

  - save current status to file:
    the path can be either one of
     a) Custom directory *dir, or if *dir == NULL:
     b) PACK_PATH set at compile time (see Makefile), else
     c) current directory
     
   A new directory (det->serial) will be created
     in the path.
*/
//int libdbase_save_status(detector *det, const char *dir);
int libdbase_save_status_text(detector *det, const char* dir);

/*
  Load status from file:
    
  - status will be read from the serial-
    number directory at one of the locations
    described above in libdbase_save_status().

  - allows user to (carefully!) modify status of digibase 
    by hand in a text-editor.
    
  - digibase's status will be updated if
    status is successfully loaded.
*/
//int libdbase_load_status(detector *det, const char *dir);
int libdbase_load_status_text(detector *det, const char* dir);

/*
  Write libdbase's detector-specific directory
  into path buffer (without trailing '/').
  Can be used by applications to store detector-
  specific data.
*/
int libdbase_get_det_path(detector *det, char *path, ssize_t len);
 
/* 
   libdbase root path (see Makefile)
*/
int libdbase_get_path(char *path, ssize_t len);

#ifdef __cplusplus
}
#endif /* end of c++ */
#endif /* end of __LIBDBASERH_H__ */ /*JK*/
