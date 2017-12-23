/***************************************************************************
                         RecMath.c  -  description
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

#include "RecMath.h"
#include "Common.h"
/* Libraries to use with ViterbiDecoder.c */

/* log with underflow check */
LogFloat safeLog(float x) {
  if(x<SMALL_NUM) return NINF;
  else return (LogFloat) log(x);
}

/* computes the real module (also for negative numbers) */
int Mod(int a, int d) {
  int r = a%d;

  if(a>=0 || r==0) return r;
  else return r+d;
}

/* Allocate space for sparse matrix (used by LoadSparseMatrix) */
SparseMatrix *CreateSparseMatrix(int ncols, int *nels) {
  int i;
  SparseMatrix *m;

  DBGPRINTF("malloc object\n");
  m = (SparseMatrix *) malloc(sizeof(SparseMatrix));
  m->ncols = ncols;
  DBGPRINTF("malloc m->nels\n");
  m->nels = (int *) malloc(ncols*sizeof(int));
  for(i=0;i<ncols;i++) m->nels[i] = nels[i];
  DBGPRINTF("malloc m->data\n");
  m->data = (float **) malloc(ncols*sizeof(float *));
  DBGPRINTF("malloc m->idxs\n");
  m->idxs = (int **) malloc(ncols*sizeof(int *));
  DBGPRINTF("malloc m->kind\n");
  m->kind = (int **) malloc(ncols*sizeof(int *));
  DBGPRINTF("malloc ncols:");
  for(i=0;i<ncols;i++) {
    /* m->col[i].nels = nels[i]; */
    //DBGPRINTF(" [%d] d ",i);
    m->data[i] = (float *) malloc(nels[i]*sizeof(float));
    //DBGPRINTF("i ");
    m->idxs[i] = (int *) malloc(nels[i]*sizeof(int));
    //DBGPRINTF("k");
    m->kind[i] = (int *) malloc(nels[i]*sizeof(int));
  }
  DBGPRINTF("\n");
  return m;
}

SparseMatrix *DuplicateSparseMatrix(SparseMatrix *in) {
  SparseMatrix *out;
  int i,j;

  if(in == NULL) return NULL;
  out = CreateSparseMatrix(in->ncols, in->nels);
  for(i=0;i<in->ncols;i++) {
    for(j=0;j<in->nels[i];j++) {
      out->data[i][j] = in->data[i][j];
      out->idxs[i][j] = in->idxs[i][j];
      out->kind[i][j] = in->kind[i][j];
    }
  }

  return out;
}

int SparseMatrixEqual(SparseMatrix *in1, SparseMatrix *in2) {
  int i,j;

  if(in1 == NULL || in2 == NULL) return -1;
  if(in1->ncols != in2->ncols) return -1;
  for(i=0;i<in1->ncols;i++) {
    if(in1->nels[i] != in2->nels[i]) return -1;
    for(j=0;j<in1->nels[i];j++) {
      if(in1->data[i][j] != in2->data[i][j]) return 0;
      if(in1->idxs[i][j] != in2->idxs[i][j]) return 0;
      if(in1->kind[i][j] != in2->kind[i][j]) return 0;
    }
  }

  return 1;
}


/* Allocate space for vector */
Vector *CreateVector(int nels) {
  Vector *v;

  DBGPRINTF("malloc object\n");
  v = (Vector *) malloc(sizeof(Vector));
  v->nels = nels;
  DBGPRINTF("malloc v->data\n");
  v->data = (float *) malloc (nels*sizeof(float));
  return v;
}

/* Allocate space for vector */
IntVector *CreateIntVector(int nels) {
  IntVector *v;

  DBGPRINTF("malloc object\n");
  v = (IntVector *) malloc(sizeof(IntVector));
  v->nels = nels;
  DBGPRINTF("malloc v->data\n");
  v->data = (int *) malloc (nels*sizeof(int));
  return v;
}

