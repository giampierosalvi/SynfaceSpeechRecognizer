/***************************************************************************
                         Recognizer.h  -  description
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

#ifndef _RECOGNIZER_H_
#define _RECOGNIZER_H_

#include "SoundSource.h"
#include "SoundProc.h"
#include "FeatureExtraction.h"
#include "LikelihoodGen.h"
#include "ViterbiDecoder.h"

/* just for debugging */
#include <time.h>

#define MAXQUEUELEN 1024
#define NUMEXTRASYM 2

/* first argument is cast to (Recognizer *) */
typedef int (RecognizerCallbackProc)(void *data, int idx, int frame_time, double playback_time);

/* define to dump intermediate results to file */
//#define DUMPDATA 1

typedef struct {
  int idx;
  int frame_time; /* time of the current frame (ms) (0 = first frame)*/
  double playback_time; /* predicted playback time in seconds */
} Result;

typedef struct {
  Result data[MAXQUEUELEN];
  int in;
  int out;
  int acc;
} ResultQueue;

/* this holds results obtained at the signal processing level, from
   SoundSource and RTSpeetures (without delays due to look-ahead) */
typedef struct {
  int *ismuted;
  int *iscalib;
  int *frame_time; /* current fram time (ms) (0 = first frame) */
  double *playback_time; /* time of predicted playback in seconds */
  int read;
  int write;
  int bufLen;
} DirectResultBuf;

typedef struct {
  int debug;
  long int currFrame;
  short stopped;
  char **ph; /* holds the names of the phones + \n */
  int numOutSym; /* including extra symbols */
  int muteIdx;
  int calibIdx;
  int prevRes; /* index to the previous result */
  int prevTimeHTK; /* time to the previous ressult */

  /* buffers to hold samples to the recognizer */
  int       audio_buffer_len; /* this is to check if the lenght has changed */
  int       asr_buffer_len;
  short    *asr_buffer;

  /* asr input level control */
  float asr_in_level_ratio; /* ratio between device/rcognizer volume */
  float asr_max_input_level; /* used to check for potential overflow */

  float     smp_ratio;  /* ratio between device/recognizer smp rate */

  float *features; /* hold pointer returned by PopFeatures */
  
  RecognizerCallbackProc *callback; /* callback procedure for synchronous mode */

  /* processing units */
  SoundSource        *s;  /* audio input */
  Decimator          *d;  /* resampling (downsampling) */
  FeatureExtraction  *fe; /* MFCC extraction */
  LikelihoodGen      *lg; /* Recurrent Neural Network */
  ViterbiDecoder     *vd; /* HMM decoding */
  ResultQueue        *rq; /* this is where the results are stored */
  DirectResultBuf    *dr; /* buffer for direct results */

} Recognizer;

/* high level functions (they should be used by the application)*/
/* Create recognizer does not load the ANN */
Recognizer *Recognizer_Create(int liveAudio);
int Recognizer_GetOutSym(Recognizer *r);
int Recognizer_Free(Recognizer **rptr);
int Recognizer_Start(Recognizer *r);
int Recognizer_Stop(Recognizer *r);

/* consumes num_samples samples of speech, eventually advances one
   or more frames */
int Recognizer_PushSpeech(Recognizer *r, const short *audio_buffer, int audio_buffer_len);
/* consumes a frame and executes the RecCallback */
void Recognizer_ConsumeFrame(Recognizer *r);
/* returns 1 if there is a result to return or 0 otherwise */
int Recognizer_GetResult(Recognizer *r, int *idx, int *frame_time, double *playback_time);
int Recognizer_SetLookahead(Recognizer *r, int lookahead);

void Recognizer_SetDebug(Recognizer *r, int level);

/* Normally, resuts are feched asynchronously with Recognizer_GetResult. The reason
   for this is the design of the face animation in Tcl. If you want to get results
   synchronously, you need to register a callback through this function. */
void Recognizer_SetCallback(Recognizer *r, RecognizerCallbackProc *proc);

#endif /* _RECOGNIZER_H_ */
