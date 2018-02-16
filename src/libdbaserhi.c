/*
 * libdbaserhi.c: Internal libdbaserh source 
 * 
 * adapted from libdbasei.c for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * libdbasei.c: Internal libdbase source 
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

#include <errno.h>  /* error codes */
#include <stdio.h>  /* printf(), fprintf(), ... */
#include <stdlib.h> /* malloc(), calloc(), free(), ... */
#include <string.h> /* memcpy() */
#include <time.h>   /* JK, time(), ctime()*/


/* libdbase non-public header */
#include "libdbaserhi.h" /*JK*/

/*
  Endianess (defined in libdbaserh.c)
*/
extern int big_endian_test;

/*
  These status msgs are written to dbase during init() 
  Use GCC's compact notation for sparse arrays:
  omitted elements are initialized to zero.

   libdbase v. 0.2
   We'll use this shorter status array instead. 
   Only the two first msg's differ significantly, the rest are set in init2()
*/
const unsigned const char _STAT[2][81] = {
  // first status msg 0-79
  {
  /*JK digiBaseRH*/
    [1]=0xb3,
    [3]=0x0c,  [4]=0x20, [6]=0x30,  [7]=0x20,  [8]=0x03, [13]=0x0a,
    [19]=0xa0, [22]=0x28,
    [41]=0xff, [42]=0x03, 
    [43]=0x80, [44]=0x02, //HV Target (in hex, 1.25 is HV_FACTOR constant, dec2hex(800/1.25)=0x0280)
    [57]=0x5e, [58]=0x01, //Gain stabilization, upper channel  (in hex, dec2hex(350) = 0x015e)
    [59]=0x2c, [60]=0x01, //Gain stabilization, center channel (in hex, dec2hex(300) = 0x012c)
    [61]=0xfa, [62]=0x00, //Gain stabilization, lower channel  (in hex, dec2hex(250) = 0x00fa) 
    [66]=0x80, 
    [67]=0x9e, [68]=0x00, //Zero stabilization, upper channel  (in hex, dec2hex(158) = 0x009e)
    [69]=0x85, [70]=0x00, //Zero stabilization, center channel (in hex, dec2hex(133) = 0x0085)
    [71]=0x6c, [72]=0x00, //Zero stabilization, lower channel  (in hex, dec2hex(108) = 0x006c)
    [73]=0x40, [77]=0x10, [78]=0x0c, [79]=0x24
   },
  {
  /*JK digiBaseRH*/
    [1]=0x31,
    [3]=0x0c, [4]=0x20, [6]=0x30,  [7]=0x20,  [8]=0x03, [13]=0x0a,
    [19]=0x20, [22]=0x28,
    [41]=0xff, [42]=0x03, 
    [43]=0x80, [44]=0x02, //HV Target (in hex, 1.25 is HV_FACTOR constant, dec2hex(800/1.25)=0x0280)
    [57]=0x5e, [58]=0x01, //Gain stabilization, upper channel  (in hex, dec2hex(350) = 0x015e)
    [59]=0x2c, [60]=0x01, //Gain stabilization, center channel (in hex, dec2hex(300) = 0x012c)
    [61]=0xfa, [62]=0x00, //Gain stabilization, lower channel  (in hex, dec2hex(250) = 0x00fa) 
    [67]=0x9e, [68]=0x00, //Zero stabilization, upper channel  (in hex, dec2hex(158) = 0x009e)
    [69]=0x85, [70]=0x00, //Zero stabilization, center channel (in hex, dec2hex(133) = 0x0085)
    [71]=0x6c, [72]=0x00, //Zero stabilization, lower channel  (in hex, dec2hex(108) = 0x006c)
    [73]=0x40, [78]=0x0c, [79]=0x24
  }
};


/* 
  JK  Read bytes from EP0_IN -- init process 
*/
int dbase_read_init(libusb_device_handle *dev, unsigned char *bytes, int len, int *read){
  if(dev == NULL){
    fprintf(stderr, "E: dbase_read_init() device handle is null pointer\n");
    return -1;
  }
  
  /* Temporary buffer to avoid overflow errors */
  const int iread_buf = 2 * (DBASE_LEN + 1) * sizeof(int32_t); /* 8192 */
  unsigned char *buf = (unsigned char *) malloc( iread_buf );
  if(buf == NULL){
    fprintf(stderr, "E: unable to allocate memory in dbase_read_init()\n");
    return -ENOMEM;
  }
  
  /* Read data from device */
  int err = libusb_bulk_transfer(dev, EP0_IN, buf, iread_buf, read, S_TIMEOUT);

  /* Copy data: buf to bytes */
  if(len < *read) {
    fprintf(stderr, "E: dbase_read_init() read %d bytes, buffer can only hold %d\n", *read, len);
    free(buf);
    return -1;
  }
  memcpy(bytes, buf, *read);
  
  if( _DEBUG > 0){
    if(*read == 1)
      printf("Read 0x%02x from device\n", (unsigned char) buf[0]);
    else
      printf("Read %d bytes from device\n", *read);
  }
  
  /* Free temporary buffer */
  free(buf);
  
  return err;
}