/* Deallocate space for sparse matrix */
void FreeSparseMatrix(SparseMatrix **mptr) {
  SparseMatrix *m = *mptr;
  int i;

  if(m==NULL) return;

  DBGPRINTF("free nclos:");
  for(i=0;i<m->ncols;i++) {
    //    DBGPRINTF(" [%d] d ",i);
    free(m->data[i]);
    //    DBGPRINTF("i ");
    free(m->idxs[i]);
    //    DBGPRINTF("k");
    free(m->kind[i]);
  }
  DBGPRINTF("\n");
  DBGPRINTF("free m->data\n");
  free(m->data);
  DBGPRINTF("free m->idxs\n");
  free(m->idxs);
  DBGPRINTF("free m->kind\n");
  free(m->kind);
  DBGPRINTF("free m->nels\n");
  free(m->nels);
  DBGPRINTF("free object\n");
  free(m);
  m = NULL;
  DBGPRINTF("exiting\n");
}

/* Deallocate space for vector */
void FreeVector(Vector **vptr) {
  Vector *v = *vptr;

  if(v==NULL) return;

  DBGPRINTF("free v->data\n");
  free(v->data);
  DBGPRINTF("free object\n");
  free(v);
  v = NULL;
  DBGPRINTF("exiting\n");
}

/* Deallocate space for vector */
void FreeIntVector(IntVector **vptr) {
  IntVector *v = *vptr;

  if(v==NULL) return;

  DBGPRINTF("free v->data\n");
  free(v->data);
  DBGPRINTF("free object\n");
  free(v);
  v = NULL;
  DBGPRINTF("exiting\n");
}

SparseMatrix *LoadSparseMatrix(char *fn, int matlabformat) {
  FILE *fp;
  int *from;
  int *to;
  float *weight;
  int *kind;
  int n=0,allocstep=2048, allocated;
  SparseMatrix *smat;

  allocated = allocstep;
  /* read in data from file */
  if((fp = fopen(fn,"r"))==NULL) Error("Can't open file",fn);
  from   = (int *)   malloc(allocated*sizeof(int));
  to     = (int *)   malloc(allocated*sizeof(int));
  weight = (float *) malloc(allocated*sizeof(float));
  kind   = (int *)   malloc(allocated*sizeof(int));
  while (!feof(fp)) {
    if(n>=allocated) {
      allocated += allocstep;
      from   = (int *)   realloc(from, allocated*sizeof(int));
      to     = (int *)   realloc(to, allocated*sizeof(int));
      weight = (float *) realloc(weight, allocated*sizeof(float));
      kind   = (int *)   realloc(kind, allocated*sizeof(int));
    }
    fscanf(fp,"%d %d %f %d\n",&(from[n]),&(to[n]),
	   &(weight[n]),&(kind[n]));
    /* printf("line: %d %d %f\n",tr[n].from,tr[n].to,tr[n].weight); */
    if(matlabformat) {
      from[n]--; to[n]--; /* C style indexes from 0 */
    }
    n++;
  }
  if(fclose(fp)) Error("Can't close file",fn);

  from   = (int *)   realloc(from, n*sizeof(int));
  to     = (int *)   realloc(to, n*sizeof(int));
  weight = (float *) realloc(weight, n*sizeof(float));
  kind   = (int *)   realloc(kind, n*sizeof(int));

  smat = CreateSparseMatrixWithData(from, to, weight, kind, n);
  free(from); free(to); free(weight); free(kind);

  return smat;
}

void Error(char *message,char *par) {
  DBGPRINTF("%s: %s\n",message,par);
  exit(0);
}

/* GET RID of this*/
/* void DisplayMatrix (Matrix *m) { */
/*   int i,j; */

