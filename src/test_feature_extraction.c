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

#include "FeatureExtraction.h"
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
  FeatureExtraction *fe;
  int j, res_idx, res_frame_time, prev_frame_time = 0;
  double res_playback_time;
  FILE *ff = NULL, *of = NULL;
  short wavbuf[WAVBUFSIZE];
  char *infile, *outfile, *dataroot, *lang;
  int n;
  int offset=120;

  //  mtrace();
  srand(0);

  if (argc == 3) {
    infile = argv[1];
    outfile = argv[2];
  } else {
    fprintf(stderr,"usage: %s <input audio file> <output feature file>\n",argv[0]);
    exit(1);
  }

  ff = fopen(infile,"rb");
  if (ff == NULL) {
    printf("failed to open file %s, exiting\n",infile);
    exit(1);
  }

  fe = FeatureExtraction_Create();

  fprintf(stderr, "configuring feature extraction...\n"); fflush(stderr);
  fe->inputrate = 8000;
  fe->numfilters = 24;
  fe->numceps = 12;
  fe->lowfreq = 0.0;
  fe->hifreq = 4000.0;
  fe->framestep = 0.01;
  fe->framelen = 0.025;
  fe->preemph = 0.97;
  fe->lifter = 22.0;

 
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