/* 
   Read bytes from EP (in) 
*/
int dbase_read(libusb_device_handle *dev, unsigned char *bytes, int len, int *read){
  if(dev == NULL){
    fprintf(stderr, "E: dbase_read() device handle is null pointer\n");
    return -1;
  }
  
  /* Temporary buffer to avoid overflow errors */
  const int iread_buf = 2 * (DBASE_LEN + 1) * sizeof(int32_t); /* 8192 */
  unsigned char *buf = (unsigned char *) malloc( iread_buf );
  if(buf == NULL){
    fprintf(stderr, "E: unable to allocate memory in dbase_read()\n");
    return -ENOMEM;
  }
  
  /* Read data from device */
  int err = libusb_bulk_transfer(dev, EP_IN, buf, iread_buf, read, S_TIMEOUT);
  /* Copy data: buf to bytes */
  /*
    2012-02-13, should only copy read bytes
    was: memcpy(bytes, buf, len); 
    Added additional check of len
  */
  if(len < *read) {
    fprintf(stderr, "E: dbase_read() read %d bytes, buffer can only hold %d\n", *read, len);
    free(buf);
    return -1;
  }
  memcpy(bytes, buf, *read);
  
  if( _DEBUG > 0){
    if(*read == 1)
      printf("Read 0x%02x from device\n", (unsigned char) buf[0]);
    else
      printf("Read %d bytes from device\n", *read);
  }
  
  /* Free temporary buffer */
  free(buf);
  
  return err;
}

/* 
  JK  Write bytes to EP0_OUT -- init process
*/
int dbase_write_init(libusb_device_handle *dev, unsigned char *bytes, int len, int *written){
  if(dev == NULL){
    fprintf(stderr, "E: dbase_write_init() device handle is null pointer\n");
    return -1;
  }
  if(bytes == NULL && len > 0){
    fprintf(stderr, "E: dbase_write_init() trying to write null pointer, but len was %d\n", len);
    return -1;
  }

  /* Write data to device */
  int err = libusb_bulk_transfer(dev, EP0_OUT, bytes, len, written, L_TIMEOUT);

  if(err < 0)
    err_str("dbase_write_init()", err);

  if( _DEBUG > 0){
     printf("Wrote %d bytes to device\n", written[0]);
  }

  return err;
}

/* 
   Write bytes to EP (out)
*/
int dbase_write(libusb_device_handle *dev, unsigned char *bytes, int len, int *written){
  if(dev == NULL){
    fprintf(stderr, "E: dbase_write() device handle is null pointer\n");
    return -1;
  }
  if(bytes == NULL && len > 0){
    fprintf(stderr, "E: dbase_write() trying to write null pointer, but len was %d\n", len);
    return -1;
  }

  /* Write data to device */
  int err = libusb_bulk_transfer(dev, EP_OUT, bytes, len, written, L_TIMEOUT);

  if(err < 0)
    err_str("dbase_write()", err);
  if( _DEBUG > 0)
    printf("Wrote %d bytes to device\n", written[0]);
  return err;
}

/* 
  JK  Write one byte to EP0_OUT -- init process
*/
int dbase_write_one_init(libusb_device_handle *dev, unsigned char byte, int *written){
  if( _DEBUG > 0)
    printf("Writing 0x%02x to device\n", byte);
  return dbase_write_init(dev, &byte, 1, written);
}

/* 
   Write one byte to EP (out) 
*/
int dbase_write_one(libusb_device_handle *dev, unsigned char byte, int *written){
  if( _DEBUG > 0)
    printf("Writing 0x%02x to device\n", byte);
  return dbase_write(dev, &byte, 1, written);
}

/* 
  JK  Check response from device -- init process
*/
int dbase_checkready_init(libusb_device_handle *dev){
  /* size of resp could be 2 and 6 bytes in init process*/
  unsigned char resp[6];
  int err, read;
  err = dbase_read_init(dev, resp, (int) sizeof(resp), &read);

  /*JK -- digiBaseRH, size of response msg is
    START, START2, START3 commands -> 2 bytes
    check init command -> 6 bytes 
   */

  if( err < 0 || read != 2){  

    if (read != 6) {
       if(_DEBUG > 0){
       printf("dbase_checkready_init() got %d bytes back\n", read);
       err_str("dbase_checkready_init()", err);
       }
       return -1;
    }
    else
       return resp[5]; //last byte from 6 total, should be 0x03

  }
  /* JK, init needed?
      4 -- init needed, 0 -- unit already awake
  */
  if(resp[0] == 4 || resp[0] == 0)      
    return resp[0];

  return -1;
}

/* 
   Check response from device
*/
int dbase_checkready(libusb_device_handle *dev){
  /* Sometimes we get status back instead of 1... */
  unsigned char resp[sizeof(status_msg)];
  int err, read;
  err = dbase_read(dev, resp, sizeof(status_msg), &read);

  if( err < 0 || read != 2){  /*JK*/

    if (read != 0) {          /* JK, NULL is requested after sending status*/

       if(_DEBUG > 0){
         printf("dbase_checkready() got %d bytes back\n", read);

         /* 80 bytes (status) might actually have been requested */
         if(read != sizeof(status_msg))
           err_str("dbase_checkready()", err);
         else return 1;
       }
       return -1;
     }
     else return 0; /*JK, read is 0 */
  }
  return 1; /*JK, read is 2*/
}

/* 
   Clear the internal spectrum of MCB 
*/
int dbase_send_clear_spectrum(libusb_device_handle *dev){
  int err, written;
  
  /* use calloc to initialize to zeros... */
  unsigned char *buf = calloc( 4097, 1 );
  if(buf == NULL){
    fprintf(stderr, "E: Unable to allocate buffer in dbase_send_clear_spectrum()\n");
    return -ENOMEM;
  }
  /* prepend 0x02 to spectrum */
  buf[0] = (unsigned char)2;
  
  /* Write det->err = dbase_read(dev, (unsigned char*)&tmp_stat, sizeof(status_msg), &io );
status.LEN int32_t zeros to dbase */
  err = dbase_write(dev, buf, 4097, &written);
  
  /* Free memory */
  free(buf);
  buf = NULL;
  
  if(err < 0 || written != 4097){
    fprintf(stderr, "E: unable to send clear spectrum (sent %d)\n", written);
    err_str("dbase_send_clear_spectrum()", err);
    return err < 0 ? err : -EIO;
  }
  return dbase_checkready(dev);
}

