/* 
 * dbaserh.c: digibase C control program
 *
 * adapted from dbase.c for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * dbase.c: digibase C control program
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

#define VERSION      "0.3" /*JK*/
#define PROGRAM_NAME "digibaseRH C-control program" /*JK*/

#include <errno.h>   /* error numbers */
#include <stdlib.h>  /* atoi(), atoi()...*/
#include <stdio.h>   /* printf(), ... */
#include <unistd.h>  /* usleep(), ... */
#include <string.h>  /* strcmp(), ... */
#include <signal.h>  /* signals */

#include <sys/stat.h> /* mkdir(), ...*/

/* libdbase public header */
#include "libdbaserh.h" /*JK*/
/* dbase header */
#include "dbaserh.h" /*JK*/

/*
  Print short howto-guide
*/
void print_usage() {
  printf("%s v. %s\n", PROGRAM_NAME, VERSION);
  printf("Usage: dbaserh \t[-d n][-t,i T] [-s n] [-l,d,q,h] [-o file]\n\t\t[-CTRL [on/off]]\n\t\t[-set hvt/fg/pw/ltp/rtp/zsch [value(s)]]\n\t\t[-clr [all/spec/pres/cnt]]\n");/*JK*/
/*  printf("Usage: dbaserh \t[-d n][-t,i T] [-s n] [-lm [c/a/t]] [-l,d,q,h] [-o file]\n\t\t[-CTRL [on/off]]\n\t\t[-set hvt/fg/pw/ltp/rtp/gsch/zsch [value(s)]]\n\t\t[-clr [all/spec/pres/cnt]]\n");*/

  /* Argument desc.: */
  printf("\t -d n\tOperate on device with device_no n (see -l)\n");
  printf("\t -t\tSample for time T, then exit (-1=infinite)\n");
  printf("\t -i\tSampling interval T\n\t\t(pha-mode: default 1s, list-mode: default 50ms)\n\t\tvalid time suffix's are one of: ms,s,m,h,d,y (omitted s is assumed)\n");
  printf("\t -s\tPrint status every n:th measurement (default off)\n");
/*JK, not tested  printf("\t -lm\tSet in list mode, print c=counts,a=amplitudes,t=times\n");*/
/*JK, not working  printf("\t -diff\tPrint differential spectra rather than cumulative\n");*/
  printf("\t -cps\tPrint cps instead of spectra\n");
  printf("\t -q\tQuiet\n");
  printf("\t -h\tPrints this message\n");
  printf("\t -l\tList connected digibase's device_no, serial_no and dev_names\n");
  printf("\t -b\tBinary output (default ASCII)\n");
  printf("\t -o\tOutput to file (default is stdout)\n");

  /* CTRL commands  */
  printf(" CTRL: \n");
  printf("\t -start\t\t Start Measurement\n");
  printf("\t -stop\t\t Stop Measurement\n");
  printf("\t -hv on/off\t High Voltage on/off\n");
/*JK, not working  printf("\t -gs on/off\t Gain stabilization on/off\n");*/

  /* Clear commands */
  printf(" Clear: \n");
  printf("\t -clr all/spec/pres/cnt (all three or one of: spectrum/presets/counters)\n");

  /* Settings commands */
  printf(" Settings:\n");
  printf("\t -set hvt val\t (High voltage taget, 50-1200 V)\n");
  printf("\t -set fgn val\t (Fine gain, 0.5-1.2)\n");
  printf("\t -set pw  val\t (Pulse width, 0.75-2.0 us)\n");
  printf("\t -set ltp val\t (Live time preset (s))\n");
  printf("\t -set rtp val\t (Real time preset (s))\n");
/*JK, not working  printf("\t -set gsch C W\t (Gain stab center ch. and width)\n");*/

  /* Print-once commands */
  printf(" Print once: \n");
  printf("\t -stat\t\t Print status\n");
  printf("\t -spec\t\t Print MCB spectrum\n");
  //printf("\t -specb\t\t Print binary MCB spectrum\n");
  printf("\t -roi low high\t Print Region of Interest [low:high channels]\n");
  printf("\t -serial\t Print digibase's serial number\n");
  printf("\t -name DEV_NAME\t Name digibase with device_no n (use with -d n)\n");

  /* Load/Save status functions */
  printf(" Status I/O:\n");
  printf("\t -save\t\t Save status to file\n");
  printf("\t -load\t\t Load status from file\n");

  /* NOT TESTED features */
  printf(" NOT TESTED:\n");
  printf("\t -zs on/off\t Zero stabilization on/off\n");
  printf("\t -set zsch C W\t (Zero stab center ch. and width)\n");
  
  /* Note*/
  printf("\n BUGS:\n");
  printf("\t Not possible to re-set Gain stabilization on/off and channels\n");
  printf("\t on initialized device. Needed if HVT is changed!\n");
  printf("\t Solution -- modification of init message in the source file \n");
  printf("\t (libdbaserhi.c) followed by new compilation.\n\n");

  printf("\nExample 1:\t dbase -q -hv on -start -t 50\nwill enable hv, start measurement, measure for 50 seconds and then exit\nwithout printing anything other than ascii spectra to stdout.\n");
//  printf("\nExample 2:\t dbase -q -hv on -lm c -t 1.0h -i 0.05 -o outp.txt\nwill enable hv, start measurement in list mode, measure for 1h and exit.\nThe counts recieved per 50ms by digibase will be printed to 'outp.txt'\n");
  printf("\nJan Kamenik, 2012-2014 (adapted from dbase, (C) Peder Kock, 2011,2012)).\n\n");
}

