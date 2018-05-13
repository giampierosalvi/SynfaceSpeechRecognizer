/***************************************************************************
                         nrg.c  -  description
                             -------------------
    begin                : Tue June 3 2003
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

#include "nrg.h"
#include "Common.h"
#include <stdlib.h>
#include <math.h>
#include <float.h> /* to get FLT_MAX */
#define PEAKMIN 1e-9

nrg* nrg_Create(int nchan, int chan, double k) {
  nrg* e;
  e = (nrg*)malloc(sizeof(nrg));
  e->nchan = nchan;
  e->chan = chan;
  e->a = 0;
  e->peak = PEAKMIN;
  e->k = k; 
  memset(e->buf,0,NRGBUFLEN*sizeof(double));
  e->currPos = 0;
  return e;
}

void nrg_Delete(nrg* e) {
  if(e!=NULL) free(e);
}

double nrg_GetEnergy(nrg* e,short* buf, int len) {
  int i;
  double x = 0;

  e->peak *= 0.99;
  if (e->peak < PEAKMIN) e->peak = PEAKMIN;
  for (i=0;i<len;i += e->nchan) {
    x = (1-e->k) * buf[i+e->chan]* buf[i+e->chan]/MAXSHORT2 + e->k * e->a;
    e->a = x;

    /* peak calculation */
    if (abs(buf[i+e->chan]) > e->peak) {

      e->peak = abs(buf[i+e->chan]);
    }
  }

  e->currPos = (e->currPos+1)%NRGBUFLEN;
  e->buf[e->currPos] = x;

  return x;
}

double nrg_GetEnergyFloat(nrg* e,float* buf, int len) {
  int i;
  double x = 0;
  double tmp;

  for (i=0;i<len;i += e->nchan) {
    tmp = buf[i+e->chan]/FLT_MAX;
    x = (1-e->k) * tmp*tmp + e->k * e->a;
    e->a = x;
  }
  e->currPos = (e->currPos+1)%NRGBUFLEN;
  e->buf[e->currPos] = x;

  return x;
}

double nrg_ReturnEnergy(nrg *e) {

  if (e != NULL) {
    return e->a;
  } else {
    return 0.0;
  }
}

void nrg_ReturnPeakDB(nrg *e, double* res) {


  //  double* res;
 
  if (e != NULL && e->peak > 0) {
    *res =  20*log10(e->peak/32768.0);
    //    fprintf(stderr,"peak=%lf (%lf)\n",e->peak,*res);
    //    return res;
  } else {
    *res = -999;
    //    return -999.0;
  }
}

/* returns average energy over min(n,NRGBUFLEN) frames. */
double nrg_ReturnAvEnergy(nrg *e, int n) {
  int i,j;
  double sum=0.0;

  if(n>NRGBUFLEN) n = NRGBUFLEN;

  for(j=i=e->currPos;i>e->currPos-n;i--,j--) {
    if(j<0) j+=NRGBUFLEN;
    sum+=e->buf[j];
  }
  return sum/n;
}