/* 
   Send status message 
*/
int dbase_write_status(libusb_device_handle *dev, status_msg * stat){
  int err, written;
  
  /* Create temporary buffer to hold status struct + 1 */
  unsigned char buf[sizeof(status_msg) + 1];
  /* Prepend zero at beginning of buffer */
  buf[0] = 0;

  /* Copy status msg to buffer */
  memcpy(buf + 1, stat, sizeof(status_msg));

  /* 2012-02-16
     Moved down after memcpy(),
     changed to buf - we shouldn't touch stat! 
  */
  if( IS_BIG_ENDIAN() )
  {
    dbase_byte_swap_status_struct( (status_msg*) (buf+1) );
  }
  
  /* Send status_msg */
  err = dbase_write(dev, buf, sizeof(status_msg) + 1, &written);
  
  /* Check nbr of sent bytes */
  if(err < 0 || written != (sizeof(status_msg) + 1) ){
    printf("E: unable to send whole status_msg ( sent=%d)\n", written);
    err_str("dbase_write_status()", err);
    return err < 0 ? err : -EIO;
  }
  /* Success? */
  return dbase_checkready(dev);
}

/* 
   Read status bytes from device 
*/
int dbase_get_status(libusb_device_handle *dev, status_msg *stat){
  /* Temporary buffer */
  status_msg tmp_stat;
  int err, io;
  
  /* Request status from device */
  err = dbase_write_one(dev, STATUS, &io);
  if(err < 0 || io != 1) {
    fprintf(stderr, "E: When writing status request (written=%d)\n", io);
    err_str("dbase_get_status()", err);
  }
  /* Read answer */
  err = dbase_read(dev, (unsigned char*)&tmp_stat, sizeof(status_msg), &io );
  
  /* repack big endian struct */
  if( IS_BIG_ENDIAN() )
  {
    dbase_byte_swap_status_struct(&tmp_stat);
  }
  /* Sanity check */
  if( err == 0 &&                             /* no libusb error */
      io == sizeof(status_msg) &&             /* whole message recieved? */
      tmp_stat.LEN == (uint16_t)DBASE_LEN &&  /* this is constant on digiBASE */
      tmp_stat.HVT > (uint16_t) MIN_HV)       /* this also seems sane */
  {
    /* Status seems ok - update pointer */
    *stat = tmp_stat;
  }
  else if(err < 0){
    err_str("dbase_get_status()", err);
  }
  else {
    fprintf(stderr, "W: sanity check failed on recieved status data\n");
  }
  
  return err;
}

/* 
   Read spectrum from device 
*/
int dbase_get_spectrum(libusb_device_handle *dev, int32_t *chans, int32_t *last){
  int err, io;
  const int len = (DBASE_LEN + 1);
  const int len_bytes = len * sizeof(int32_t);
  unsigned char onflag = 1;
  
  /* Temporary buffer */
  int32_t tmp[ len ];
  
  /* Request spectrum */
  err = dbase_write_one(dev, SPECTRUM, &io);
  if(err < 0 || io != 1){
    fprintf(stderr, "E: get_spectrum (written=%d)\n", io);
    err_str("dbase_get_spectrum()", err);
    return err < 0 ? err : -EIO;
  }
  
  /* Read spectrum bytes into temporary buffer */
  err = dbase_read(dev, (unsigned char*)tmp, len_bytes, &io);
  
  //  if(err < 0 || io != 4096){
  if(err < 0 || io != len_bytes){
    fprintf(stderr, "E: possible incomplete read of spectral data (read %d bytes)\n", io);
    err_str("dbase_get_spectrum()", err);
    return err;
  }
  
  /* Read ok; parse int32, calc changes since last spectrum and update spectrum */
  for(io=0; io < len; io++){
    /* Check for first msmt after clear */
    if(onflag){
      /* spec = all zeros; first measurement  */
      onflag &= (chans[io] == 0);
    }
    /*
      Watch out for byte order
    */
    if( IS_BIG_ENDIAN() ){
      BYTESWAP(tmp[io]);
    }
    last[io] = tmp[io] - chans[io];
    chans[io] = tmp[io];
  }
  
  /* If onflag is true, set all diff spectra to zeros */
  if(onflag) {
    for(io=0; io < len; io++)
      last[io] = 0;
  }
  
  return err;
}

/*
  Print spectrum to file handle
*/
void dbase_print_spectrum_file(const int32_t *spec, int len, FILE *fh){
  if(spec == NULL){
    fprintf(stderr, "E: dbase_print_spectrum_file(), *spec was NULL\n");
    return;
  }
  if(fh == NULL){
    fprintf(stderr, "E: dbase_print_spectrum_file(), *fh was NULL\n");
    return;
  }
  if(len <= 0){
    fprintf(stderr, "E: dbase_print_spectrum_file(), len was %d\n", len);
    return;
  }
  /* 2012-02-14
     Used to call fprintf 'len' times,
     print to memory string first...
  */
  int k, left, w = 0;
  /* allocate 1 char per chan (including space) */
  const int n = 2048;
  char *buf = (char *) malloc( n );
  if(buf == NULL){
    fprintf(stderr, "E: dbase_print_spectrum_file() unable to allocate memory\n");
    return;
  }
  
  for(k = 0; k < len; k++) {
    left = n - w;
    w += snprintf(buf+w, left, "%d ", spec[k]);
    /* Check for overflows in buffer -> flush to fh */
    if( left < 10 ) {
      fprintf(fh, "%s", buf);
      w = 0; /* reset written to 0 */
    }
  }
  fprintf(fh, "%s\n", buf);


  if(_DEBUG)
    fprintf(fh, "%d chars since buffer was cleared\n", w);
  free(buf);

  /* flush stream */
  if( fflush(fh) < 0 )
    fprintf(stderr, "E: dbase_print_spectrum_file() when flushing stream\n");

  /* old code
  for(k=0;k<len;k++)
     fprintf(fh, "%d ", spec[k]); 
  fprintf(fh, "\n"); */
}