/*   printf("Displaying matrix: num. rows = %d, num. cols = %d\n",m->nrows,m->ncols); */
/*   for(i=0;i<m->nrows;i++) { */
/*     for(j=0;j<m->ncols;j++) */
/*       if(m->data[i][j]==NINF) printf("-Inf "); */
/*       else printf("%f ",m->data[i][j]); */
/*     printf("\n"); */
/*   } */
/* } */

void DisplaySparseMatrix (FILE *fh, SparseMatrix *m) {
  int i,j;

  if(m==NULL) {
    fprintf(fh, "the sparse matrix pointer is  NULL\n");
    return;
  }
  fprintf(fh, "Displaying matrix: num. cols = %d\n",m->ncols);
  for(i=0;i<m->ncols;i++) {
    fprintf(fh, "Col %d: num. els = %d\n",i,m->nels[i]);
    for(j=0;j<m->nels[i];j++) {
      fprintf(fh, "  El %d, Row %d, Val %f\n",j,m->idxs[i][j],m->data[i][j]);
    }
  }
}

void DisplayVector (FILE *fh, Vector *v) {
  int i;
  if(v==NULL) {
    fprintf(fh, "the vector pointer is  NULL\n");
    return;
  }
  fprintf(fh, "Displaying vector: num. els = %d\n",v->nels);
  for(i=0;i<v->nels;i++) {
    switch(isinf(v->data[i])) {
    case -1:
      fprintf(fh, "-Inf\n");
      break;
    case 1:
      fprintf(fh, "Inf\n");
      break;
    default:
      fprintf(fh, "%f\n",v->data[i]);
    }
  }
}

void DisplayIntVector (FILE *fh, IntVector *v) {
  int i;

  if(v==NULL) {
    fprintf(fh, "the vector pointer is  NULL\n");
    return;
  }
  fprintf(fh, "Displaying vector: num. els = %d\n",v->nels);
  for(i=0;i<v->nels;i++) {
    fprintf(fh, "%d\n",v->data[i]);
  }
}

/* OBS! System dependent: check that is not used. */
char *basename(char *fname) {
  char *begin;

  begin = fname+strlen(fname);
  while(*begin != '/') begin--;

  return begin+1;
}

/* Create* finctions: call these if the data is already available */
/* remember that the indexes have to be in C style already */
/* inputs must be freed by caller functions */
SparseMatrix *CreateSparseMatrixWithData(int *from, int *to, float *weight, int *kind, int nElements) {
  int i,maxto=0,*maxfrom,rowidx;
  SparseMatrix *smat;

  for(i=0;i<nElements;i++) if(to[i] > maxto) maxto = to[i];
  maxto++; /* from last index to size */
  maxfrom = (int *) malloc(maxto*sizeof(int));
  memset(maxfrom,0,maxto*sizeof(int));
  /* counts the number of elements for each row */
  for(i=0;i<nElements;i++) maxfrom[to[i]]++;
  /* Allocate space for the sparse matrix */
  smat = CreateSparseMatrix(maxto, maxfrom);
  /* fill it with values */
  for(i=0;i<nElements;i++) {
    rowidx = --maxfrom[to[i]];
    smat->data[to[i]][rowidx] = weight[i];
    smat->idxs[to[i]][rowidx] = from[i];
    smat->kind[to[i]][rowidx] = kind[i];
  }
  free(maxfrom);

  return smat;
}

/* the reason to copy the data, instead of just pointing
   to it, is to be consistent with AcquireSparseMatrix */
Vector *CreateVectorWithData(float *data, int n) {
  int i;
  Vector *v;

  v = CreateVector(n);
  for(i=0;i<n;i++) v->data[i] = data[i];

  return v;
}

/* the reason to copy the data, instead of just pointing
   to it, is to be consistent with AcquireSparseMatrix */
IntVector *CreateIntVectorWithData(int *data, int n) {
  int i;
  IntVector *v;

  v = CreateIntVector(n);
  for(i=0;i<n;i++) v->data[i] = data[i];

  return v;
}
