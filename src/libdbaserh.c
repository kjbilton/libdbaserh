/*
 * libdbaserh.c: Library to communicate with ORTEC's digiBaseRH over USB
 * 
 * adapted from libdbase.c for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * libdbase.c: Library to communicate with ORTEC's digiBASE over USB
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

#include <errno.h>     /* error codes */
#include <stdio.h>     /* printf(), fprintf(), ... */
#include <stdlib.h>    /* malloc(), calloc(), free(), ... */
#include <string.h>    /* memcpy() */
#include <sys/stat.h>  /* mkdir(), ...*/

/* libdbase non-public header */
#include "libdbaserhi.h" /*JK*/

/*
  Endianess
*/
int big_endian_test = 1;
/* 
   The libusb context 
*/
libusb_context *cntx = NULL;
/* 
   Number of initialized dbases 
*/
int detectors = 0;

/*
  Initialize all digibases found on the system
  - returns NULL terminated detector* array and
    number of found* dbases
*/
int libdbase_get_list(detector *** list, int *found) {
  /* Allow up to 32 dbases, should be 'nuf */
  int LEN = 32, serials[LEN], k;
    
  /* Get all dbase(s) serial numbers */
  *found = libdbase_list_serials(serials, LEN);
  if(_DEBUG)
    printf("libdbase_list_init(): Found %d digiBaseRH(s)\n", *found);/*JK*/

  if(*found == 0)
    return 0;  /* nothing found, no errors */
  if(*found < 1){
    k=(*found); *found = 0;
    return k;  /* nothing found, return error */
  }

  /* allocate dbase handles list */
  list[0] = (detector**) malloc ((*found+1) * sizeof(detector*));
  if(*list == NULL) { 
    fprintf(stderr, 
	    "E: libdbase_get_list(): Out of memory when allocating *det-list\n\tfound was %d\n", 
	    *found); 
    return -ENOMEM;
  }
  /* null-terminate list */
  list[0][*found] = NULL;

  /* find and open each dbase */
  for(k=0; k < *found; k++){
    /* each detector* is allocated within libdbase_init() */
    if( (list[0][k] = libdbase_init(serials[k])) == NULL){
      printf("E: libdbase_list_init() when opening detector %d with serial %d\n", 
	     k+1, serials[k]);
      return -1;
    }
    else if(_DEBUG)
      printf("Initialized detector %d: Serial %d\n", k+1, serials[k]);
  }
  return 0;
}

/* 
   Close connection to each detector and free list 
   - this function calls libdbase_close() on each 
     detector* in list.
*/
int libdbase_free_list(detector *** list){
  if(list == NULL || list[0] == NULL)
    return -1;
  int k=0, err=0;
  
  /* Iterate over whole detector-list */
  while( list[0][k] != NULL ) {
    /* each detector pointer is freed here... */
    if( (err = libdbase_close(list[0][k])) < 0 )
      fprintf(stderr, "libdbase_free_list(): when closing detector %d\n", k+1);
    else if(_DEBUG)
      printf("detector pointer %d in list freed successfully \n", k+1);
    k++;
  }
  /* ...and the detector array pointer here */
  free( list[0] );
  return err;
}


/* 
   Initialize usb device and, 
   if found, initialize first detector 
*/
detector* libdbase_init(int dbase_serial){
  
  /* (lib)usb device handle */
  struct libusb_device_handle *dev_handle;
  int err, serial = -1;
  
  /* Initialize usb */
  if(cntx == NULL){
    err = libusb_init(&cntx);
    if(_DEBUG > 0)
      printf("Initializing libusb_context (%d)\n", dbase_serial);
    if(err < 0){
      err_str("libusb_init()", err);
      return NULL;
    }
  }

  /* Locate digiBASE */
  if(_DEBUG > 0){
    printf("Locating %s digiBaseRH (%d) on usb bus\n", /*JK*/
	   dbase_serial == -1 ? "first" : "", dbase_serial);
  }
  dev_handle = dbase_locate(&serial, dbase_serial, cntx);
  if(dev_handle == NULL){
    fprintf(stderr, "E: libdbase_init() Unable to find and open digiBaseRH\n");/*JK*/
    return NULL;
  }
  
  /* Allocate the detector struct */
  detector *det = (detector *) malloc( sizeof(detector) );
  if(det == NULL){
    fprintf(stderr, "E: Unable to allocate memory for detector struct\n");
    return NULL;
  }
  
  /* Assign serial number...*/ 
  if(serial > -1) {
    det->serial = serial;
    if(_DEBUG > 0)
      printf("Found digiBaseRH with Serial #: %d\n", det->serial);/*JK*/
  }
  /* ...and libusb device_handle */
  det->dev = dev_handle;
  
  /* Set configuration and claim interface with OS */
  if( (err = libusb_set_configuration(dev_handle, 1)) < 0){
    err_str("libusb_set_configuration()", err);
    return NULL;
  }
  if( (err = libusb_claim_interface(dev_handle, 0)) < 0){
    err_str("libusb_claim_interface()", err);
    return NULL;
  }
  if( (err = libusb_set_interface_alt_setting(dev_handle, 0, 0)) < 0){
    err_str("libusb_set_interface_alt_setting()", err);
    return NULL;
  }
  
  /* Initialize dbase */
  if( (err = dbase_init(dev_handle)) > -1){
    /* Assign status struct */
    err = dbase_get_status(det->dev, &det->status);
    if( err < 0 )
      err_str("dbase_get_status()", err);
  }
  else{
    err_str("dbase_init()", err);
    return NULL;
  }

  /* Initialize spec and last_spec to zeros */
  for( err = 0; err < DBASE_LEN + 1; err++){
    det->spec[err] = 0;
    det->last_spec[err] = 0;
  }

  /* Increase the number of dbase's on the system */
  detectors++;
  if(_DEBUG > 0)
    printf("Open: detectors in libdbase: %d\n", detectors);
  
  /* All is fine, return detector pointer */
  return det;
}

