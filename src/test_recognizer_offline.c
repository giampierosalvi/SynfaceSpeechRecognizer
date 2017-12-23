/***************************************************************************
                          test_recognizer2.c  -  description
                             -------------------
    begin                : Mon Dec 10 2012
    copyright            : (C) 2012-2017 by Giampiero Salvi
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

#include "Recognizer.h"
#include <unistd.h> // for sleep()
#include <limits.h>

//#include <mcheck.h>

/* Note that configuration of the recognizer is more complicated than
   it should. This is due to the problems related with Tcl integration
   and working with wrapped Tcl code. */

// 16 MB

#define MAX_FILE_SIZE 16777216
#ifndef error
#  define error() return -1
#endif
#define WAVBUFSIZE 16000

extern int countInitialSamples;
extern int outputInitialSamples;


/* there is a simpler function in LikelihoodGen.h, but this is the way it's done from Tcl */
int ReadANN(Recognizer *r, char *filename) {
  FILE *f;
  unsigned char *buffer;
  int n;
  BinaryBuffer *binbuf;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    error();
  }
  buffer = (unsigned char *) malloc(MAX_FILE_SIZE*sizeof(unsigned char));
  n = fread(buffer, 1, MAX_FILE_SIZE, f);
  fclose(f);
  binbuf = BinaryBuffer_Create((unsigned char *) buffer, n);
  free(buffer);
  LGLoadANN(r->lg, binbuf);
  BinaryBuffer_Free(binbuf);
  Recognizer_GetOutSym(r);

  return 0;
}

int ReadPhPrior(Recognizer *r, char *filename) {
  FILE *f;
  float tmp[200]; // priors are usually around of length 50
  int n = 0;
  Vector *pp;

  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    error();
  }
  while(fscanf(f, "%f\n", &tmp[n]) != EOF) n++;
  fclose(f);

  pp = CreateVectorWithData(tmp,n);
  LikelihoodGen_SetPhPrior(r->lg,pp);
  return 0;
}

int ReadGrammar(Recognizer *r, char *filebasename, float gramfact, float insweight) {
  FILE *f;
  char filename[512];
  int n = 0;
  int from[4000], to[4000], type[4000];
  float weight[4000];
  SparseMatrix *transmat;
  Vector *stPrior;
  IntVector *fisStateId;

  /* transition matrix */
  sprintf(filename, "%s.transmat", filebasename);
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%d %d %f %d\n", &from[n], &to[n], &weight[n], &type[n]) != EOF) {
    from[n]--; to[n]--; // convert from matlab to C indexes
    // here check Inf values (but there are none in the files I have checked)
    weight[n] *= -1;
    if(type[n]) // apply grammar factor and insertion weight
      weight[n] = weight[n] * gramfact + insweight;
    n++;
  }
  fclose(f);
  transmat = CreateSparseMatrixWithData(from, to, weight, type, n);

  /* state prior */
  sprintf(filename, "%s.prior", filebasename);
  n=0;
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%f\n", &weight[n]) != EOF) {
    from[n] *= -1; // this should work with the inf valuse as well
    n++;
  }
  fclose(f);
  stPrior = CreateVectorWithData(weight,n);

  /* fis state id */
  sprintf(filename, "%s.map", filebasename);
  n=0;
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%d\n", &from[n]) != EOF) {
    from[n]--; // convert from matlab to C indexes
    n++;
  }
  fclose(f);
  fisStateId = CreateIntVectorWithData(from,n);

  ViterbiDecoder_SetGrammar(r->vd,transmat,stPrior,fisStateId);
  return 0;
}

void printFloatArray(float *a, int len) {
  int i;
  for(i=0; i<len; i++) printf(" %f", a[i]);
}
void printShortArray(short *a, int len) {
  int i;
  for(i=0; i<len; i++) printf(" %d", a[i]);
}
void printShortArrayRange(short *a, int len) {
  short smin = SHRT_MAX;
  short smax = SHRT_MIN;
  int i;
  for(i=0; i<len; i++) {
    if (a[i]<smin) smin = a[i];
    if (a[i]>smax) smax = a[i];
  }
  printf("min=%d max=%d\n", smin, smax);
}

int main(int argc, char **argv) {
  Recognizer *r = NULL;
  int j, res_idx, res_frame_time, prev_frame_time = 0;
  double res_playback_time;
  FILE *ff = NULL, *of = NULL;
  short wavbuf[WAVBUFSIZE];
  char stringbuf_ann[1024];
  char stringbuf_pri[1024];
  char stringbuf_gram[1024];
  char *infile, *outfile, *dataroot, *lang;
  int n;
  int offset=120;

  //  mtrace();
  srand(0);

  if (argc > 4) {
    dataroot = argv[1];
    lang = argv[2];
    infile = argv[3];
    outfile = argv[4];
  } else {
    fprintf(stderr,"usage: %s <data dir> <lang (sv,en,de,fl)> <input audio file> <output lab file>\n",argv[0]);
    exit(1);
  }

  ff = fopen(infile,"rb");
  if (ff == NULL) {
    printf("failed to open file %s, exiting\n",infile);
    exit(1);
  }

  r = Recognizer_Create(0);

  //  printf("configuring feature extraction...\n"); fflush(stdout);
  r->fe->inputrate = 8000;
  r->fe->numfilters = 24;
  r->fe->numceps = 12;
  r->fe->lowfreq = 0.0;
  r->fe->hifreq = 4000.0;
  r->fe->framestep = 0.01;
  r->fe->framelen = 0.025;
  r->fe->preemph = 0.97;
  r->fe->lifter = 22.0;

 
  sprintf(stringbuf_ann,"%s/spdatnet_%s.rtd",dataroot,lang);
  sprintf(stringbuf_pri,"%s/ph_prior_%s.txt",dataroot,lang);
  sprintf(stringbuf_gram,"%s/netsimple_%s",dataroot,lang);

  printf("%s\n%s\n%s\n",stringbuf_ann,stringbuf_pri,stringbuf_gram);
  //  printf("configuring neural network...\n"); fflush(stdout);
  ReadANN(r, stringbuf_ann);
  ReadPhPrior(r, stringbuf_pri);
  //  printf("configuring Viterbi decoder...\n"); fflush(stdout);
  r->vd = ViterbiDecoder_Create();
  ReadGrammar(r, stringbuf_gram, 1, 0);
  ViterbiDecoder_SetFrameLen(r->vd, LikelihoodGen_GetOutSize(r->lg));
  Recognizer_SetLookahead(r, 3);

  //  printf("starting the recognizer...\n"); fflush(stdout);
  Recognizer_Start(r);

  of = fopen(outfile,"w");

  j=0;
  while (!feof(ff)) {
    n = fread(wavbuf, sizeof(short), WAVBUFSIZE, ff);
    // printf("pushing %d samples\n",n);
    Recognizer_PushSpeech(r,wavbuf,n);
    while(Recognizer_GetResult(r, &res_idx, &res_frame_time, &res_playback_time)) {
      res_frame_time +=offset;
      fprintf(of,"%f %f %s\n", prev_frame_time*.001, res_frame_time*.001, r->ph[res_idx]);
      prev_frame_time = res_frame_time;
      j++;
    }
  }
  fclose(of);
  //  printf("got %d\n", j); fflush(stdout);
  
  //  printf("stopping recognizer...\n"); fflush(stdout);
  Recognizer_Stop(r);
  
  //  printf("cleaning up...\n"); fflush(stdout);
  //ViterbiDecoder_Free(r->vd);
  Recognizer_Free(&r);
  
  //   muntrace();
  
  return 0;
}