/* 
   Actual binary print method 
*/
void dbase_print_file_spectrum_binary(const int32_t *arr, int len, FILE *fh){
 if(fh != NULL && arr != NULL){
   int wtn;
   wtn = fwrite(arr, sizeof(int32_t), len, fh);
   if(wtn != len){
     fprintf(stderr, 
	     "E: dbase_print_file_spectrum_binary() written %d should be %d, errno=%d\n",
	     wtn,
	     len,
	     errno);
   }
   if( fflush(fh) < 0 )
     fprintf(stderr, "E: dbase_print_file_spectrum_binary() when flushing stream\n");
 }
 else if(arr != NULL)
   fprintf(stderr, "E: dbase_print_file_spectrum_binary(): FILE handle was NULL\n");
 else
   fprintf(stderr, "E: dbase_print_file_spectrum_binary(): spectrum handle was NULL\n");
}

/* 
  JK, digibase init-sequence TODO: "clean" the init process
*/
int dbase_init(libusb_device_handle *dev){
  int err, written;
  unsigned char buf0[4] = {[0] = START, [2] = 0x02};/*JK, digiBaseRH start msgs are 4 bytes*/

  /* PHASE I: */
  if(_DEBUG > 0)
    printf("\n\nInit - PHASE I:\n");
 
 
  /* ?? Don't know if these are neccessary... ?? */
 {
    err = libusb_clear_halt(dev, EP0_IN); /*JK*/
    if(err < 0){
      fprintf(stderr, "W: libusb_clear_halt() (init EP IN:  %d) failed\n", EP0_IN); /*JK*/
      err_str("dbase_init()", err);
      }
    err = libusb_clear_halt(dev, EP0_OUT); /*JK*/
    if(err < 0){
      fprintf(stderr, "W: libusb_clear_halt() (init EP OUT: %d) failed\n", EP0_OUT); /*JK*/
      err_str("dbase_init()", err);
    }
  }
  
  /* Already awake?
     - If digiBaseRH is awake and initialized it will
     respond with a 2byte msg to first START command
     init needed   -- 0x04 0x80
     already initialized -- 0x00 0x00
  */

  /* Send START command */ 
  err = dbase_write_init(dev, buf0, sizeof(buf0), &written); /*JK*/


  if(err < 0 || written != sizeof(buf0)){  /*JK*/ 
    fprintf(stderr, "E: When writing START command (err=%d, written=%d)\n", err, written);
    err_str("dbase_init(START)", err);
  }
  
  /* New Connection? */
  err = dbase_checkready_init(dev); /*JK*/

  if( err == 4 ) { /*JK*/
    if(_DEBUG > 0)
      printf("Device uninitialized - Starting dbase_init2():\n");
    return dbase_init2(dev);
  }

  /* Already awake - init done */
  else if ( err == 0 ){
     if (_DEBUG > 0)
        printf("dbase_init() - Device already awake.\n\n");
     return dbase_init3(dev);
  }
  /* Something's not right */
  return -1;
}