/*
  Global vars 
*/
/* detector handle */
detector *det = NULL;
/* output file handle */
FILE *fh = NULL;
/* verbose output */
int q = 0;

/* Handle CTRL+C */
void handle_signal(int sig){
  if(!q)
    printf("Interrupted - exiting\n");
  dbase_exit(1);
}

/* Main function */
int main(int argc, char **argv) {
  int k;
  int errc;

  if(argc < 2){
    print_usage();
    return EXIT_SUCCESS;
  }

  /* attach signal handler */
  (void)signal(SIGINT, handle_signal);

  /* Arguments from user */
  /* General */
  int on=0, stat=-1,d=0, b=0, ser=0, spec=0, cps=0, list=0, dev=0, name=0;
  /* ROI prints */
  int roi=0;
  uint roi_lc=0, roi_hc=0;
  /* Save/load */
  int save=0, load=0;
  /* Times */
  unsigned long long t=0ULL, s=0ULL, sleept=1000000ULL;
  /* List mode arguments */
  int lm=0;
  char lmc=0,lmt=0,lma=0;
  /* output file, hv settings etc. */
  char *ofile=NULL, *hv=NULL, *gs=NULL, *zs=NULL, *dev_name=NULL;
  /* Settings parameters */
  settings opts;
  init_settings(&opts);

  /* 
     Parse quiet/help argument first 
  */
  for(k=1; k < argc; k++){
    if(strcmp(argv[k],"-q") == 0)
      q = 1;
    /* Print help message */
    else if(strcmp(argv[k],"-h") == 0 || strcmp(argv[k],"--help") == 0){
      print_usage();
      return EXIT_SUCCESS;
    }
  }

  /* 
     Parse arguments 
  */
  for(k=1; k < argc; k++){
    /* Status interval */
    if(strcmp(argv[k],"-s") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -s\n");
	  return EXIT_FAILURE;
	}
	s = atol(argv[k+1]);
	k++;
      }
    /* HV on/off */
    else if(strcmp(argv[k],"-hv") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -hv\n");
	  return EXIT_FAILURE;
	}
	hv = argv[k+1];
	if(strcmp(hv,"on") != 0 && strcmp(hv,"off") != 0){
	  if(!q)
	    fprintf(stderr, "E: hv parameter must be 'on' or 'off'\n");
	  return EXIT_FAILURE;
	}
	k++;
      }
    /* Zero stab. on/off */
    else if(strcmp(argv[k],"-zs") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -zs\n");
	  return EXIT_FAILURE;
	}
	zs = argv[k+1];
	if(strcmp(zs,"on") != 0 && strcmp(zs,"off") != 0){
	  if(!q)
	    fprintf(stderr, "E: zs parameter must be 'on' or 'off'\n");
	  return EXIT_FAILURE;
	}
	k++;
      }
    /* Gain stab. on/off */
    else if(strcmp(argv[k],"-gs") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -gs\n");
	  return EXIT_FAILURE;
	}
	gs = argv[k+1];
	if(strcmp(hv,"on") != 0 && strcmp(gs,"off") != 0){
	  if(!q)
	    fprintf(stderr, "E: gs parameter must be 'on' or 'off'\n");
	  return EXIT_FAILURE;
	}
	k++;
      }
    /* Sampling interval */
    else if(strcmp(argv[k],"-i") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -i\n");
	  return EXIT_FAILURE;
	}
	parse_time(&sleept, argv[k+1]);
	k++;
      }
    /* List mode , with (at least one) parameter(s) [c/a/t] */
    else if(strcmp(argv[k],"-lm") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -lm\n");
	  return EXIT_FAILURE;
	}
	lm = 1;
	/* Can be one or several of: c,a,t */
	lmc = (strstr(argv[k+1], "c") == NULL) ? 0 : 1;
	lma = (strstr(argv[k+1], "a") == NULL) ? 0 : 1;
	lmt = (strstr(argv[k+1], "t") == NULL) ? 0 : 1;
	if(lmc == 0 && lmt == 0 && lma == 0){
	  if(!q)
	    fprintf(stderr, "E: list mode arguments must be one or more of 'c,a,t'\n");
	  return EXIT_FAILURE;
	}
	k++;
      }
    /* Measurement Time */
    else if(strcmp(argv[k],"-t") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -t\n");
	  return EXIT_FAILURE;
	}
	parse_time(&t, argv[k+1]);
	k++;
      }
    /* Name device */
    else if(strcmp(argv[k],"-name") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -name\n");
	  return EXIT_FAILURE;
	}
	name = 1;
	dev_name = argv[k+1];
	k++;
      }
    /* Print status */
    else if(strcmp(argv[k],"-stat") == 0)
      {
	stat = 1;
      }
    /* Print spectrum */
    else if(strcmp(argv[k],"-spec") == 0)
      {
	spec = 1;
      }
    /* Print binary (spectrum/status) */
    else if(strcmp(argv[k],"-b") == 0)
      {
	b = 1;
	/* since it's binary output assume quiet */
	q = 1;
      }
    /* Start measurement */
    else if(strcmp(argv[k],"-start") == 0)
      {
	on = 1;
      }
    /* Stop measurement */
    else if(strcmp(argv[k],"-stop") == 0)
      {
	on = -1;
      }
    /* Print differential spectra */
    else if(strcmp(argv[k],"-diff") == 0)
      {
	d = 1;
      }
    /* Operate on device with specified device_no */ /*JK*/
    else if(strcmp(argv[k],"-d") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -d\n");
	  return EXIT_FAILURE;
	}
	dev = atoi(argv[k+1]);
	k++;
      }
    /* Print serial number */
    else if(strcmp(argv[k],"-serial") == 0)
      {
	ser = 1;
      }
    else if(strcmp(argv[k],"-l") == 0)
      {
	list = 1;
      }
    /* Print ROI channels */
    else if(strcmp(argv[k],"-roi") == 0)
      {
	roi = 1;
	/* Parse channels */
	if(argc < k+3){
	  fprintf(
		  stderr, 
		  "E: lacking arguments for -roi, must give low and high channels\n"
		  );
	  return EXIT_FAILURE;
	}
	roi_lc = atoi(argv[k+1]);
	roi_hc = atoi(argv[k+2]);
	k+=2;
      }
    /* Print cps */
    else if(strcmp(argv[k],"-cps") == 0)
      {
	cps = 1;
      }
    /* Write output to FILE */
    else if(strcmp(argv[k],"-o") == 0){
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -o\n");
	  return EXIT_FAILURE;
	}
      ofile = argv[k+1];
      k++;
    }
    /* Clear commands */
    else if(strcmp(argv[k],"-clr") == 0)
      {
	if(argc < k+2){
	  fprintf(stderr, "E: lacking argument to -clr\n MUST be one of all/spec/pres/cnt\n");
	  return EXIT_FAILURE;
	}
	if(parse_settings(argv[k+1], argv, &opts, k+2, argc, &k) < 0){
	  fprintf(stderr, "E: -clr arguments given but no value\nMust give one of all/spec/pres/cnt\n");
	return EXIT_FAILURE;
	}
	/* flag that options have changed */
	opts.haveopts = 1;
      }
    /* Change Settings */
    else if(strcmp(argv[k],"-set") == 0)
      {
	if(argc < k+3){
	  fprintf(stderr, "E: lacking arguement(s) to -set \n");
	  return EXIT_FAILURE;
	}
	if( parse_settings(argv[k+1], argv, &opts, k+2, argc, &k) < 0 ){
	  fprintf(stderr, "E: -set argument given but no value\n");
	  return EXIT_FAILURE;
}
	/* flag that options have changed */
	opts.haveopts = 1;
      }
    /* Save status */
    else if(strcmp(argv[k],"-save") == 0)
      {
	save = 1;
      }
    /* Load status */
    else if(strcmp(argv[k],"-load") == 0)
      {
	load = 1;
      }
    /* Unknown argument */
    else if(!q)
      {
	printf("Error: unknown command: %s\n", argv[k]);
	return EXIT_FAILURE;
      }
  }

  /* One of these must be set or we won't do anything... */
  if( opts.haveopts == 0 && t==0UL && on==0 && stat==-1 && 
      s == 0 && hv == NULL && lm == 0 && ser == 0 &&
      gs == NULL && zs == NULL && spec == 0 && save == 0 && 
      load == 0 && roi == 0 && list == 0 && name == 0)
    {
      if(!q)
	printf("No valid commands given - aborting\n");
      return EXIT_FAILURE;
    }

  /* Allow up to 32 dbases */
  int LEN = 32, serials[LEN], found = -1;
  char dir[PATH_MAX_LEN];
    
  /* get dbase(s) serial numbers */
  if(dev > 0 || list){
    found = libdbase_list_serials(serials, LEN);
    if(!q)
      printf("Found %d digibase(s)\n", found);
    if(found < 0){
      if(!q)
	fprintf(stderr, "E: couldn't find serial numbers\n");
      return EXIT_FAILURE;
    }
  }

  /* Print device list and exit */
  if(list){
    if(found < 1){
      printf("Did not find any digibase on usb bus\n");
      return EXIT_FAILURE;
    }
    char path[PATH_MAX_LEN], dname[128];
    libdbase_get_path(dir, sizeof(dir));
    for(k = 0; k < found; k++) {
      snprintf(path, sizeof(path), "%s/%d/name", dir, serials[k]);
      FILE *f = NULL;
      f = fopen(path, "r");
      int hasname = 0, name_len;
      if(f != NULL){
	if((name_len = fread(dname, 1, sizeof(dname)-1, f)) > 0){
	  hasname = 1;
	  /* null terminate it */
	  dname[name_len] = '\0';
	}
	fclose(f);
      }
      printf("[%d]:\t%d\t%s\n", k+1, serials[k], hasname ? dname : "");
    }
    dbase_exit(0);
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

  /* Find and open connection to detector */
  if(dev > 0){
    if(!q)
      printf("Opening device no %d (serial %d)\n", dev, serials[dev]);
    det = libdbase_init(serials[dev], str);
    if(det == NULL){
      if(!q)
	fprintf(stderr, "Error: Couldn't open device no %d\n", dev);
      return EXIT_FAILURE;
    }
  }
  else{
    if(!q)
      printf("Opening first device\n");
    if((det = libdbase_init(-1, str)) == NULL){
      if(!q)
	fprintf(stderr, "Error: Couldn't open digibase\n");
      return EXIT_FAILURE;
    }
  }

  /* Name device */
  if(name){
    if(dev_name == NULL || strcmp(dev_name, "") == 0 ||
       strlen(dev_name) > PATH_MAX_LEN){
      if(!q)
	fprintf(stderr, "Error: invalid device name\n");
      dbase_exit(-1);
    }
    libdbase_get_det_path(det, dir, sizeof(dir));
    /* Create dir if it doesn't exist */
    errc = mkdir(dir, 
		S_IRUSR | S_IWUSR | S_IXUSR |  /* User perm,   7 */
		S_IRGRP | S_IWGRP | S_IXGRP |  /* Group perm,  7 */
		S_IROTH | S_IXOTH              /* Others perm, 5 */
		);
    if(errc != 0 && errno != EEXIST){
      fprintf(stderr, "Error creating directory for detector at: %s\n", dir);
      dbase_exit(-1);
    }
    char path[202];
    snprintf(path, sizeof(path), "%s/name", dir);
    /* Write user-given name to device 'dev's' directory */
    FILE *f = fopen(path, "w");
    if(f == NULL){
      fprintf(stderr, "Error when opening name file for writing:\n%s\n", path);
      dbase_exit(-1);
    }
    fprintf(f, "%s", dev_name);
    fclose(f);
    if(!q)
      printf("Named detector %d (serial %d) to %s\n", dev, det->serial, dev_name);
    dbase_exit(0);
  }

  /* Open output file (-o FILE) */
  if(ofile != NULL){
    fh = fopen(ofile, "w");
    if(fh == NULL){
      if(!q)
	fprintf(stderr, "Error: unable to open output file %s\n", ofile);
      dbase_exit(-1);
    }
  }

  /* Load status from file */
  if(load == 1){
    //if(libdbase_load_status(det, NULL) < 0)
    if(libdbase_load_status_text(det, NULL) < 0)
      fprintf(stderr, "E: when loading status\n");
    else if(!q)
	printf("Status loaded\n");
  }

  /* Settings first */
  if(1 == opts.haveopts){
    /* hv target set ?*/
    if(opts.hvt > 0){
      if(libdbase_set_hv(det, opts.hvt) < 0 && !q)
	fprintf(stderr, "E: setting HVT\n");
      else if(!q)
	printf("Setting HV Target to %d\n", opts.hvt);
    }
    /* pulse width set? */
    if(opts.pw > 0.0){
      if(libdbase_set_pw(det, opts.pw) < 0 && !q)
	fprintf(stderr, "E: setting pulse width\n");
      else if(!q)
	printf("Setting PW to %0.2f\n", opts.pw);
    }
    /* live time preset set? */
    if(opts.ltp > -1.0){
      if(libdbase_set_ltp(det, opts.ltp) < 0 && !q)
	fprintf(stderr, "E: setting live time preset\n");
      else if(!q)
	printf("Setting LTP to %0.2f\n", opts.ltp);
    }
    /* real time preset set? */
    if(opts.rtp > -1.0){
      if(libdbase_set_rtp(det, opts.rtp) < 0 && !q)
	fprintf(stderr, "E: setting real time preset\n");
      else if(!q)
	printf("Setting RTP to %0.2f\n", opts.rtp);
    }
    /* fine gain set? */
    if(opts.fgn > 0.0){
      if(libdbase_set_fgn(det, opts.fgn) < 0 && !q)
	fprintf(stderr, "E: setting fine gain\n");
      else if(!q)
	printf("Setting FG to %0.5f\n", opts.fgn);
    }
    /* Clear commands [spec/all/pres/cnt] */
    if(opts.clrspec > 0){
      if(!q) 
	printf("Clearing spectrum...\n");
      if( (errc = libdbase_clear_spectrum(det)) < 0)
	fprintf(stderr, "E: clearing spectrum (err=%d)\n", errc);
    }
    if(opts.clrall > 0){
      if(!q) 
	printf("Clearing spectrum, counters and presets...\n");
      if( (errc = libdbase_clear_all(det)) < 0)
	fprintf(stderr, "E: clearing all (err=%d)\n", errc);
    }
    if(opts.clrpres > 0){
      if(!q) 
	printf("Clearing presets...\n");
      if( (errc = libdbase_clear_presets(det)) < 0)
	fprintf(stderr, "E: clearing presets (err=%d)\n", errc);
    }
    if(opts.clrcnt > 0){
      if(!q) 
	printf("Clearing counters...\n");
      if( (errc = libdbase_clear_counters(det)) < 0)
	fprintf(stderr, "E: clearing counters (err=%d)\n", errc);
    }
  }

  /* Toggle HV (-hv on/off) */
  if(hv != NULL){
    if(strcmp(hv, "on") == 0){
      libdbase_hv_on(det);
      /* Sleep 3s to stabilize high voltage 
	 as this command might be used in conjunction with
	 for example -on. */
      /*if(!q) // Some problems with darwin+sleep
	printf("Sleeping 3s to stabilize HV\n");
	  sleep(3);*/
    }
    else if(strcmp(hv, "off") == 0)
      libdbase_hv_off(det);
    if(!q)
      printf("Setting HV to: %s\n", hv);
  }

  /* Toggle gain stabilization (-gs on/off) */
  if(gs != NULL){
    if(strcmp(gs, "on") == 0)
      libdbase_gs_on(det);
    else if(strcmp(gs, "off") == 0)
      libdbase_gs_off(det);
    if(!q)
      printf("Setting Gain Stabilization to: %s\n", gs);
  }
  /* Toggle zero stabilization (-zs on/off) */
  if(zs != NULL){
    if(strcmp(zs, "on") == 0)
      libdbase_zs_on(det);
    else if(strcmp(zs, "off") == 0)
      libdbase_zs_off(det);
    if(!q)
      printf("Setting Zero Stabilization to: %s\n", zs);
  }
  /* Print serial no (-serial) */
  if(ser == 1){
    fprintf(fh == NULL ? stdout : fh, "Serial# : %d\n", det->serial);
  }
  /* Start detector (-on) */
  if(on == 1){
    if(!q)
      printf("Starting detector...\n");
    libdbase_start(det);
  }
  /* Stop detector (-off) */
  else if(on == -1){
    if(!q)
      printf("Stopping detector...\n");
    libdbase_stop(det);
  }
  /* Print spectrum (-spec) */
  if(spec == 1){
    if(libdbase_get_spectrum(det) < 0){
      fprintf(stderr, "Error when aquiring spectrum from device\n");
      dbase_exit(-1);
    }
    else{
      if(!q)
	printf("%sSpectrum:\n", d == 1 ? "Differential " : "");
      /* normal, differential and/or binary */
      if(d == 0){
	if(b == 0)
	  libdbase_print_file_spectrum(det, fh == NULL ? stdout : fh);
	else
	  libdbase_print_file_spectrum_binary(det, fh == NULL ? stdout : fh);
	}
      else {
	if(b == 0)
	  libdbase_print_diff_file_spectrum(det, fh == NULL ? stdout : fh);
	else
	  libdbase_print_diff_file_spectrum_binary(det, fh == NULL ? stdout : fh);
      }
    }
  }
  /* Print ROI Channels (-roi L H) */
  if(roi == 1){
    if(libdbase_get_spectrum(det) < 0){
      fprintf(stderr, "Error when aquiring spectrum from device\n");
      dbase_exit(-1);
    }
    else {
      uint roi_sum;
      libdbase_print_roi(det, roi_lc, roi_hc, &roi_sum, fh == NULL ? stdout : fh);
      fprintf(fh == NULL ? stdout : fh, 
	      "\nROI[%u:%u]: %u\n", roi_lc, roi_hc, roi_sum);
    }
  }

  /* Collect data and print spectrum */
  int err = 1;
  /* List Mode measurement */
  if(lm == 1 && sleept <= 1000000UL && sleept > 0UL && t > 0UL){
    /* Change default 1s to 50ms */
    if(sleept == 1000000UL)
      sleept = 50000UL;
    if(!q)
      printf("Starting List Mode measurement freq: %llu Hz", 1000000ULL/sleept);
    /* Start list mode measurement */
    measure_list_mode(lmc, lma, lmt, t, sleept);
  }

  /*JK, time stamp*/
  time_t timestamp = time(0);

  unsigned long long i;
  /* PHA Mode measurement, max freq 20Hz */
  if(lm == 0 && t > 0UL && sleept > 50000UL){
    /* Clear counters and spectrum */
    libdbase_clear_all(det);
    /* Number of measurement cycles */
    unsigned long long cycles = (unsigned long long) (t / sleept);
    uint last_sum = 0, sum;
    for(i=0; i < cycles; i++){
      if(!q)
	printf("Msmt (%llu/%llu): Sleeping %llu ms\n", i+1, cycles, (sleept/1000UL));

      /*JK print start time (on the screen)*/
      timestamp = time(0); /*JK*/ 
      fprintf(fh == NULL ? stdout : fh, /*JK*/ 
              "\nStart (dbaserh): %s", ctime(&timestamp)); /*JK*/ 
      fprintf(fh == NULL ? stdout : fh, /*JK*/ 
              "Counting Time (dbaserh):  %llu.%02llu s\n",
              (sleept/1000000UL), (sleept%1000000UL)); /*JK*/ 

      usleep(sleept);

      if( (err = libdbase_get_spectrum(det)) >= 0)
	{
         timestamp = time(0); /*JK*/ 
         fprintf(fh == NULL ? stdout : fh, /*JK*/ 
                "End (dbaserh):   %s\n", ctime(&timestamp)); /*JK*/
 
	  if(cps == 1){
	    /* Just print counts per second */
	    libdbase_print_roi(det, 0, DBASE_LEN, &sum, NULL);
	    double cps = (sum - last_sum) / (double)(sleept / 1000000ULL);
	    fprintf(fh == NULL ? stdout : fh, 
		    "%s%0.00f\n", q ? "" : "CPS: ", cps);
	    last_sum = sum;
	  }
	  else if(d == 0){
	    /* print cumulative spectrum */
	    if(b == 0)
	      libdbase_print_file_spectrum(det, fh == NULL ? stdout : fh);
	    else
	      libdbase_print_file_spectrum_binary(det, fh == NULL ? stdout : fh);
	  }
	  else{
	    /* print differential spectrum */
	    if(b == 0)
	      libdbase_print_diff_file_spectrum(det, fh == NULL ? stdout : fh);
	    else
	      libdbase_print_diff_file_spectrum_binary(det, fh == NULL ? stdout : fh);
	  }
	  /* Status interval - print status */
	  if(s > 0UL && (i+1) % s == 0){
	    err = libdbase_get_status(det);
	    if(err >= 0){
	      if(b == 0)
		libdbase_print_file_status(&det->status, det->serial, fh == NULL ? stdout : fh);
	      else {
		fwrite(&det->status, sizeof(status_msg), 1, fh == NULL ? stdout : fh);
		fflush(fh == NULL ? stdout : fh);
	      }
	    }
	    else if(!q){
	      fprintf(stderr, 
		      "E: couldn't read status (err=%d) - exiting\n", err);
	      break;
	    }
	  }
	}
      else if(!q){
	fprintf(stderr, "E: couldn't read spectrum (err=%d) - exiting\n", err);
	break;
      }
    }
  }

  /* Stat command given */
  if(err >=0 && stat == 1){
    err = libdbase_get_status(det);
    if(err >= 0)
      libdbase_print_file_status(&det->status, det->serial, fh == NULL ? stdout : fh);
    else if(!q)
      fprintf(stderr, "E: couldn't read status (err=%d)\n", err);
  }

  /* Save status to file */
  if(save == 1){
    //if(libdbase_save_status(det, NULL) < 0)
    if(libdbase_save_status_text(det, NULL) < 0)
      fprintf(stderr, "E: when saving status\n");
    else if(!q)
      printf("Status saved to file\n");
  }

  /* Close output file */
  if(fh != NULL)
    fclose(fh);
  /* Close and free memory */
  libdbase_close(det);
  return EXIT_SUCCESS;
}

