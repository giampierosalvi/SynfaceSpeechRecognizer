/***************************************************************************
                         SoundProc.c  -  description
                             -------------------
    begin                : Tue May 13 2003
    copyright            : (C) 2003-2017 by Giampiero Salvi
    email                : giampi@kth.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ***************************************************************************/

#include <string.h> 
#include "Common.h"
#include "SoundProc.h"

Decimator *Decimator_Create() {
  Decimator *d;

  d = (Decimator *) malloc(sizeof(Decimator)); 
  /* default values to reduce risk for crash */
  d->factor = 6;
  d->ntaps = 20;
  d->state = 0;
  d->h = (double *) malloc(2*d->ntaps*sizeof(double));
  d->z = (double *) malloc(d->ntaps*sizeof(double));
  memset(d->h, 0, 2*d->ntaps*sizeof(double));
  memset(d->z, 0, d->ntaps*sizeof(double));

  return(d);
}

int Decimator_Free(Decimator **dptr) {
  Decimator *d = *dptr;

  if(d == NULL) return 0;

  if(d->z != NULL) free(d->z);
  if(d->h != NULL) free(d->h);
  free(d);
  *dptr = NULL;

  return 0;
}

int Decimator_ConfigureFilter(Decimator *d, double *h, int ntaps) {
  int i;

  if(d->z != NULL) free(d->z);
  if(d->h != NULL) free(d->h);
  d->ntaps = ntaps;
  d->h = (double *) malloc(2*ntaps*sizeof(double));
  d->z = (double *) malloc(ntaps*sizeof(double));
  for(i=0; i<ntaps; i++) {
    /* invert the coefficients (from matlab order) */
    d->h[i] = d->h[i+ntaps] = h[ntaps-i-1];
  }
  memset(d->z, 0, ntaps*sizeof(double));

  return(0);
}

int Decimator_ConfigureFactor(Decimator *d, int factor) {
  d->factor = factor;
  return 0;
}

/* if *out == *in it safely overwrites */
int Decimator_ProcessBuffer(Decimator *d, short *out, const short *in, int in_len) {
  int i, j;
  int state = d->state;
  int ntaps = d->ntaps;
  int factor = d->factor;
  double *z = d->z;
  //double *h = d->h;
  double *p_h, *p_z;
  double accum;

  for(i = 0; i < in_len; i++) {
    /* store input at the beginning of the delay line */
    d->z[state] = (double) in[i];

    /* compute output only if needed */
    if(i % factor == 0) {
      p_h = d->h + ntaps - state;
      p_z = z;
      accum = 0;
      for(j = 0; j < ntaps; j++) {
	accum += *p_h++ * *p_z++;
      }
      /* here chech if we need to clip */
      out[i/factor] = (short) accum;
    }
    /* decrement state, wrapping if below zero */
    if(--state < 0) {
      state += ntaps;
    }
  }
  d->state = state;

  return(i/factor);
}


// To be implemented: calibration tone detection

/* matlab code to generate the calibration signal:
   fs = 8000;    % sampling freq.
   fws = 0.025;  % frame len. in seconds
   flen= fs*fws; % frame len. in samples
   t = 0:1/fs:5; % time axis
   % three sinusoids
   a1 = 0.33; f1 = 1500; fi1 = 0.1;
   a2 = 0.33; f2 = 2050; fi2 = 0.3;
   a3 = 0.33; f3 = 2850; fi3 = 0.2;
   x = a1*sin(2*pi*f1*t+fi1)+a2*sin(2*pi*f2*t+fi2)+a3*sin(2*pi*f3*t+fi3);
   writewav(x,fs,'synface-calib.wav','u')
xalaw = lin2pcma(x);
c = fwrite(fid,xalaw,'int8')
fclose(fid)
sox synface-calib.wav -r 8000 -A -v 0.2 -b synface-calib.raw
*/

// from RTSpeetures
/*   /\* check for calibration signal Giampi *\/ */
/*   if(S->calib_check_flag) { */
/*     sum1 = 0.0; sum2 = 0.0; */
/*     for (fft_point=0; fft_point<info->fft_init->size; fft_point++) { */
/* #ifdef DUMPFFT */
/*       fprintf(fh,"%f ",data[fft_point]); */
/* #endif */
/*       sum1 += data[fft_point]*S->calib_check_template[fft_point]; */
/*       sum2 += data[fft_point]; */
/*     } */
/*     S->calib_check_prob = sum1/sum2; */
/*     /\*fprintf(stderr,"sum1=%.1f sum2=%.1f prob=%.3f\n", */
/*       sum1,sum2,S->calib_check_prob);*\/ */
/* #ifdef DUMPFFT */
/*     fprintf(fh,"\n"); */
/* #endif */
/*   } */

/*   /\* defaults for the calibration signal estimator, Giampi*\/ */
/*   S->calib_check_flag = 0; */
/*   S->calib_check_prob = 0.0; */
/*   S->calib_check_template = NULL; */

/* #ifdef DUMPFFT */
/*   /\* temporary here to dump data *\/ */
/*   fh = fopen("dumpfft.txt","w"); */
/* #endif */
      
/* /\* configures the information needed for the calibration signal prob. */
/*    estimation *\/ */
/* int Speetures_CalibCheckConf(Speetures *S, int calib_check_len, */
/* 			     float *calib_check_template) { */

/*   if(calib_check_len != 0) { */
/*     int expectedLen = S->allocated_window_size/2; */
/*     float sum=0.0; */
/*     float sumsq = 0.0; */
/*     int i; */

/*     Speetures_FreeCalibCheck(S); */
/*     S->calib_check_template = (float *) malloc(expectedLen * sizeof(float)); */
/*     if(calib_check_len > expectedLen) */
/*       DBGPRINTF("Speetures_CalibCheckConf: freq templete longer " */
/* 	      "than fft [%d,%d], ignoring extra samples\n", */
/* 	      calib_check_len, expectedLen); */
/*     if(calib_check_len < expectedLen) */
/*       DBGPRINTF("Speetures_CalibCheckConf: freq templete shorter " */
/* 	      "than fft [%d,%d], zero padding\n", */
/* 	      calib_check_len, expectedLen); */
    
/*     for(i=0;i<expectedLen;i++) { */
/*       if(i<calib_check_len) { */
/* 	sum+=calib_check_template[i]; */
/* 	sumsq+=pow(calib_check_template[i],2); */
/* 	S->calib_check_template[i] = calib_check_template[i]; */
/*       } else S->calib_check_template[i] = 0; */
/*     } */
/*     for(i=0;i<expectedLen;i++) S->calib_check_template[i]*=sum/sumsq; */
/*     /\* check ifwe should free this (it's allocated with ckalloc in */
/*        createFloatArrayFromTclList in rec.c)*\/ */
/*     //free(calib_check_template); */
/*     S->calib_check_flag = 1; */
/*   } */

/*   return 0; */
/* } */

/* int Speetures_FreeCalibCheck(Speetures *S) { */
/*   S->calib_check_flag = 0; */
/*   S->calib_check_prob = 0.0; */
/*   if(S->calib_check_template != NULL) */
/*     free(S->calib_check_template); */

/*   return 0; */
/* } */

/* float Speetures_GetCalibProb(Speetures *S) { */
/*   return S->calib_check_prob; */
/* } */

/* int Speetures_IsCalib(Speetures *S) { */
/*   return(S->calib_check_flag && S->calib_check_prob > S->calib_check_thresh); */
/* } */