/* 
   New Connection - send firmware packages
*/
int dbase_init2(libusb_device_handle *dev){
  int k, err, written;
  unsigned char buf0[4] = {[0] = START2, [2] = 2};  /* JK, start_msg buffer */
  
  /* PHASE II */
  if(_DEBUG > 0)
    printf("\n\nInit - PHASE II:\n");
  
  /* START2 command */
  err = dbase_write_init(dev, buf0, sizeof(buf0), &written); //HK

  if(err < 0 || written != sizeof(buf0) || dbase_checkready_init(dev) != 0x00){ /*JK*/
    fprintf(stderr, "Error sending START2 command to device - aborting!\n");
    err_str("dbase_init2(), when writing START2 command", err);
    return -1;
  }
  
  /* Get filename: 
     defined though gcc compiler option (-DPACK_PATH ...)
     in Makefile
  */
#ifndef PACK_PATH
    /* if no path is given, assume firmware in . */
    char str[] = "digiBaseRH.rbf"; /*JK*/ 
#else
    char str[strlen(PACK_PATH) + 32];
    snprintf(str, sizeof(str), "%s/digiBaseRH.rbf", PACK_PATH); /*JK*/ 
#endif

  /* Write packs 1,2 */ /*JK, digiBaseRH - 2 packs of firmware*/
  for(k = 0; k < 2; k++){
 
    /* Get binary data from file */
    int len;
    unsigned char *buf = dbase_get_firmware_pack(str, k, &len);
    if(buf == NULL){
      fprintf(stderr, "E: unable to read firmware - aborting\n");
      return -EIO;
    }

    /* Write pack k to dbase */
    err = dbase_write_init(dev, buf, len, &written); /*JK*/
   
    /* Free memory */
    free(buf);

    if(err < 0){
      err_str("dbase_init2(), when writing package", err);
      return err;
    }
    if(written != len){
      fprintf(stderr, "E: written=%d, read from file=%d\n", written, len);
      return -EIO;
    }
    if(dbase_checkready_init(dev) != 0){ /*JK*/
      fprintf(stderr, "E: sending pack%d to device - aborting!\n", k+1);
      return -EIO;
    }
 
  /*JK, dummy package used for digiBase
        digiBaseRH seems not to use it --> commented out */  
  /*   if( k == 1 ){ */
         /* Write dummy pack (NULL) */
  /*      dbase_write(dev, NULL, 0, &written); */
  /*      if(err < 0 || dbase_checkready(dev) != 0x01){ */
  /*        err_str("dbase_init2(), when writing dummy package", err); */
  /*	fprintf(stderr, "E: sending dummy pack to device - aborting!\n"); */
  /*        return -EIO; */
  /*      } */
  /*      if(_DEBUG > 0) */
  /*        printf("Wrote dummy package to device.\n"); */
  /*    } */

  }/*JK, end of for loop*/
  
  /* Write start */
  buf0[0] = START; /*JK*/
  err = dbase_write_init(dev, buf0, sizeof(buf0), &written); /*JK*/

  /* Confirm */
  if( err < 0 || written != sizeof(buf0) || dbase_checkready_init(dev) != 0){ /*JK*/
    fprintf(stderr, "E: sending START command to device - aborting!\n");
      err_str("dbase_init2(), when writing START command", err);
      return -EIO;
  }

  /* JK, START3 msg for digiBaseRH*/
  buf0[0] = START3; /*JK*/
  err = dbase_write_init(dev, buf0, sizeof(buf0), &written); /*JK*/

  /* Confirm */
  if( err < 0 || written != sizeof(buf0) || dbase_checkready_init(dev) != 0){
    fprintf(stderr, "E: sending START3 command to device - aborting!\n");
    err_str("dbase_init2(), when writing START3 command", err);
    return -EIO;
  }

  
  /* PHASE III: */
  if(_DEBUG > 0)
    printf("\n\nInit - PHASE III:\n");

  /* 
    Write status to digiBaseRH, 7 msgs total: 
      - No1 msg from predefined _STAT
      - 2times No2 msg from _STAT 
      - one string 0x04 = START2, 
      - No2 msg from _STAT with changed [77] byte, clear counter
      - 2times No2 msg from _STAT
  */

  unsigned char *_stat = (unsigned char*) _STAT;
  const int sz = (int) sizeof(status_msg) + 1;
  unsigned char buf[81];  /* status_msg buffer */

  for(k = 0; k < 7; k++) {
      memcpy(buf, _stat, sz);       // _STAT is const

     if(k == 4){  /*JK*/
       if(_DEBUG) printf("setting clr bit\n");
       buf[77] |= (uint8_t) 0x01;
     }
      if(k == 5){  /*JK*/
	if(_DEBUG) printf("clearing clr bit\n");
	buf[77] &= (uint8_t) 0xfe;
      }
//JK      if(k == 4){
//JK      /* 2012-02-27: clr l/r tp bits */
//JK	if(_DEBUG) printf("clearing ltp/rtp in ctrl byte\n");
//JK	buf[1] &= (uint8_t) LRTP_OFF_MASK;    
//JK      }

      if (k != 3) /*JK*/
         err = dbase_write(dev, buf, sz, &written);
      else   /*JK, 4th msg is one byte*/
         err = dbase_write_one(dev, START2, &written);
         
      if (k == 0) 
         _stat += sz; /* Move pointer to next status_msg in _STAT */
 
      if (err < 0 || written != sz)
         {
          if (written != 1) {               /*JK, 4th msg is one byte long*/
             fprintf(stderr, "E: writing status package %d, written = %d\n", k+1, written);
             err_str("dbase_init2(), when writing status package", err);
             return err < 0 ? err : -EIO;
             }    /*JK*/
          } 

       if (dbase_checkready(dev) != 0){ /*JK, digiBaseRH NULL replay after STAT msgs*/
          fprintf(stderr, "E: when sending pack%d command to device - aborting!\n", k+1);
          return -EIO;
          }
       
    } /*JK -- end of for loop*/

  /* Finally, clear spectrum */
  err = dbase_send_clear_spectrum(dev);
  if(err < 0){
    fprintf(stderr, "E: failed to send clear spectrum in init()\n");
    err_str("dbase_init2(), when writing clear spectrum", err);
    return err;
  }


   /* JK, check msg -- do not know if it is required */
   buf0[0] = 0x12;
   buf0[2] |= 0x04;
   err = dbase_write_init(dev, buf0, sizeof(buf0), &written);

   if (err < 0 || written != sizeof(buf0) || dbase_checkready_init(dev) != 0x03){ 
      fprintf(stderr, "E: When writing check command (err=%d, written=%d)\n", err, written);
      err_str("dbase_init2(), when writing check command", err);
      return -EIO;
   }
   

  for(k = 0; k < 2; k++) {

      if(k == 1){  /*JK*/
        if(_DEBUG) printf("setting CNT byte\n");
        buf[77] |= (uint8_t) 0x04;
      } 

      err = dbase_write(dev, buf, sz, &written);

      if (err < 0 || written != sz) {
         fprintf(stderr, "E: writing status package %d (2nd batch), written = %d\n", k+1, written);
         err_str("dbase_init2(), when writing status package", err);
         return err < 0 ? err : -EIO;
      } 

      if (dbase_checkready(dev) != 0){ /*JK, digiBaseRH NULL replay after STAT msgs*/
         fprintf(stderr, "E: when sending pack %d (2nd batch) command to device - aborting!\n", k+1);
         return -EIO;
      }
       
  }

/*JK -- section to be deleted, used for development */
/*  status_msg tmp_stat;*/
/*  for (k = 0; k < 0; k++){*/ /*JK*/
/*      dbase_get_status(dev, &tmp_stat);*/ /*JK*/
/*      dbase_print_msg((unsigned char*)&tmp_stat, sizeof(tmp_stat));*/ /*JK*/
/*  }*/ /*JK*/

  return 1;

 
}  /* JK, end of dbase_init2()*/



  /* JK last init part for both initialized and awake status */