/*
  This function handles measurements in list-mode
*/
void measure_list_mode(char lmc, char lma, char lmt, 
		       unsigned long long lm_time, 
		       unsigned long long sleept){
  if(det == NULL) 
    return;

  /* Calc time per session (read/sleep cycle) */
  unsigned long long k, e = (lm_time / sleept);

  /* buffer size (max nbr of event per readout) */
  int len = 2048;
  int read, n;
  uint32_t time=0;

  /* Set in list mode */
  if(libdbase_set_list_mode(det) < 0)
    return;

  /* Start */
  libdbase_start(det);

  /* 
     Grab a chunk of memory (max nbr of events is len) 
     - *data can be null, in which case only the number
       of events are returned
  */
  pulse *data = NULL;
  if(lma == 1 || lmt == 1){
    data = calloc(len, sizeof(pulse));
    if(data == NULL){
      fprintf(stderr, 
	      "dbase E: couln't allocate memory for listmode pulses\n");
      return;
    }
  }

  if(!q){
    fprintf(fh == NULL ? stdout : fh, 
	    "Reading %llu events in list mode, sleeptime is %llu ms\n", 
	    e, sleept/1000ULL);
  }
  /* Read e sessions, sleep between readouts */
  for(k=0; k < e; k++)
    {
      /* sleep (collect pulses) */
      usleep(sleept);
    
      /* Read pulses */
      libdbase_read_lm_packets(det, data, len, &read, &time);
      
      /* output result */
      if(lmc == 1)
	/* counts */
	fprintf(fh == NULL ? stdout : fh, "%d\n", read);
      if(data != NULL)
	{
	  if(lma == 1 && lmt == 1)
	    for(n=0; n < read; n++)
	      /* amp and time */
	      fprintf(fh == NULL ? stdout : fh, 
		      "%u\t%u\n", data[n].amp, data[n].time);
	  else if(lma == 1)
	    for(n=0; n < read; n++)
	      /* amp */
	      fprintf(fh == NULL ? stdout : fh, 
		      "%u\n", data[n].amp);
	  else
	    for(n=0; n < read; n++)
	      /* time */
	      fprintf(fh == NULL ? stdout : fh, 
		      "%u\n", data[n].time);
	}
      
      /* Clear data before next readout */
      if(data != NULL)
	memset((char*)data, 0, read * sizeof(pulse));
    }
  /* Free memory */
  if(data != NULL)
    free(data);

  /* Stop */
  libdbase_stop(det);

  /* Set in pha mode again before returning */
  libdbase_set_pha_mode(det);
}

