/***************************************************************************
                          test_recognizer_offline.c  -  description
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
#include <sndfile.h> /* opening and reading sound files */

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


int ReadGrammarToBeDeleted(Recognizer *r, char *modelDir, float gramfact, float insweight) {
  FILE *f;
  char filename[512];
  int n = 0;
  int from[4000], to[4000], type[4000];
  float weight[4000];
  SparseMatrix *transmat;
  Vector *stPrior;
  IntVector *fisStateId;

  /* transition matrix */
  sprintf(filename, "%s/hmm_transmat.txt", modelDir);
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
  transmat = SparseMatrix_CreateWithData(from, to, weight, type, n);

  /* state prior */
  sprintf(filename, "%s/hmm_prior.txt", modelDir);
  n=0;
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%f\n", &weight[n]) != EOF) {
    from[n] *= -1; // this should work with the inf value as well
    n++;
  }
  fclose(f);
  stPrior = Vector_CrearteWithData(weight,n);

  /* fis state id */
  sprintf(filename, "%s/hmm_map.txt", modelDir);
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
  fisStateId = IntVector_CreateWithData(from,n);

  ViterbiDecoder_SetGrammar(r->vd,transmat,stPrior,fisStateId);
  return 0;
}

void printFloatArray(float *a, int len) {
  int i;
  for(i=0; i<len; i++) printf(" %f", a[i]);
}
void printShortArray(short *a, int len) {
  int i;
  for(i=0; i<len; i++) printf(" %hd", a[i]);
}
void printShortArrayRange(short *a, int len) {
  short smin = SHRT_MAX;
  short smax = SHRT_MIN;
  int i;
  for(i=0; i<len; i++) {
    if (a[i]<smin) smin = a[i];
    if (a[i]>smax) smax = a[i];
  }
  printf("min=%hd max=%hd\n", smin, smax);
}

int main(int argc, char **argv) {
  Recognizer *r = NULL;
  int j, res_idx, res_frame_time, prev_frame_time = 0;
  double res_playback_time;
  SNDFILE *ff = NULL;
  SF_INFO sf_info;
  FILE *of = NULL;
  short wavbuf[WAVBUFSIZE];
  char *infile, *outfile, *confFilename;
  int n;
  int offset=120;

  //  mtrace();
  srand(0);
  if (argc == 4) {
    confFilename = argv[1];
    infile = argv[2];
    outfile = argv[3];
  } else {
    fprintf(stderr,"usage: %s <conf file> <input audio file> <output lab file>\n",argv[0]);
    exit(1);
  }

  ff = sf_open(infile, SFM_READ, &sf_info);
  if (ff == NULL) {
    fprintf(stderr, "failed to open file %s, exiting\n",infile);
    fprintf(stderr, "%s\n", sf_strerror(ff));
    exit(1);
  } else {
    fprintf(stderr, "opened sound file with [n=%ld, sf=%d, ch=%d, fmt=%d]\n",
            sf_info.frames, sf_info.samplerate, sf_info.channels, sf_info.format);
  }

  r = Recognizer_Create(0);
  
  if(Configuration_ApplyConfigFromFilename(r, confFilename) != 0) {
  //if(Recognizer_LoadModel(r, confFilename) != 0) {
    fprintf(stderr, "Failed to load model files, aborting.\n");
    exit(1);
  }         
  
  //  printf("starting the recognizer...\n"); fflush(stdout);
  Recognizer_Start(r);

  of = fopen(outfile,"w");

  j=0;
  while ((n=sf_read_short(ff, wavbuf, WAVBUFSIZE))) {
    // printf("pushing %d samples\n",n);
    Recognizer_PushSpeech(r,wavbuf,n);
    while(Recognizer_GetResult(r, &res_idx, &res_frame_time, &res_playback_time)) {
      res_frame_time +=offset;
      fprintf(of,"%f %f %s\n", prev_frame_time*.001, res_frame_time*.001, r->ph[res_idx]);
      prev_frame_time = res_frame_time;
      j++;
    }
  }
  sf_close(ff);
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
