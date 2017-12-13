/***************************************************************************
                         SoundProc.h  -  description
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

#ifndef SOUNDPROC_H
#define SOUNDPROC_H

#include <stdlib.h>
#include <string.h>

typedef struct Decimator {
  int ntaps;
  double *h;    /* [2*ntaps] (double vector of coefficients)*/
  double *z;    /* [ntaps] */
  int state;
  int factor;   /* decimation factor */
} Decimator;

/* decimation factor, number of low-pass filter coefficients */
Decimator *Decimator_Create(void);
int Decimator_Free(Decimator **dptr);
int Decimator_ConfigureFilter(Decimator *d, double *h, int ntaps);
int Decimator_ConfigureFactor(Decimator *d, int factor);
int Decimator_ProcessBuffer(Decimator *d, short *out, const short *in, int in_len);

/* typedef struc Calibration { */
/*   /\* Checks for calibration signal (configurable in InitSpeetures) *\/ */
/*   int calib_check_flag; */
/*   float *calib_check_template; */
/*   float calib_check_prob; */
/*   float calib_check_thresh; */

/* } Calibration; */

/* /\* configures the information needed for the calibration signal prob. */
/*    estimation *\/ */
/* Calibration *Calibration_Create(); */
/* int Calibration_Free(Calibration *c); */
/* int Calibration_Configure(Calibration *c, float *template, int len); */
/* /\* returns the probability of the calibration signal *\/ */
/* float Calibration_GetProb(Speetures *S); */
/* int Calibration_IsCalib(Speetures *S); */



#endif /* SOUNDPROC_H */