/* Parse a simple time string */
void parse_time(unsigned long long *duration, const char *input){
  unsigned long long t = 0LL;
  /*  
      valid time suffix's are ONE of: s,m,h,d,y (omitted s is assumed)\n");
  */
  if(strstr(input, "y") != NULL)
    t = 1000000ULL*3600ULL*24ULL*365ULL;  /* assume no leap year.. =/ */
  else if(strstr(input, "d") != NULL)
    t = 1000000ULL*3600ULL*24ULL;
  else if(strstr(input, "h") != NULL)
    t = 1000000ULL*3600ULL;
  else if(strstr(input, "ms") != NULL)
    t = 1000ULL;
  else if(strstr(input, "m") != NULL)
    t = 1000000ULL*60ULL;
  else
    t = 1000000UL;
  t = (unsigned long long) (t * atof(input));
  duration[0] = t;
}

/* Initialize settings struct */
void init_settings(settings* opts){
  if(opts == NULL)
    return;

  /* Initialize settings */
  opts->haveopts = 0;
  opts->hvt = 0;
  opts->pw = 0.0;
  opts->ltp = -1.0;
  opts->rtp = -1.0;
  opts->fgn = 0.0;

  /* initialize clear options */
  opts->clrall = 0;
  opts->clrspec = 0;
  opts->clrpres = 0;
  opts->clrcnt = 0;

  int k = 0;
  for(k = 0;k < 2; k++){
    opts->gsch[k] = 0;
    opts->zsch[k] = 0;
  }
}

