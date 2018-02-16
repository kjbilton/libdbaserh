/* 
 * dbaserh.h: digibase C control program header file
 * 
 * adapted from dbase.h for the use with digiBaseRH
 * (2012-2014 Jan Kamenik)
 * ================================================= 
 * 
 * 
 * dbase.h: digibase C control program header file
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

#define PATH_MAX_LEN 256

/* 
   Struct holding various user settings
   -set 
   -clr 
   vars are stored in this struct
*/
typedef struct{
  int haveopts;  /* options set flag */
  int hvt;       /* HV target */
  double pw;     /* pulse width */
  double fgn;    /* fine gain */
  double ltp;    /* live time preset */
  double rtp;    /* real time preset */
  int gsch[2];   /* gain stab. chans. */
  int zsch[2];   /* zero stab. chans. */

  int clrall;    /* clear all */
  int clrspec;   /* clear spectrum */
  int clrpres;   /* clear presets */
  int clrcnt;    /* clear counters */

} settings;

/* Signal handler */
void handle_signal(int sig);

/* Measure in list mode */
void measure_list_mode(char lmc, 
		       char lma, 
		       char lmt, 
		       unsigned long long lm_time, 
		       unsigned long long sleept);

/* Parse time argument */
void parse_time(unsigned long long *duration, 
		const char *input);

/*
  Print short howto-guide
*/
void print_usage();

/*
  Parse settings struct
*/
int parse_settings(const char* set, 
		   char** val, 
		   settings* opts, 
		   int pos, 
		   int argc, 
		   int *k);

/*
  Set settings to default
*/
void init_settings(settings* opts);

/*
  Make a clean exit
*/
void dbase_exit(int ecode);