int dbase_init3(libusb_device_handle *dev){
  int err, written, k;
  unsigned char buf0[4] = {[0] = 0x12, [2] = 0x06};  /* JK, check_msg buffer */
  status_msg tmp_stat;

  /* JK, read status 3times. Sometimes 1-2 incomplete responses, 3rd is always complete!
    (timeout, 16 bytes instead of 80). Increase of read timeout S_TIMEOUT does not help. 
    TODO: find out why
  */
     for (k = 0; k < 3; k++){ /*JK*/
         printf("Getting of digiBaseRH status (No %d/3).\n",(k+1));/*JK*/
         dbase_get_status(dev, &tmp_stat);   /*JK*/
         dbase_print_msg((unsigned char*)&tmp_stat, sizeof(tmp_stat)); /*JK*/
     } /*JK*/

  /*JK, check_msg -- do not know if it is required */
   err = dbase_write_init(dev, buf0, sizeof(buf0), &written);

   if (err < 0 || written != sizeof(buf0)){ 
      fprintf(stderr, "E: When writing check command (err=%d, written=%d)\n", err, written);
      err_str("dbase_init3(), when writing check command", err);
   }
 
   if ( dbase_checkready_init(dev) != 3 )  /* last byte from 6 total*/
     return -1;


  for(k = 0; k < 2; k++) {

      if(_DEBUG) printf("setting CNT byte\n");

      if(k == 0)  /*JK*/
        tmp_stat.CNT &= (uint8_t) 0x00;
      else
        tmp_stat.CNT |= (uint8_t) 0x04;

      err = dbase_write_status(dev, &tmp_stat);

      if (err < 0) {
         fprintf(stderr, "E: writing status package %d", k+1);
         err_str("dbase_init3(), when writing status package", err);
         return err < 0 ? err : -EIO;
      } 
  }

/*JK -- section to be deleted, used for development */
/*     for (k = 0; k < 0; k++){*/ /*JK*/
/*         dbase_get_status(dev, &tmp_stat);*/ /*JK*/
/*         dbase_print_msg((unsigned char*)&tmp_stat, sizeof(tmp_stat));*/ /*JK*/
/*     }*/ /*JK*/

  return 1;
}



/* Get firmware pack 0 <= n <= 1 */ /*JK, 2 packs for digiBaseRH*/
unsigned char* dbase_get_firmware_pack(const char* file, int n, int *len)
{
  if(file == NULL || n > 1 || n < 0)/*JK*/
    return NULL;

  /* fw packet sizes */
  const int const pzs[2] = { 61424, 14039 }; /*JK*/

  /* Open fw file */
  FILE * fh = fopen(file, "rb");
  if(fh == NULL){
    fprintf(stderr, 
	    "E: dbase_get_firmware_pack() Unable to open digiBaseRH.rbf file; %s (%s)\n%s\n", /*JK*/
	    file, 
	    strerror(errno),
	    "\tYou should set the right path in Makefile and recompile!"
	    );
    return NULL;
  }

  /* Read in packet n */
  int k, io, pos=0;
  for(k=0; k < n; k++)
    pos += pzs[k];
  unsigned char *buf = (unsigned char*) malloc (1024 * 62);
  if(buf == NULL){
    fprintf(stderr, "E: get_firmware_pack(): Out of memory()\n");
    fclose(fh);
    return NULL;
  }
  /* copy bytes inp -> buf */
  fseek(fh, pos, SEEK_SET);
  io = fread(buf+4, 1, pzs[n], fh);/*JK, 4 bytes inserted at the beginning of fw blob*/
  buf[0] = 0x05;
  buf[1] = buf[3] = 0x00;
  buf[2] = 0x02;

  if(io != pzs[n]){
    fprintf(stderr,
            "E: dbase_get_firmware_pack(): Unable to read file %s, only read %d bytes (%s)\n",
            file, io, strerror(errno));
    fclose(fh);
    return NULL;
  }
  /* Set length */
  *len = io + 4;/*JK*/
  if(_DEBUG > 0){
    fprintf(stderr, "Read in package (%d/2): %d bytes read\n", k+1, io);/*JK*/
  }

  /* success */
  fclose(fh);
  return buf;
}

/*
  Locate digiBASE on usb bus and return device handle

  - serial will be assigned with dbase's serial number
  - given_serial can be -1 for first device, otherwise
    the device with serial number 'given_serial' will be
    returned.
*/
libusb_device_handle* dbase_locate(int *serial, 
				   int given_serial, 
				   libusb_context *cntx) 
{
  libusb_device **list, *found = NULL;  /* temp list and device */
  ssize_t cnt = libusb_get_device_list(cntx, &list);
  if(cnt < 0)
    return NULL;
  if(_DEBUG > 0)
    printf("Found %d usb devices\n", (int)cnt);
  ssize_t i;
  int err;

  struct libusb_device_descriptor desc; /* device descriptor struct */
  libusb_device_handle *handle = NULL;  /* digibase handle */

  /* Iterate over found usb devices until digibase is found */
  for(i = 0; i < cnt; i++)
    {
      libusb_device *device = list[i];
      err = libusb_get_device_descriptor(device, &desc);
      /* Is the device a dbase? */
      if(err == 0 && 
	 desc.idVendor == VENDOR_ID &&
	 desc.idProduct == PROD_ID)
	{
	  if(_DEBUG > 0){
	    printf("Device %d was digiBaseRH\n", (int)i);/*JK*/
	    printf("Opening device for serial no aquisition\n");
	  }
	  /* Open libusb connection: assign handle */
	  if((err = libusb_open(device, &handle)) < 0){
	    fprintf(stderr, "E: dbase_get_serial() Error opening device\n");
	    libusb_free_device_list(list, 1);
	    return NULL;
	  }
	  /* Return first dbase */
	  if(given_serial == -1){
	    found = device;
	    err = dbase_get_serial(found, handle, desc, serial);
	    if(err < 0)
	      fprintf(stderr, "W: dbase_get_serial returned %d\n", err);
	    break;
	  }
	  /* Check if this dbase has the correct serial no */
	  else{
	    int tmp_serial = -1;
	    err = dbase_get_serial(device, handle, desc, &tmp_serial);
	    if(err < 0){
	      /* bail */
	      libusb_close(handle);
	      libusb_free_device_list(list, 1);
	      return NULL;
	    }
	    if(given_serial == tmp_serial){
	      found = device;
	      *serial = tmp_serial;
	      break;
	    }
	  }
	  /* 
	     Not the correct dbase 
	     close connection and check next device
	  */
	  libusb_close(handle);
	}
      else if(err < 0)
	err_str("libusb_get_device_descriptor()", err);
    }
  
  /* free libusb list, tell libusb to unref devices */
  libusb_free_device_list(list, 1);
  /* return dbase handle */
  return handle;
}