/* 
   Parse user commands that begin with -set or -clr
*/
int parse_settings(const char* set, char** val, 
		   settings* opts, int pos, int argc, int *k){
  int err = -1;

  /* Check against all settings in struct */
  /* 'Settings' first */
  if(0 == strcmp(set, "hvt")){
    opts->hvt = atoi(val[pos]);
    err = 0;
  }
  else if(0 == strcmp(set, "pw")){
    opts->pw = atof(val[pos]);
    err = 0;
  }
  else if(0 == strcmp(set, "ltp")){
    opts->ltp = atof(val[pos]);
    err = 0;
  }
  else if(0 == strcmp(set, "rtp")){
    opts->rtp = atof(val[pos]);
    err = 0;
  }
  else if(0 == strcmp(set, "fgn")){
    opts->fgn = atof(val[pos]);
    err = 0;
  }
  else if(0 == strcmp(set, "gsch")){
    if(argc < pos + 2){
      fprintf(stderr, "E: not enough arguments for -gsch\n");
      return -1;
    }
    opts->gsch[0] = atoi(val[pos]);
    opts->gsch[1] = atoi(val[pos+1]);
    err = 0;
    k[0]++;
  }
  else if(0 == strcmp(set, "zsch")){
    if(argc < pos + 2){
      fprintf(stderr, "E: not enough arguments for -zsch\n");
      return -1;
    }
    opts->zsch[0] = atoi(val[pos]);
    opts->zsch[1] = atoi(val[pos+1]);
    err = 0;
    k[0]++;
  }

  /* then 'clear' commands */
  else if(0 == strcmp(set, "all")){
    opts->clrall = 1;
    err = 0;
  }
  else if(0 == strcmp(set, "spec")){
    opts->clrspec = 1;
    err = 0;
  }
  else if(0 == strcmp(set, "pres")){
    opts->clrpres = 1;
    err = 0;
  }
  else if(0 == strcmp(set, "cnt")){
    opts->clrcnt = 1;
    err = 0;
  }
  k[0] += 2;
  return err;
}

/* Make a clean exit, 
   closing all file handles and the detector */
void dbase_exit(int ecode){
  if(det != NULL){
    if(!q)
      printf("Closing detector...\n");
    libdbase_close(det);
  }
  if(fh != NULL){
    if(!q)
      printf("Closing output file...\n");
    fclose(fh);
  }
  if(!q)
    printf("done.\n");
  if(ecode < 0)
    exit(1);
  exit(0);
}
