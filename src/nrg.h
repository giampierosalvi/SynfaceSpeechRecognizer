/***************************************************************************
                         nrg.h  -  description
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

#ifndef _NRG_H_
#define _NRG_H_

/* maximum length in frames for the time average */
#define NRGBUFLEN 1024
#define MAXSHORT2 1073741824.0 /* max short squared in float */

typedef struct {
  int nchan;
  int chan;
  double a;  /* last calculated energy  */
  double peak;  /* last calculated peak value */
  double k;  /* recursion factor [0.0 , 1.0]; 
		high value = slow change in output */
  double buf[NRGBUFLEN]; /* circular buffer for long average (calibration) */
  int currPos;            /* current position in the buffer */
} nrg;

nrg* nrg_Create(int nchan, int chan, double k);
double nrg_GetEnergy(nrg* e,short* buf, int len);
double nrg_ReturnEnergy(nrg* e);
double nrg_ReturnAvEnergy(nrg* e, int n);
double nrg_ReturnPeakDb(nrg* e);
void nrg_Delete(nrg* e);

#endif /* _NRG_H_ */