/* Returns the serial number for a located digibase */
int dbase_get_serial(libusb_device *dev, 
		     libusb_device_handle *handle,
		     struct libusb_device_descriptor desc,
		     int *serial){
  int err;
  unsigned char str[16];
  /* Get iSerialNumber string descriptor */
  err = libusb_get_string_descriptor_ascii(
					   handle, 
					   desc.iSerialNumber,
					   str,
					   sizeof(str)
					   );
  /* Actually the len of str here... */
  if(err > 0){
    *serial = atoi((char*)str);
    if(_DEBUG > 0)
      printf("Got serial: %d\n", *serial);
  }
  else if(err < 0){
    fprintf(stderr, "E: dbase_get_serial(): unable to aquire serial no\n");
    err_str("dbase_get_serial()", err);
    return err;
  }
  return 0;
}

/*
  Basic consistency check 
*/
int check_detector(const detector *det, const char * func){
  if(det == NULL){
    fprintf(stderr, "E: %s(): detector pointer was null\n", 
	    func == NULL ? "" : func);
    return -1;
  }
  if(det->dev == NULL){
    fprintf(stderr, "E: %s(): detector->libusb_device_handle was null\n",
	    func == NULL ? "" : func);
    return -1;
  }
  return 0;
}

/*
  getline(3) needed below isn't on MAC OS, define it here...
 
  This code is public domain -- Will Hartung 4/9/09 
*/
//#ifdef __APPLE__
size_t my_getline(char **lineptr, size_t *n, FILE *stream) {
  char *bufptr = NULL;
  char *p = bufptr;
  size_t size;
  int c;

  if (lineptr == NULL || stream == NULL || n == NULL) {
    return -1;
  }
  bufptr = *lineptr;
  size = *n;

  c = fgetc(stream);
  if (c == EOF) {
    return -1;
  }
  if (bufptr == NULL) {
    bufptr = malloc(128);
    if (bufptr == NULL) {
      return -1;
    }
    size = 128;
  }
  p = bufptr;
  while(c != EOF) {
    if ((p - bufptr) > (size - 1)) {
      size = size + 128;
      bufptr = realloc(bufptr, size);
      if (bufptr == NULL) {
	return -1;
      }
    }
    *p++ = c;
    if (c == '\n') {
      break;
    }
    c = fgetc(stream);
  }

  *p++ = '\0';
  *lineptr = bufptr;
  *n = size;

  return p - bufptr - 1;
}
//#endif


/* 
   Read in status meassage from text file 
*/
int parse_status_lines(status_msg *status, FILE *fh){
  size_t len = 63;
  char *line = (char *) malloc ( len + 1 );      /* Line buffer */

  float tt, tt2; 
  int t, t2, t3, llen, lineno=0;

  /* sscanf() buffer, risk of overflow in sscanf() later... */
  const int buflen = 64;
  char str[buflen];  

  while( (llen = my_getline(&line, &len, fh)) >= 0 && lineno < 19)
    {
      /* Check line length against str buffer size */
      if(llen > buflen) {
	fprintf(stderr, 
	       "E: Risk of overflow in parse_status_lines()\n\t buflen=%d getline() read %d bytes!\n",
		buflen, llen);
	free(line);
	return -1; /* bail before corrupting memory */
      }
      switch(lineno)
	{
	case 4 :
	  sscanf(line, "RTP on     : %s]", str);
	  if(strcmp(str, "Yes") == 0)
	    status->CTRL |= (uint8_t) RTP_MASK;
	  else
	    status->CTRL &= (uint8_t) RTP_OFF_MASK;
	  if(_DEBUG) printf("Parsed RTP EN to: %s\n", str);
	  break;
	case 5 :
	  sscanf(line, "LTP on     : %s]", str);
	  if(strcmp(str, "Yes") == 0)
	    status->CTRL |= (uint8_t) LTP_MASK;
	  else
	    status->CTRL &= (uint8_t) LTP_OFF_MASK;
	  if(_DEBUG) printf("Parsed LTP EN to: %s\n", str);
	  break;
	case 6 :
	  sscanf(line, "Gain stab. : %s", str);
	  if(strcmp(str, "Yes") == 0)
	    status->CTRL |= (uint8_t) GS_MASK;
	  else
	    status->CTRL &= (uint8_t) GS_OFF_MASK;
	  if(_DEBUG) printf("Parsed GS EN to: %s\n", str);
	  break;
	case 7 :
	  sscanf(line, "Zero stab. : %s", str);
	  if(strcmp(str, "Yes") == 0)
	    status->CTRL |= (uint8_t) ZS_MASK;
	  else
	    status->CTRL &= (uint8_t) ZS_OFF_MASK;
	  if(_DEBUG) printf("Parsed ZS EN to: %s\n", str);
	  break;
	case 10 :
	  sscanf(line, "HV Target  : %d V", &t);
	  status->HVT = (int)(t / HV_FACTOR);
	  if(_DEBUG) printf("Parsed HVT: %d -> %d\n", t, status->HVT);
	  break;
	case 11:
	  sscanf(line, "Pulse width: %f us", &tt);
	  status->PW = GET_INV_PW(tt);
	  if(_DEBUG) printf("Parsed PW: %0.2f -> %d\n", tt, status->PW);
	  break;
	case 12:
	  //sscanf(line, "Fine Gain  : %f ", &tt);
	  //status->FGN = tt * GAIN_FACTOR;
	  sscanf(line, "Fine Gain  : %f (set: %f)\n", &tt, &tt2);
	  status->FGN = GET_GAIN_VALUE(tt);
	  if(_DEBUG) printf("Parsed FG: %0.5f -> %d\n", tt, status->FGN);
	  break;
	case 13:
	  sscanf(line, "Live Time Preset  : %f s", &tt);
	  status->LTP = GET_TICKS(tt); 
	  if(_DEBUG) printf("Parsed LTP: %0.2f -> %d\n", tt, status->LTP);
	  break;
	case 15:
	  sscanf(line, "Real Time Preset  : %f s", &tt);
	  status->RTP = GET_TICKS(tt); 
	  if(_DEBUG) printf("Parsed RTP: %0.2f -> %d\n", tt, status->RTP);
	  break;
	case 17:
	  sscanf(line, "Gain Stab. chans  : [%d %d %d]", &t, &t2, &t3);
	  status->GSL = t; status->GSC=t2;status->GSU=t3;
	  if(_DEBUG) printf("Parsed GAIN: %d %d %d\n", status->GSL, status->GSC, status->GSU);
	  break;
	case 18:
	  sscanf(line, "Zero Stab. chans  : [%d %d %d]", &t, &t2, &t3);
	  status->ZSL=t;status->ZSC=t2;status->ZSU=t3;
	  if(_DEBUG) printf("Parsed GAIN: %d %d %d\n", status->ZSL, status->ZSC, status->ZSU);
	  break;
	default : break;
	}
      lineno++;
    }

  free(line);
  return 0;
}

