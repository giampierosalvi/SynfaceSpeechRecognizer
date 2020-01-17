/***************************************************************************
                          test_recognizer_live.c  -  description
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

#ifndef error
#  define error() return -1
#endif

extern int countInitialSamples;
extern int outputInitialSamples;

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

int display_results(void *data, int idx, int frame_time, double playback_time) {
  Recognizer *r = (Recognizer *) data;
  printf("%d %f %d [%s]\n", frame_time, playback_time, idx, r->ph[idx]); fflush(stdout);
  return 0;
}

int main(int argc, char **argv) {
  Recognizer *r = NULL;
  int i, j, res_idx, res_frame_time;
  char ch;
  double res_playback_time;

  //mtrace();

  /* try this several times to check for memory leaks */
  //for(i=0;i<10;i++) {
  //printf("freing recognizer object if existing...\n");
  //Recognizer_Free(&r);

  printf("creating recognizer object in live mode...\n"); fflush(stdout);
  r = Recognizer_Create(1);

  printf("loading recognition models...\n"); fflush(stdout);
  if(Recognizer_LoadModel(r, "../share/swedish") != 0) {
    fprintf(stderr, "Failed to load model files, aborting.\n");
    exit(1);
  }

  printf("setting callback to display results...\n"); fflush(stdout);
  Recognizer_SetCallback(r, (RecognizerCallbackProc *) &display_results);
    
  /* test three times to check memory management */
  for(i=0; i<3; i++) {
    //while(1) {
    printf("starting the recognizer in synchronous mode...\n"); fflush(stdout);
    Recognizer_Start(r);
    printf("\n\nPress any key to restart the recognizer...\n"); fflush(stdout);
    ch = getchar();// getchar();
    printf("stopping recognizer...\n"); fflush(stdout);
    Recognizer_Stop(r);
  }
  
  printf("setting result callback to NULL...\n"); fflush(stdout);
  Recognizer_SetCallback(r, NULL);
  printf("starting the recognizer in asynchronous mode...\n"); fflush(stdout);
  Recognizer_Start(r);
  
  while(1) {
    int exit=0;
    printf("\n\nPress the following key + enter to:\n  r: show results\n  e: get energy\n  t: show audio stream elapsed time\n  any other key: exit...\n"); fflush(stdout);
    ch = getchar(); getchar();
    switch(ch) {
    case 'r':
      j=0;
      while(Recognizer_GetResult(r, &res_idx, &res_frame_time, &res_playback_time)) {
	printf("%d %f [%s]\n", res_frame_time, res_playback_time, r->ph[res_idx]); fflush(stdout);
	j++;
      }
      printf("got %d\n", j); fflush(stdout);
      break;
    case 'e':
      printf("Audio energy=%f\n", nrg_ReturnEnergy(r->s->nrgY));
      break;
    case 't':
      printf("Audio stream time=%f\n", SoundIO_GetStreamTime(r->s));
      printf("countInitialSamples=%d\n", countInitialSamples);
      printf("r->currFrame=%ld\n", r->currFrame);
      printf("r->s->hasCallback=%d\n", r->s->hasCallback);
      printf("r->asr_in_level_ratio=%f\n", r->asr_in_level_ratio);
      printf("Audio buffer: min=%d max=%d\n", r->s->audio_sample_min, r->s->audio_sample_max);
      printf("ASR buffer:   ");
      printShortArrayRange(r->asr_buffer, r->asr_buffer_len);
      printf("r->features=[");
      printFloatArray(r->lg->sim->input_stream[0], 13);
      printf(" ]\n");
      break;
    default:
      exit=1;
    }
    if (exit) break; 
  }

  printf("stopping recognizer...\n"); fflush(stdout);
  Recognizer_Stop(r);

  printf("cleaning up...\n"); fflush(stdout);
  //ViterbiDecoder_Free(r->vd);
  Recognizer_Free(&r);

  //muntrace();

  return 0;
}