/* 
   Release resources used by detector struct 
   and close underlying libusb connection
*/
int libdbase_close(detector *det){
  if( check_detector(det, "libdbase_close()") < 0) 
    return -1;
  
  /* Release interface */
  int close_status = libusb_release_interface(det->dev, 0);
  if(_DEBUG > 0) 
    fprintf(stderr, "freeing det...");
  if(close_status < 0)
    err_str("\nlibdbase_close()", close_status);  
  
  /* Close libusb handle */
  libusb_close(det->dev);
  /* Remove the detector */
  detectors--;
  if(_DEBUG > 0)
    printf("Close: detectors in libdbase: %d\n", detectors);

  /* if it's the last one, exit libusb */
  if(detectors == 0){
    if(_DEBUG > 0)
      printf("Last detector closed, calling libusb_exit()\n");
    libusb_exit(cntx);
  }
  
  /* Free detector struct */
  free(det); det = NULL;
  if(_DEBUG > 0) 
    printf("...done\n");
  
  /* Return libusb's release status */
  return close_status;
}

/*   Digibase Control functions:


  Start measurement
*/
int libdbase_start(detector *det){
  if( check_detector(det, "libdbase_start") < 0) return -1;

  /* Set start bit */
  det->status.CTRL |= (uint8_t) ON_MASK;

  if(_DEBUG > 0)
    printf("\nStarting detector: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);
}

/*
  Stop measurement
*/
int libdbase_stop(detector *det){
  if( check_detector(det, "libdbase_stop") < 0) return -1;

  /* Clear start bit */
  det->status.CTRL &= (uint8_t) OFF_MASK;

  if(_DEBUG > 0)
    printf("\nStopping detector: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);
}

/*
  Turn on Gain stabilization
  will be set to channels given in
  det->status (GSL, GSC, GSU)
*/
int libdbase_gs_on(detector *det){
  if( check_detector(det, "libdbase_gs_on") < 0) return -1;

  /* Set gs bit */
  det->status.CTRL |= (uint8_t) GS_MASK;

  if(_DEBUG > 0)
    printf("\nTurning GS on: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);
}

/*
  Turn off Gain stabilization
*/
int libdbase_gs_off(detector *det){
  if( check_detector(det, "libdbase_gs_off") < 0) return -1;

  /* Clear gs bit */
  det->status.CTRL &= (uint8_t) GS_OFF_MASK;

  if(_DEBUG > 0)
    printf("\nTurning GS off: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);
}

/*
  Turn on Zero stabilization
  will be set to channels given in
  det->status (ZSL, ZSC, ZSU)
*/
int libdbase_zs_on(detector *det){
  if( check_detector(det, "libdbase_zs_on") < 0) return -1;

  /* Set zs bit */
  det->status.CTRL |= (uint8_t) ZS_MASK;

  if(_DEBUG > 0)
    printf("\nTurning ZS on: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);  
}

/*
  Turn off Zero stabilization
*/
int libdbase_zs_off(detector *det){
  if( check_detector(det, "libdbase_zs_off") < 0) return -1;

  /* Clear zs bit */
  det->status.CTRL &= (uint8_t) ZS_OFF_MASK;

  if(_DEBUG > 0)
    printf("\nTurning ZS off: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);  
}

/*
  Turn High Voltage on
  will be set to det->status.HVT
*/
int libdbase_hv_on(detector *det){
  if( check_detector(det, "libdbase_hv_on") < 0) return -1;
  if(det->status.HVT > (uint16_t) MAX_HV){
    printf("E: hv setting (%d) exceeds maximum (%d)", det->status.HVT, MAX_HV);
    return -1;
  }

  /* Set hv bit */
  det->status.CTRL |= (uint8_t) HV_MASK;

  if(_DEBUG > 0)
    printf("\nTurning HV on: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);  
}

/* 
   Turn High Voltage off 
*/
int libdbase_hv_off(detector *det){
  if( check_detector(det, "libdbase_hv_off") < 0) return -1;
  
  /* Clear hv bit */
  det->status.CTRL &= (uint8_t) HV_OFF_MASK;

  if(_DEBUG > 0)
    printf("\nTurning HV off: CTRL=0x%02x\n", (unsigned char)det->status.CTRL);

  /* Write changes to device */
  return dbase_write_status(det->dev, &det->status);
}

/*
  Settings Functions:

  Set high voltage, argument given in volts (V) 
*/
int libdbase_set_hv(detector *det, ushort hv){
  if( check_detector(det, "libdbase_set_hv") < 0) return -1;
  if( hv > (ushort) MAX_HV ){
    fprintf(stderr, "E: given high voltage (%hu) exceeds maximum (%d)\n", hv, MAX_HV);
    return -1;
  }

  /* set high voltage */
  det->status.HVT = (uint16_t) (hv / HV_FACTOR);

  if(_DEBUG > 0)
    printf("\nHigh voltage target set: %d V\n", (int) (det->status.HVT * HV_FACTOR));

  return dbase_write_status(det->dev, &det->status);
}

/* 
   Set Real time preset, argument given in seconds (s) 
*/
int libdbase_set_rtp(detector *det, double real_time_preset){
  if( check_detector(det, "libdbase_set_rtp") < 0) return -1;
  if( real_time_preset < 0.0){
    fprintf(stderr, "E: real_time_preset must be a positive number (was %f)\n", 
	    real_time_preset);
    return -1;
  }
  /* set real time ticks */
  det->status.RTP = GET_TICKS(real_time_preset);
  /* set rtp bit */
  det->status.CTRL |= (uint8_t) RTP_MASK;

  if(_DEBUG > 0) {
    printf("\nReal time preset set: %d (ticks) %0.2f (s)\n", 
	   det->status.RTP, real_time_preset);
  }
  return dbase_write_status(det->dev, &det->status);
}

/* 
   Set Live time preset, argument given in seconds (s) 
*/
int libdbase_set_ltp(detector *det, double live_time_preset) {
  if( check_detector(det, "libdbase_set_ltp") < 0) return -1;
  if( live_time_preset < 0.0){
    fprintf(stderr, "E: live_time_preset must be a positive number (was %f)\n", 
	    live_time_preset);
    return -1;
  }
  /* set live time ticks */
  det->status.LTP = GET_TICKS(live_time_preset);
  /* set ltp bit */
  det->status.CTRL |= (uint8_t) LTP_MASK;

  if(_DEBUG > 0)
    printf("\nLive time preset set: %d (ticks) %0.2f (s)\n", 
	   det->status.LTP, live_time_preset);

  return dbase_write_status(det->dev, &det->status);
}

/* 
   Disable live time preset 
*/
int libdbase_set_ltp_off(detector *det) {
  if( check_detector(det, "libdbase_set_ltp_off") < 0) 
    return -1;
  if(_DEBUG > 0) 
    printf("\nLTP disabled\n");

  /* clear ltp bit */
  det->status.CTRL &= (uint8_t) LTP_OFF_MASK;

  return dbase_write_status(det->dev, &det->status);
}

/* 
   Disable real time preset 
*/
int libdbase_set_rtp_off(detector *det) {
 if( check_detector(det, "libdbase_set_rtp_off") < 0) 
   return -1;
  if(_DEBUG > 0) 
    printf("\nRTP disabled\n");

  /* clear rtp bit */
  det->status.CTRL &= (uint8_t) RTP_OFF_MASK;

  return dbase_write_status(det->dev, &det->status);
}

/*
  Clear real/live time presets 
*/
int libdbase_clear_presets(detector *det){
  if( check_detector(det, "libdbase_clear_presets") < 0) return -1;
  det->status.LTP = (uint32_t) 0;
  det->status.RTP = (uint32_t) 0;

  /* 2012-02-24: clr l/r tp bits */
  det->status.CTRL &= (uint8_t) LRTP_OFF_MASK;

  if(_DEBUG > 0)
    printf("\nPresets cleared: (%d , %d)\n",
         det->status.LTP,
         det->status.RTP);

  return dbase_write_status(det->dev, &det->status);
}

/* 
   Clear real/live time counters 
   - counter times are clear by toggling
     CNT byte (low bit)
*/
int libdbase_clear_counters(detector *det){
  if( check_detector(det, "libdbase_clear_counters") < 0) return -1;
  int err = 0;

  /* set counter bit */
  det->status.CNT |= (uint8_t) 0x01;
  err = dbase_write_status(det->dev, &det->status);
  if(err < 0){
    fprintf(stderr, "E: libdbase_clear_counters(), when setting clear bit\n");
    err_str("libdbase_clear_counters()", err);
    return err;
  }
  /* clear counter bit */
  det->status.CNT &= (uint8_t) 0xfe;
  if(_DEBUG > 0)
    printf("Counters cleared\n");
  return dbase_write_status(det->dev, &det->status);
}

/* 
   Clear presets, counters and spectrum
*/
int libdbase_clear_all(detector *det){
  int err;
  err = libdbase_clear_presets(det);
  if(err < 0){
    fprintf(stderr, "E: libdbase_clear_all() failed to clear presets\n");
    return -1;
  }
  err = libdbase_clear_counters(det);
  if(err < 0){
    fprintf(stderr, "E: libdbase_clear_all() failed to clear counters\n");
    return -1;
  }
  /* libdbase_clear_spectrum prints errors internally */
  return libdbase_clear_spectrum(det);
}


/* 
   Set gain stabilization channels 
*/
int libdbase_set_gs_chans(detector *det, ushort center, ushort width){
  if( check_detector(det, "libdbase_set_gs_chans") < 0) return -1;
  if(0 == center || center > det->status.LEN || 
    width == 0 || width > det->status.LEN)
  {
      fprintf(stderr, 
	      "E: unable to set gain stab. channels to: [%d, %d, %d]\n",
	      center - width/2,
	      center,
	      center + width/2);
      return -1;
  }
  det->status.GSL = (uint16_t) (center - width/2);
  det->status.GSC = (uint16_t)  center;
  det->status.GSU = (uint16_t) (center + width/2);
  if(_DEBUG > 0)
    printf("\nGain stab. roi set to: [%hu %hu %hu]\n", 
         det->status.GSL,
         det->status.GSC,
         det->status.GSU); 
  return dbase_write_status(det->dev, &det->status);
}

/* 
   Set zero stabilization channels 
*/
int libdbase_set_zs_chans(detector *det, ushort center, ushort width){
  if( check_detector(det, "libdbase_zs_chans") < 0) return -1;
  if(center == 0 || center > det->status.LEN || 
    width == 0 || width > det->status.LEN)
  {
      fprintf(stderr, 
	      "E: unable to set zero stab. channels to: [%hu, %hu, %hu]\n",
	      center - width/2,
	      center,
	      center + width/2);
      return -1;
  }
  det->status.ZSL = (uint16_t) (center - width/2);
  det->status.ZSC = (uint16_t)  center;
  det->status.ZSU = (uint16_t) (center + width/2);
  if(_DEBUG > 0)
    printf("\nZero stab. roi set to: [%hu %hu %hu]\n", 
	   det->status.ZSL,
	   det->status.ZSC,
	   det->status.ZSU); 
  return dbase_write_status(det->dev, &det->status);
}

/* 
   Set pulse width (shaping time), argument given in micro seconds (us) 
*/
int libdbase_set_pw(detector *det, float pulse_width){
  if( check_detector(det, "libdbase_set_pw") < 0) return -1;
  if(pulse_width < 0.75f || pulse_width > 2.0f)
  {
      fprintf(stderr, 
	      "E: pulse width out of range: %0.1f, should be in range: 0.75-2.0\n", 
	      pulse_width);
      return -1;
  }
  det->status.PW = (uint8_t) (GET_INV_PW(pulse_width));
  if(_DEBUG > 0)
    printf("\nPulse width set to: %d\n", (unsigned char) det->status.PW); 
  return dbase_write_status(det->dev, &det->status);
}

/* 
   Set fine gain 
*/
int libdbase_set_fgn(detector *det, float fg){
  if( check_detector(det, "libdbase_set_fgn") < 0) return -1;
  if(fg < 0.40f || fg > 1.1999f){
    fprintf(stderr, "E: fine gain out of range: %0.1f, should be in range: 0.4 < x < 1.2\n", fg);
    return -1;
  }
  /* 
     There is an gain factor when setting the gain,
     see GET_GAIN_VALUE macro in libdbasei.h
  */
  det->status.FGN = (uint32_t) GET_GAIN_VALUE(fg);
  //det->status.u5 = GET_GAIN_VALUE(fg);

  if(_DEBUG > 0){
    printf("\nSetting fine gain to: %0.4f (0x%06x)\n", fg, det->status.FGN);
    printf("Actual fg = 0x%06x\n", det->status.AFGN);
  }
  return dbase_write_status(det->dev, &det->status);

  /*
  int err;
  if( err > 0 && ABS(det->status.AFGN - det->status.FGN) > 100) {
    fprintf(stderr, "E: unable to set fine gain to %.5f diff=%d\n", fg, 
	    ABS(det->status.AFGN - det->status.FGN));
    err = -1;
  }
  return err;*/
}

/*
  Read one spectrum from digiBASE:
  - spectrum is stored at det->spec 
  - differential spectrum at det->last_spec
*/
int libdbase_get_spectrum(detector *det){
  if( check_detector(det, "libdbase_get_spectrum") < 0) {
    fprintf(stderr, "E: libdbase_get_spectrum(), det is NULL pointer\n");
    return -1;
  }
  return dbase_get_spectrum(det->dev, det->spec, det->last_spec);
}

/*
  Send clear spectrum to detector
*/
int libdbase_clear_spectrum(detector *det){
  if( check_detector(det, "libdbase_clear_spectrum") < 0){
    fprintf(stderr, "E: libdbase_clear_spectrum(), det is NULL pointer\n");
    return -1;
  }
  return dbase_send_clear_spectrum(det->dev);
}

/*
  Print spectrum to stdout (wrapper)
*/
void libdbase_print_spectrum(const detector *det){
  if( check_detector(det, "libdbase_print_spectrum") < 0) 
    return;
  dbase_print_spectrum_file(det->spec, det->status.LEN+1, stdout);
}

/*
  Print differential spectrum to stdout (wrapper)
*/
void libdbase_print_diff_spectrum(const detector *det){
  if( check_detector(det, "libdbase_print_diff_spectrum") < 0)
    return;
  dbase_print_spectrum_file(det->last_spec, det->status.LEN+1, stdout);
}

/*
  Print spectrum to FILE (wrapper)
*/
void libdbase_print_file_spectrum(const detector *det, FILE *fh) {
  if( check_detector(det, "libdbase_print_file_spectrum") < 0)
    return;
  dbase_print_spectrum_file(det->spec, det->status.LEN+1, fh);
}

/*
  Print differential spectrum to FILE (wrapper)
*/
void libdbase_print_diff_file_spectrum(const detector *det, FILE *fh){
  if( check_detector(det, "libdbase_print_diff_file_spectrum") < 0)
    return;
  dbase_print_spectrum_file(det->last_spec, det->status.LEN+1, fh);
}

/*
   Print binary spectrum to FILE 
*/
void libdbase_print_file_spectrum_binary(const detector *det, FILE *fh){
  if( check_detector(det, "libdbase_print_file_spectrum_binary") < 0)
    return;
  dbase_print_file_spectrum_binary(det->spec, DBASE_LEN + 1, fh);
}

/*
  Print binary differential spectrum to FILE
*/
void libdbase_print_diff_file_spectrum_binary(const detector *det, FILE *fh){
  if( check_detector(det, "libdbase_print_diff_file_spectrum_binary") < 0)
    return;
  dbase_print_file_spectrum_binary(det->last_spec, DBASE_LEN + 1, fh);
}

/*
  Print (Region of Interest) part of spectrum (and returns sum in *sum)
  - if *fh is NULL only sum is calculated
*/
void libdbase_print_roi(detector *det, uint start_ch, uint end_ch, uint *sum, FILE *fh){
  if(check_detector(det, "libdbase_print_roi") < 0 
     || start_ch > end_ch 
     || end_ch > det->status.LEN) {
    if(start_ch > end_ch)
      fprintf(stderr, "E: libdbase_print_roi() start_ch > end_ch\n");
    else if(end_ch > det->status.LEN)
      fprintf(stderr, "E: libdbase_print_roi() end_ch out of range\n");
    else
      fprintf(stderr, "E: libdbase_print_roi() det is NULL pointer\n");
    return;
  }
  int k;
  *sum = 0;
  if(_DEBUG > 0)
    printf("Channels %u:%u:\n", start_ch, end_ch);
  /* print roi channels */
  if(fh != NULL){
    for(k = start_ch; k < end_ch; k++){
      sum[0] += det->spec[k];
      fprintf(fh, "%d ", det->spec[k]);
    }
    fprintf(fh,"\n");
  }
  /* (and) sum roi */
  for(k = start_ch; k < end_ch; k++)
    sum[0] += det->spec[k];     
}

/*
  Get detector status msg
*/
int libdbase_get_status(detector *det){
  if( check_detector(det, "libdbase_get_status") < 0)
    return -1;
  return dbase_get_status(det->dev, &det->status);
}
/* low-level wrapper */
int libdbase_set_status(detector *det) {
  if( check_detector(det, "libdbase_set_status") < 0)
    return -1;
  return dbase_write_status(det->dev, &det->status);
}

/*
  MISC FUNTIONS:
*/  
/* 
   Print current status to stdout (wrapper)
*/
void libdbase_print_status(const detector *det){
  if(check_detector(det, "libdbase_print_status") < 0)
    return;
  libdbase_print_file_status(&det->status, det->serial, stdout);
}

/* 
   Print current status to file 
*/
void libdbase_print_file_status(const status_msg * status, const int serial, FILE *fh){
  if(fh == NULL){
    fprintf(stderr, "E: libdbase_print_file_status(), *fh was NULL\n");
    return;
  }
  if(status == NULL){
    fprintf(stderr, "E: libbdase_printf_file_status(), *status was null\n");
    return;
  }
  /* 
     2012-02-14
     _should_ be more efficient to print the whole char array
     at once. Construct char array using safe sprintf first.
  */
  const size_t len = 512;
  int w = 0;
  char buf[(int)len];

  /* Print header with serial number */
  w += snprintf(buf, len, "======== MCB STATUS (%d) =======\n", serial);

  /* CTRL byte information */
  w += snprintf(buf+w, len-w, "Running    : %s\n", (status->CTRL & ON_MASK) > 0 ? "Yes" : "No");
  w += snprintf(buf+w, len-w, "Mode       : %s\n", (status->CTRL & MODE_MASK) > 0 ? "PHA" : "List");
  w += snprintf(buf+w, len-w, "HV on      : %s\n", (status->CTRL & HV_MASK) > 0 ? "Yes" : "No");
  w += snprintf(buf+w, len-w, "RTP on     : %s\n", (status->CTRL & RTP_MASK) > 0 ? "Yes" : "No");
  w += snprintf(buf+w, len-w, "LTP on     : %s\n", (status->CTRL & LTP_MASK) > 0 ? "Yes" : "No");
  w += snprintf(buf+w, len-w, "Gain stab. : %s\n", (status->CTRL & GS_MASK) > 0 ? "Yes" : "No");
  w += snprintf(buf+w, len-w, "Zero stab. : %s\n\n", (status->CTRL & ZS_MASK) > 0 ? "Yes" : "No");

  /* General settings */
  w += snprintf(buf+w, len-w, "MCB Chans. : %d\n", status->LEN + 1);
  w += snprintf(buf+w, len-w, "HV Target  : %d V\n", (int)(status->HVT * HV_FACTOR));
  w += snprintf(buf+w, len-w, "Pulse width: %.2f us\n", GET_PW(status->PW));
  //w += snprintf(buf+w, len-w, "Fine Gain  : %.5f\n", GET_INV_GAIN_VALUE(status->FGN)); 
  w += snprintf(buf+w, len-w, "Fine Gain  : %.5f (set: %.5f)\n", 
		GET_INV_GAIN_VALUE(status->AFGN),
		GET_INV_GAIN_VALUE(status->FGN));
  w += snprintf(buf+w, len-w, "Live Time Preset  : %.2f s\n", status->LTP * TICKSTOSEC);
  w += snprintf(buf+w, len-w, "Live Time         : %.2f s\n", status->LT * TICKSTOSEC);
  w += snprintf(buf+w, len-w, "Real Time Preset  : %.2f s\n", status->RTP * TICKSTOSEC);
  w += snprintf(buf+w, len-w, "Real Time         : %.2f s\n", status->RT * TICKSTOSEC);
  w += snprintf(buf+w, len-w, "Gain Stab. chans  : [%hu %hu %hu]\n",
		status->GSL,
		status->GSC,
		status->GSU);
  w += snprintf(buf+w, len-w, "Zero Stab. chans  : [%hu %hu %hu]\n",
		status->ZSL,
		status->ZSC,
		status->ZSU);
  w += snprintf(buf+w, len-w, "============================\n");

  /* Print buffer to file handle */
  fprintf(fh, "%s", buf);

  if(_DEBUG)
    fprintf(fh, "Total of %d chars, buffer was %d \n", w, (int)len);

  /* old code, calling fprintf several times */
  /*fprintf(fh, "======== MCB STATUS (%d) =======\n", det->serial);
  fprintf(fh,"Running    : %s\n", (det->status.CTRL & ON_MASK) > 0 ? "Yes" : "No");
  fprintf(fh,"Mode       : %s\n", (det->status.CTRL & MODE_MASK) > 0 ? "PHA" : "List");
  fprintf(fh,"HV on      : %s\n", (det->status.CTRL & HV_MASK) > 0 ? "Yes" : "No");
  fprintf(fh,"Gain stab. : %s\n", (det->status.CTRL & GS_MASK) > 0 ? "Yes" : "No");
  fprintf(fh,"Zero stab. : %s\n\n", (det->status.CTRL & ZS_MASK) > 0 ? "Yes" : "No");
  fprintf(fh,"MCB Chans. : %d\n", det->status.LEN + 1);
  fprintf(fh,"HV Target  : %d V\n", (int)(det->status.HVT * HV_FACTOR));
  fprintf(fh,"Pulse width: %.2f us\n", GET_PW(det->status.PW));
  fprintf(fh,"Fine Gain  : %.5f \n", det->status.FGN / GAIN_FACTOR);
  fprintf(fh,"Live Time Preset  : %.2f s\n", det->status.LTP * TICKSTOSEC);
  fprintf(fh,"Live Time         : %.2f s\n", det->status.LT * TICKSTOSEC);
  fprintf(fh,"Real Time Preset  : %.2f s\n", det->status.RTP * TICKSTOSEC);
  fprintf(fh,"Real Time         : %.2f s\n", det->status.RT * TICKSTOSEC);
  fprintf(fh,"Gain Stab. chans  : [%hu %hu %hu]\n",
        det->status.GSL,
        det->status.GSC,
        det->status.GSU);
  fprintf(fh,"Zero Stab. chans  : [%hu %hu %hu]\n",
        det->status.ZSL,
        det->status.ZSC,
        det->status.ZSU);
  fprintf(fh,"============================\n"); */
}

/*
  Save status (binary)
*/
/*int libdbase_save_status(detector *det, const char* dir){
  int err;
  char path[MAX_PATH_LEN - 10];

  if( check_detector(det, "libdbase_save_status") < 0 || !det->serial){
    if(!det->serial)
      fprintf(stderr, "E: libdbase_save_status(), serial no. not set\n");
    return -1;
  }
  
  status_msg tmp;
  if( check_status_sanity(det->status, &tmp) < 0 ){
    fprintf(stderr,
	    "E: libdbase_save_status() status is corrupt - aborting\n");
    return -1;
  }

  if(dir != NULL)
    snprintf(path, sizeof(path), "%s/%d", dir, det->serial);
  else {
    if(libdbase_get_det_path(det, path, sizeof(path)) < 0){
      fprintf(stderr, "E: unable to load libdbase path\n");
      return -1;
    }

    err = mkdir(path, 
		S_IRUSR | S_IWUSR | S_IXUSR |  // 7
		S_IRGRP | S_IWGRP | S_IXGRP |  // 5
		S_IROTH | S_IXOTH              // 5
		);
    if(err != 0 && errno != EEXIST){
      fprintf(stderr, 
	      "E: libdbase_save_status() Unable to create directory: %s (err=%d)\n", 
	      path, 
	      errno);
      return -1;
    }
    if(_DEBUG > 0){
      if(errno != EEXIST)
	printf("Created directoy: %s\nstatus code mkdir: %d\n", path, err);
      else
	printf("Using existing directory %s\n", path);
    }
  }
  char file[MAX_PATH_LEN];
  snprintf(file, sizeof(file), "%s/status", path);
  if(_DEBUG > 0)
    printf("Writing status to file: %s\n", file);
  FILE* fh = fopen(file, "wb");
  if(fh == NULL){
    fprintf(stderr, "E: libdbase_save_status() Unable to open file %s\n", file);
    return -1;
  }
  err = fwrite(&tmp, 1, sizeof(status_msg), fh);
  fclose(fh);
  if(err < 0){
    fprintf(stderr,
	    "E: libdbase_save_status() Error when writing status to file %s\n", 
	    file);
  }
  return 0;
}*/

/*
  Save status as ASCII
*/
int libdbase_save_status_text(detector *det, const char* dir){
   /* Write path string */
  char path[MAX_PATH_LEN - 10];

  /* Check detector pointer and serial (int) */
  if( check_detector(det, "libdbase_save_status_text") < 0 || !det->serial){
    if( !det->serial)
      fprintf(stderr, "E: libdbase_save_status_text(), serial no. not set\n");
    return -1;
  }
  
  status_msg tmp;
  /* Check sanity of status struct (cannot be in listmode etc.) */
  if( check_status_sanity(det->status, &tmp) < 0 ){
    fprintf(stderr, "E: libdbase_save_status_text() status is corrupt - aborting\n");
    return -1;
  }

  /* Custom path */
  if(dir != NULL)
    snprintf(path, sizeof(path), "%s/%d", dir, det->serial);
  else {
    int err;
    /* 
       Save struct at compile-time defined path,
       fall back on curr dir if none was given
    */
    if(libdbase_get_det_path(det, path, sizeof(path)) < 0){
      fprintf(stderr, "E: unable to load libdbase path\n");
      return -1;
    }

    /* Create directory if it doesn't exist */
    err = mkdir(path, 
		S_IRUSR | S_IWUSR | S_IXUSR |  /* User perm,   7 */
		S_IRGRP | S_IWGRP | S_IXGRP |  /* Group perm,  7 */
		S_IROTH | S_IXOTH              /* Others perm, 5 */
		);
    /* Ignore EEXIST error */
    if(err != 0 && errno != EEXIST){
      fprintf(stderr, 
	      "E: libdbase_save_status_text() Unable to create directory: %s (err=%d)\n", 
	      path, 
	      errno);
      return -1;
    }
    if(_DEBUG > 0){
      if(errno != EEXIST)
	printf("Created directoy: %s\nstatus code mkdir: %d\n", path, err);
      else
	printf("Using existing directory %s\n", path);
    }
  }
  /* Create status file */
  char file[MAX_PATH_LEN];
  snprintf(file, sizeof(file), "%s/status.txt", path);

  if(_DEBUG > 0)
    printf("Writing status to file: %s\n", file);

  FILE* fh = fopen(file, "w");
  if(fh == NULL){
    fprintf(stderr, "E: libdbase_save_status_text() Unable to open file %s\n", file);
    return -1;
  }
  /* Write (corrected) status struct as text */
  libdbase_print_file_status(&tmp, det->serial, fh);

  /* Close file */
  fclose(fh);

  return 0;
}

/*
  Load status from save file
  if dir is NULL, status is loaded from default path.
*/
/*
int libdbase_load_status(detector *det, const char *dir){
  if( check_detector(det, "libdbase_load_status") < 0){
    return -1;
  }
  int err;
  char path[MAX_PATH_LEN - 10];
  if(dir != NULL)
    snprintf(path, sizeof(path), "%s/%d", dir, det->serial);
  else {
    if(libdbase_get_det_path(det, path, sizeof(path)) < 0){
      fprintf(stderr, "E: unable to load libdbase path\n");
      return -1;
    }
  }
  
  char file[MAX_PATH_LEN];
  snprintf(file, sizeof(file), "%s/status", path);

  if(_DEBUG > 0)
    printf("Reading status from file: %s\n", file);

  FILE *fh = fopen(file, "rb");
  if(fh == NULL){
    fprintf(stderr,
	    "E: libdbase_load_status() Unable to open status file %s\n", 
	    file);
    return -1;
  }
  
  status_msg buf;
  err = fread((unsigned char*)&buf, 1, sizeof(status_msg), fh);
  fclose(fh);
  if(err != sizeof(status_msg)){
    fprintf(stderr,
	    "E: Unable to read status message from file %s, read %d bytes\n",
	    file, err);
    return -1;
  }

  det->status = buf;
  double fgn = (double)(det->status.FGN / GAIN_FACTOR);
  det->status.FGN = (uint32_t) GET_GAIN_VALUE(fgn);
  err = dbase_write_status(det->dev, &det->status);

  return err;
}*/

/*
  Load status from ASCII save file
  if dir is NULL, status is loaded from default path.
*/
int libdbase_load_status_text(detector *det, const char *dir){
  if( check_detector(det, "libdbase_load_status_text") < 0)
    return -1;

  int err;
  char path[MAX_PATH_LEN - 10];
  /* Load from custom path */
  if(dir != NULL)
    snprintf(path, sizeof(path), "%s/%d", dir, det->serial);
  else {
    /* Load from path set at compile time */
    if(libdbase_get_det_path(det, path, sizeof(path)) < 0){
      fprintf(stderr, "E: unable to load libdbase path\n");
      return -1;
    }
  }
  
  /* Status file path */
  char file[MAX_PATH_LEN];
  snprintf(file, sizeof(file), "%s/status.txt", path);

  if(_DEBUG > 0)
    printf("Reading status from file: %s\n", file);

  /* Open file for reading (text) */
  FILE *fh = fopen(file, "r");
  if(fh == NULL){
    fprintf(stderr, 
	    "E: libdbase_load_status_text() Unable to open status file %s\n", 
	    file);
    return -1;
  }
  
  /* Parse human-readable text file to det.status */
  err = parse_status_lines(&det->status, fh);

  /* Close file */
  fclose(fh);

  /* Control */
  if(_DEBUG)
    libdbase_print_status(det);

  /* Write to dbase */
  if( !err )
    return dbase_write_status(det->dev, &det->status);
  return err;
}

/*
  Check status sanity before saving it to file.

  - Returns a status_msg fit for saving through *mod
  - On return err(<0) stat is corrupt and should not be saved.
*/
int check_status_sanity(status_msg stat, status_msg *mod){
  int err = 1;
  
  /* Create a copy of stat */
  memcpy((unsigned char*) mod, (unsigned char*)&stat, sizeof(status_msg));

  /* 
     If any of these tests fail, 
     status is corrupt and err flag is set
  */
  err &= (stat.LEN == DBASE_LEN);
  err &= (stat.HVT > MIN_HV);
  err &= (stat.HVT < MAX_HV);

  if(_DEBUG > 0)
    printf("Status %s the sanity tests\nCTRL before mod:0x%02x\n",
	   err == 0 ? "failed" : "passed",
	   (unsigned char)mod->CTRL);
  /* 
     The CTRL byte should be set to:
     - HV off
     - PHA mode
     - stopped
  */ 
  /* Stopped */
  mod->CTRL &= (uint8_t) OFF_MASK;
  /* HV off  */
  mod->CTRL &= (uint8_t) HV_OFF_MASK;
  /* PHA mode */
  mod->CTRL |= (uint8_t) MODE_PHA_MASK;

  /*
    Clear counter fields
  */
  mod->LT = 0;
  mod->RT = 0;

  if(_DEBUG > 0)
    printf("CTRL after mod: 0x%02x\n", (unsigned char)mod->CTRL);

  return err == 0 ? -1 : err;
}

/*
  Set digiBASE in PHA (Pulse Height Analysis) Mode (default)
*/
int libdbase_set_pha_mode(detector *det){
  int err;
  /* Set mode bit */
  det->status.CTRL |= (uint8_t) MODE_PHA_MASK;
  err = dbase_write_status(det->dev, &det->status);
  if(err < 0){
    fprintf(stderr, "E: When setting the pha-mode bit\n");
    err_str("libdbase_set_pha_mode()", err);
  }
  /* And clear mcb */
  err = libdbase_clear_spectrum(det);
  if(err < 0){
    fprintf(stderr, "E: When clearing spectrum after setting pha mode\n");
    err_str("libdbase_set_pha_mode()", err);
  }
  return err;
}

/*
  Set digiBASE in List Mode
*/
int libdbase_set_list_mode(detector *det){
  int err;
  /* Clear on bit */
  det->status.CTRL &= (uint8_t) OFF_MASK;
  /* Clear mode bit */
  det->status.CTRL &= (uint8_t) MODE_LM_MASK;
  /* Set msb bit */
  det->status.CTRL |= (uint8_t) 0x80;
  err = dbase_write_status(det->dev, &det->status);
  if(err < 0){
    fprintf(stderr, "E: When setting the list-mode bit\n");
    err_str("libdbase_set_list_mode()", err);
  }

  /* Clear live/real counters */
  libdbase_clear_counters(det);

  /* Clear msb bit */
  det->status.CTRL &= (uint8_t) 0x7f;

  /* After this, the device should be in list-mode */
  err = dbase_write_status(det->dev, &det->status);
  if(err < 0){
    fprintf(stderr, "E: When setting msb bit in list-mode\n");
    err_str("libdbase_set_list_mode()", err);
  }
  return err;
}

/*
  Read packets in list mode
 
  - buf can be null, in which case only the number
    of events is returned.
  - len is max number of pulses in *buf
  - number of read events is returned through *read
  - *time should point to an int initialized to zero when first called

*/
int libdbase_read_lm_packets(detector *det, pulse *buf, 
			     int len, int *read, uint32_t *time){
  if( check_detector(det, "libdbase_read_lm_packets") < 0) return -1;
  if(len <= 0 || read == NULL || time == NULL){
      if(len <= 0)
      fprintf(stderr,"E: libdbase_parse_lm_packets(), length must be a positive number\n");
      else if(read == NULL)
      fprintf(stderr,"E: libdbase_parse_lm_packets(), read pointer can't be null\n");
      else
      fprintf(stderr,"E: libdbase_parse_lm_packets(), time pointer can't be null\n");
    return -1;
  }
  int err, io;
  
  /* Request packets from digibase */
  err = dbase_write_one(det->dev, SPECTRUM, &io);
  if(err < 0 || io != 1){
    fprintf(stderr, "E: libdbase_parse_lm_packets() when requesting spectrum (written=%d)\n", io);
    err_str("libdbase_parse_lm_packets()", err);
    return err;
  }
  
  /* Allocate temporary buffer */
  const int blen = len * sizeof(uint32_t);
  uint32_t *tmp = malloc( blen );
  if(tmp == NULL) {
    fprintf(stderr, "E: failed to allocate temporary buffer in libdbase_read_lm_packets()\n");
    return -ENOMEM;
  }
  
  /* 
     Read packets, 
     use libusb directly to avoid an additional buffer (in dbase_read()) 
  */
  /*err = libusb_bulk_transfer(det->dev, EP_IN, (unsigned char*) tmp, len, &io, S_TIMEOUT);*/
  /* 2012-02-25: modified len to correct nbr of bytes */
  err = libusb_bulk_transfer(det->dev, EP_IN, (unsigned char*) tmp, blen, &io, S_TIMEOUT);
  if(err < 0){
    fprintf(stderr, "E: when reading list mode data (read %d bytes)\n", io);
    err_str("libdbase_parse_lm_packets()", err);
    fprintf(stderr, "If you got an overflow error, try increasing the buffer size\n");
    /* Make a clean exit */
    free(tmp);
    return err;
  }
  
  /*
    Got io/4 int32's from digibase,
    but some int32's are timestamps.
    Parsing is done here.
   */
  int k, r=0;
  /* Swap bytes on BIG_E machines */
  if( IS_BIG_ENDIAN() ){
    for(k=0; k < io/4; k++)
      BYTESWAP(tmp[k]);
  }
    
  /* Only count the number of events with amplitude */
  if(buf == NULL)
  {
    for(k=0; k < io/4; k++)
      {
	/* End of data? */
	if(tmp[k] == 0)
	  break;
	if(tmp[k] <= TS_MASK)
	  r++;
      }
  }
  /* Parse additional information as well (amp, time) */
  else
  {
    for(k=0; k < io/4; k++)
    {
	/* End of data? */	
	if(tmp[k] == 0)
	  break;
 	/* Amplitude & time? */
	else if(tmp[k] <= TS_MASK){
	  buf[r].amp =  (uint32_t) ((tmp[k] & A_MASK) >> 21);
	  /* First time tick after (in worst case) MAX_T us */
	  if(time[0] > 0){
	    buf[r].time = (uint32_t) (tmp[k] & T_MASK);
	    /* Timer rollover? */
	    if(buf[r].time > MAX_T){
	      buf[r].time -= MAX_T;
	    }
	    buf[r].time += time[0];
	  }
	  r++;
	}
	/* Timestamp then */
	else
	  time[0] = (uint32_t) (tmp[k] & TS_MASK);
      }
  }
  
  /* Set number of read events */
  *read = r;
  /* Free internal buffer */
  free(tmp);
  /* Success */
  return 0;
}

/*
  Iterate through libusb devices and 
  extract serial numbers of all digibases found.
  
  - returns no more than len serial numbers in
    *serials (which must be allocated before call).
  - returns number of found serial numbers
 */
int libdbase_list_serials(int *serials, int len) {
  if(serials == NULL || len == 0){
    if(len != 0)
      fprintf(stderr,
	      "E: serials array must be initialized before calling libdbase_list_serials()\n");
    return -1;
  }
  /*
    This function might be called before before libdbase_init()
    Create context if it's uninitialized
  */
  if(cntx == NULL)
    libusb_init(&cntx);

  /* Extract serial numbers */
  libusb_device **list = NULL;
  libusb_device_handle *handle = NULL;
  ssize_t cnt = libusb_get_device_list(cntx, &list);
  int found = 0;
  if(cnt < 0)
    return -1;
  if(_DEBUG > 0)
    printf("Found %d usb devices\n", (int)cnt);
  ssize_t i = 0;
  int err = 0;
  struct libusb_device_descriptor desc;
  for(i=0; i < cnt; i++){
    libusb_device *device = list[i];
    err = libusb_get_device_descriptor(device, &desc);
    /* is the device a dbase ? */
    if(err == 0 && 
       desc.idVendor == VENDOR_ID && 
       desc.idProduct == PROD_ID){
      /* Open connection to device to aquire serial no */
      err = libusb_open(device, &handle);
      if(err < 0){
	/* Unable to open usb connection to device - bail */
	fprintf(stderr, "E: libdbase_list_serials() Error opening device\n");
	libusb_free_device_list(list, 1);
	return -1;
      }
      err = dbase_get_serial(device, handle, desc, &serials[found]);
      found++;
      /* Close device connection */
      libusb_close(handle);
      /* Find more? */
      if(found == len){
	if(i < cnt - 1)
	  fprintf(stderr, "W: libdbase_list_serials() not all devices checked\nconsider increasing the buffer size\n");
	break;
      }
    }
    else if(err < 0)
      err_str("libusb_get_device_descriptor()", err);
  }
  if(_DEBUG > 0)
    printf("libdbase_list_serials() Exiting loop, found %d dbase(s)\n", found);
  
  /* free libusb list */
  libusb_free_device_list(list, 1);

  /* exit libusb */
  libusb_exit(cntx);
  
  /* 
     destroy context so libusb_init() 
     is called the next time also
  */
  cntx = NULL;

  return found;
}

/* 
   Get libdbase's path for current detector,
   can be used by applications to save 
   detector-specific data.
*/
int libdbase_get_det_path(detector *det, char *path, ssize_t len){
  if( check_detector(det, "libdbase_get_det_path") < 0)
    return -1;

  /* Write path set at compile time */
  char dir[MAX_PATH_LEN];
  int read = libdbase_get_path(dir, sizeof(dir));
  /* Make sure there's room for serial # (5) and '/' (1) */
  if(read < 0 || (len < read + 6)){
    fprintf(stderr, "E: libdbase_get_path path buffer too small\n");
    return -1;
  }
  return snprintf(path, len, "%s/%d", dir, det->serial);
}

/*
  Get libdbase's root path set at compile time
*/
int libdbase_get_path(char *path, ssize_t len){
#ifdef PACK_PATH
  if(strlen(PACK_PATH) > len){
    fprintf(stderr, 
	    "E: path buffer cannot hold libdbase's PACK_PATH\n"
	    "\tYou need a buffer size of at least %d bytes\n",
	    (int)strlen(PACK_PATH));
    return -1;
  }
  return snprintf(path, len, "%s", PACK_PATH);
#else
  return snprintf(path, len, ".");
#endif
}