/* 
   Print libusb errors 
*/
void err_str(const char* func, enum libusb_error err){
  const char* err_msg = NULL;
  switch(err)
  {
    case LIBUSB_SUCCESS:
      if(_DEBUG > 0)
        printf("LUSB: OK (%s)\n", func == NULL ? "" : func);
      return;
    case LIBUSB_ERROR_IO:
      err_msg = "LUSB_E: IO error";
      break;
    case LIBUSB_ERROR_ACCESS:
      err_msg = "LUSB_E: Access error";
      break;
    case LIBUSB_ERROR_NO_DEVICE:
      err_msg = "LUSB_E: No device error";
      break;
    case LIBUSB_ERROR_NOT_FOUND:
      err_msg = "LUSB_E: Not found error";
      break;
    case LIBUSB_ERROR_BUSY:
      err_msg = "LUSB_E: Device busy error";
      break;
    case LIBUSB_ERROR_TIMEOUT:
      err_msg = "LUSB_E: Timeout error";
      break;
    case LIBUSB_ERROR_OVERFLOW:
      err_msg  ="LUSB_E: Overflow error";
      break;
    case LIBUSB_ERROR_PIPE:
      err_msg = "LUSB_E: Pipe error";
      break;
    case LIBUSB_ERROR_INTERRUPTED:
      err_msg = "LUSB_E: Interrupted error";
      break;
    case LIBUSB_ERROR_NO_MEM:
      err_msg = "LUSB_E: No memory error";
      break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
      err_msg = "LUSB_E: Not supported error";
      break;
    case LIBUSB_ERROR_OTHER:
      err_msg = "LUSB_E: Other error";
      break;
    default:
      err_msg = "LUSB_E: Unknown error";
      break;
  }
  if(err_msg != NULL){
    if(func != NULL)
      fprintf(stderr, "LUSB_E in function %s:\n", func);
    fprintf(stderr, "%s\n", err_msg);
  }
}

/*
  Big endian conversion functions
 
  Swap byte order in status struct 
*/
void dbase_byte_swap_status_struct(status_msg *stat)
{
  /* 2012-02-27, we'll swap these as well */
  BYTESWAP(stat->TMR); 
  BYTESWAP(stat->AFGN);

  BYTESWAP(stat->FGN);
  BYTESWAP(stat->LLD);
  BYTESWAP(stat->LTP);
  BYTESWAP(stat->LT);
  BYTESWAP(stat->RTP);
  BYTESWAP(stat->RT);
  BYTESWAP(stat->LEN);
  BYTESWAP(stat->HVT);
  BYTESWAP(stat->GSU);
  BYTESWAP(stat->GSC);
  BYTESWAP(stat->GSL);
  BYTESWAP(stat->ZSU);
  BYTESWAP(stat->ZSC);
  BYTESWAP(stat->ZSL);
}

/* swap byte-order in b */
void dbase_switch_endian(unsigned char *b, int n)
{
  register int i = 0;
  register int j = n-1;
  char c;
  while (i < j)
  {
    c = b[i];
    b[i] = b[j];
    b[j] = c;
    i++, j--;
  }
}


/* JK -- print n bytes of msg, DEBUG purpose */
void dbase_print_msg(unsigned char *string, int n)
{
  int k;
  if (_DEBUG > 0){
     printf("====================\n");
     for (k = 0; k < n; k++){
         printf("%02x ",string[k]);
         if ((k+1) % 8 == 0)
            printf(" ");
         if ((k+1) % 16 == 0)
            printf("\n");
     }
     printf("====================\n\n");
  }
}
