/***************************************************************************
                         RecMath.h  -  description
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

#ifndef _RECMATH_H_
#define _RECMATH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/* #include <> */

/* extra definitions for MinGW untill I find what's wrong */
#ifndef isnan
 # define isnan(x) \
   (sizeof (x) == sizeof (long double) ? isnan_ld (x) \
   : sizeof (x) == sizeof (double) ? isnan_d (x) \
   : isnan_f (x))
 static inline int isnan_f  (float       x) { return x != x; }
 static inline int isnan_d  (double      x) { return x != x; }
 static inline int isnan_ld (long double x) { return x != x; }
#endif
          
#ifndef isinf
 # define isinf(x) \
   (sizeof (x) == sizeof (long double) ? isinf_ld (x) \
   : sizeof (x) == sizeof (double) ? isinf_d (x) \
   : isinf_f (x))
 static inline int isinf_f  (float       x) { return isnan (x - x); }
 static inline int isinf_d  (double      x) { return isnan (x - x); }
 static inline int isinf_ld (long double x) { return isnan (x - x); }
#endif

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef INT16_MIN
#define INT16_MIN       (-0x7fff-1)
#endif

#ifndef INT16_MAX
#define INT16_MAX       0x7fff
#endif

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

/* type definition */
typedef float  LogFloat;   /* types just to signal log values */

/* -Inf in log domain */
/* #define NINF -3.40282347E+38 */
//#define NINF -3.4e+38f /*-10000000000.0 fix this better */
#define NINF -__builtin_inf()
//#define INF 3.4e+38f /*10000000000.0 fix this better */
#define INF __builtin_inf()
/* e^-91 fix this better, note that log(exp(-92)) gives numerical problems
   with double it is log(exp(-715)) that first gives problems */
#define SMALL_NUM 3.4e-38f

typedef struct {
  float **data;
  int **kind; /* used to distinguish betw. grammar and transitions */
  int **idxs;
  int ncols;
  int *nels;
} SparseMatrix;

typedef struct {
  float **data;
  int ncols;
  int nrows;
} Matrix;

typedef struct {
  float *data;
  int nels;
} Vector;

typedef struct {
  int *data;
  int nels;
} IntVector;

typedef struct {
  char **data;
  int nels;
} CharVector;

void Error(char *message,char *par);

/* log with underflow check */
LogFloat safeLog(float x);

/* real module */
int Mod(int a, int d);

/* Loads a sparse matrix from a text file, allocating memory  */
/* for it; the file format is, for each line:                 */
/* row column value kind                                      */
SparseMatrix *LoadSparseMatrix(char *fn, int matlabformat);

/* Create* functions: call these if the data is already available;
   the data is copied and it is up to the calling procedure to free
   the original vectors */
SparseMatrix *CreateSparseMatrix(int *from, int *to, float *weight,
				  int *kind, int n);
Vector *CreateVector(float *data, int n);
IntVector *CreateIntVector(int *data, int n);

/* Deallocate space for sparse matrix */
void FreeSparseMatrix(SparseMatrix **mptr);
SparseMatrix *DuplicateSparseMatrix(SparseMatrix *in);
int SparseMatrixEqual(SparseMatrix *in1, SparseMatrix *in2);

/* Deallocate space for matrix */
//void FreeMatrix(Matrix *m);

/* Deallocate space for vector */
void FreeVector(Vector **vptr);
void FreeIntVector(IntVector **vptr);
void FreeCharVector(CharVector **vptr);

/* Display a (sparse) matrix */
//void DisplayMatrix (Matrix *m);
void DisplaySparseMatrix (FILE *fh, SparseMatrix *m);

/* Display a vector */
void DisplayVector (FILE *fh, Vector *v);
void DisplayIntVector (FILE *fh, IntVector *v);

/* strip path from file name */
char *basename(char *fname);

#endif /* _RECMATH_H_ */
