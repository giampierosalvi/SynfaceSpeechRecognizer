/***************************************************************************
                         Recognizer.c  -  description
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

#include "Recognizer.h"
#include "Common.h"
#include "LowpassFilter.h"
#include <stdio.h>
#include <limits.h>

#define SMPRATE 16000

/* internal function definition */
/* sets delay in the direct result buffer */
int DirectResultBuf_SetDelay(DirectResultBuf* dr, int delay);
void Recognizer_ApplyVolume(Recognizer *r);


/* result queue related functions */
int ResultQueue_Push(ResultQueue *rq, int idx, int frame_time, double playback_time);
Result *ResultQueue_Pop(ResultQueue *rq);
/* direct result buffer */
DirectResultBuf* DirectResultBuf_Create(int bufLen);
int DirectResultBuf_Free(DirectResultBuf **drptr);

/* decoding methods */
int MaximumAPosteriori(float *app, int size);
// to be implemented
//int MaximumLikelihood(Recognizer *r);

/* here we allocate the objects that are not dynamically configurable,
   or for which dynamic configuration doesn't require memory allocation.

   liveAudio=1: use the soundcard to produce speech samples
   liveAudio=0: speech samples are pushed by the application with Recognizer_PushSpeech
*/
Recognizer *Recognizer_Create(int liveAudio) {
  Recognizer *r;
  //int i;

  DBGPRINTF("creating Recognizer object...\n");
  r = (Recognizer *) malloc(sizeof(Recognizer));
  r->currFrame = 0;
  r->stopped = 1;
  r->ph = NULL;
  r->numOutSym = 0;
  r->muteIdx = 0;
  r->calibIdx = 0;
  r->prevRes = 0;
  r->prevTimeHTK = 0;

  /* sound buffers to the recognizer */
  r->audio_buffer_len = 0;
  r->asr_buffer_len = 0;
  r->asr_buffer = NULL;

  /* ratio between device and asr input level */
  r->asr_in_level_ratio = 1.0;
  r->asr_max_input_level = SHRT_MAX;

  if (liveAudio) {
    DBGPRINTF("initialize audio...\n");
    r->s = SoundIO_Create();
  } else {
    r->s = NULL;
  }

  DBGPRINTF("creating downsampling structure...\n");
  r->d = Decimator_Create();

  DBGPRINTF("creating feature extraction structure...\n");
  r->fe = FeatureExtraction_Create();

  DBGPRINTF("creating likelihood generator structure...\n");
  r->lg = LikelihoodGen_Create();

  if (r->s) {
    DBGPRINTF("setting SoundIO callback\n");
    SoundIO_SetCallback(r->s, (SoundIOCallbackProc *) &Recognizer_PushSpeech,
			    (void *) r);
  }
  DBGPRINTF("creating viterbi decoder structure...\n");
  r->vd = ViterbiDecoder_Create(); // we create this even if it is not used

  /* result queue */
  DBGPRINTF("creating result queue...\n");
  r->rq = (ResultQueue *) malloc(sizeof(ResultQueue));
  r->rq->in = 0;
  r->rq->out = 0;
  r->rq->acc = 0;

  /* buffer for direct results (used only with viterbi decoder when
     the results coming from SoundIO and RTSpeetures need to be
     realigned with the lookahead). By default (no viterbi) the delay
     is 0. This is modified from tcl when viterbi is created. */
  DBGPRINTF("creating direct result buffer\n");
  r->dr = DirectResultBuf_Create(0);

  r->currFrame = 0;

  r->callback = NULL;
  
  Recognizer_SetDebug(r, 0);

  DBGPRINTF("exiting\n");
  return r;
}

int Recognizer_GetOutSym(Recognizer *r) {
  int i;
  int len;

  if(r->ph != NULL) {
    if(r->debug>0) {
      DBGPRINTF("r->ph is not NULL: freing...\n");
    }
    for(i=0; i<r->numOutSym;i++) {
      if(r->ph[i] != NULL)
	free(r->ph[i]);
    }
    free(r->ph);
    r->ph = NULL;
  }

  /* NEURAL NETWORK */
  if(r->lg == NULL) return -1;
  if(r->lg->sim == NULL) return -1;

  /* the socket communication is deprecated. Now the array is used
     to hold the output symbols, including extra signals (appended
     in the end of the array). At the moment only "muted" and "calibration"
     are included. The final \n has been removed as well */
  r->numOutSym = r->lg->outputStreamSizes[0]+NUMEXTRASYM;
  DBGPRINTF("before r->ph allocation\n");
  r->ph = (char **) malloc(r->numOutSym*sizeof(char *));
  for(i=0; i<r->lg->outputStreamSizes[0];i++) {
    len = strlen(r->lg->sim->output_component_name[0][i]);
    r->ph[i] = (char *) malloc((len+1)*sizeof(char));
    strcpy(r->ph[i],r->lg->sim->output_component_name[0][i]);
    //r->ph[i][len] = '\n'; r->ph[i][len+1] = '\0';
  }
  /* here add one entry for each extra symbol */
  DBGPRINTF("setting the extra symbols\n");
  r->muteIdx = r->lg->outputStreamSizes[0];
  len = strlen("muted");
  r->ph[r->muteIdx] = (char *) malloc((len+1)*sizeof(char));
  strcpy(r->ph[r->muteIdx],"muted");
  r->calibIdx = r->lg->outputStreamSizes[0]+1;
  len = strlen("calibration");
  r->ph[r->calibIdx] = (char *) malloc((len+1)*sizeof(char));
  strcpy(r->ph[r->calibIdx],"calibration");

  return 0;
}

/* everything is more dynamic now: check that, when this
   function is called, the pointers aren't void */
int Recognizer_Free(Recognizer **rptr) {
  Recognizer *r = *rptr;
  int i;

  if(r == NULL) return 0;

  Recognizer_Stop(r);

  if (r->s) {
    //DBGPRINTF("freing SoundIO...\n");
    if(SoundIO_Free(&r->s)!=0)
      DBGPRINTF("failed freing SoundIO");
  }
  DBGPRINTF("freing Decimator...\n");
  if(Decimator_Free(&r->d)!=0)
    DBGPRINTF("failed freing Decimator\n");

  DBGPRINTF("freing buffers...\n");
  if(r->asr_buffer != NULL) free(r->asr_buffer); r->asr_buffer = NULL;

  DBGPRINTF("freing FeatureExtraction...\n");
  if(FeatureExtraction_Free(&r->fe)!=0)
    DBGPRINTF("failed freing FeatureExtraction\n");

  if(r->debug>0) DBGPRINTF("freing LikelihoodGen...\n");
  if(LikelihoodGen_Free(&r->lg)!=0)
    DBGPRINTF("failed freing LikelihoodGen\n");

  DBGPRINTF("freing ViterbiDecoder...\n");
  if(ViterbiDecoder_Free(&r->vd)!=0)
    DBGPRINTF("failed freing ViterbiDecoder");

  DBGPRINTF("freing ResultQueue...\n");
  free(r->rq);

  if(r->ph != NULL) {
    DBGPRINTF("freing output symbols...\n");
    for(i=0; i<r->numOutSym; i++) {
      if(r->ph[i]==NULL) {
	DBGPRINTF("empty symbol!!!\n");
      }
      else {
	if(r->debug>0) {
	  DBGPRINTF("freing symbol [%d][%s]\n",i,r->ph[i]);
	}
	free(r->ph[i]);
      }
    }
    free(r->ph);
  }

  DBGPRINTF("freing DirectResultBuf...\n");
  DirectResultBuf_Free(&r->dr);

  DBGPRINTF("freing Recognizer...\n");
  free(r);
  *rptr = NULL;
  DBGPRINTF("freing Recognizer succeded!\n");

  return 0;
}

int Recognizer_SetLookahead(Recognizer *r, int lookahead) {
  if(r->vd!=NULL) ViterbiDecoder_SetLookahead(r->vd,lookahead);
  if(r->dr!=NULL) DirectResultBuf_SetDelay(r->dr,lookahead);
  /* ...check if more settings are needed, for example some smart
     way to change the feedback delay */
  return 0;
}

/* problem: results coming from SoundIO (muted) and RTSpeetures
   (calibration) are instantaneous, while results coming from the
   recognizer (phones) are delayed vd->maxDelay frames. I need a
   buffer to store the direct results. */
DirectResultBuf* DirectResultBuf_Create(int delay) {
  DirectResultBuf* dr = NULL;

  dr = (DirectResultBuf *) malloc(sizeof(DirectResultBuf));
  dr->ismuted = NULL;
  dr->iscalib = NULL;
  dr->frame_time = NULL;
  dr->playback_time = NULL;
  DirectResultBuf_SetDelay(dr,delay);

  return(dr);
}

int DirectResultBuf_SetDelay(DirectResultBuf* dr, int delay) {
  int bufLen = delay+1;

  if(delay < 0) return -1;

  dr->bufLen = bufLen;
  if(dr->ismuted != NULL) free(dr->ismuted);
  dr->ismuted = (int *) malloc(bufLen*sizeof(int));
  memset(dr->ismuted,0,bufLen*sizeof(int));
  if(dr->iscalib != NULL) free(dr->iscalib);
  dr->iscalib = (int *) malloc(bufLen*sizeof(int));
  memset(dr->iscalib,0,bufLen*sizeof(int));
  if(dr->frame_time != NULL) free(dr->frame_time);
  dr->frame_time = (int *) malloc(bufLen*sizeof(int));
  memset(dr->frame_time,0,bufLen*sizeof(int));
  if(dr->playback_time != NULL) free(dr->playback_time);
  dr->playback_time = (double *) malloc(bufLen*sizeof(double));
  memset(dr->playback_time,0,bufLen*sizeof(double));
  if(bufLen>1) dr->read = 1;
  else dr->read = 0; /* start reading bufLen-1 steps back */
  dr->write = 0;

  return 0;
}

int DirectResultBuf_Free(DirectResultBuf **drptr) {
  DirectResultBuf *dr = *drptr;

  if(dr == NULL) return 0;

  if(dr->ismuted != NULL) free(dr->ismuted);
  if(dr->iscalib != NULL) free(dr->iscalib);
  if(dr->frame_time != NULL) free(dr->frame_time);
  if(dr->playback_time != NULL) free(dr->playback_time);
  free(dr);
  dr = NULL;

  return 0;
}

//int checkbuf = 0;

/* internal to Recognizer.c */
int DirectResultBuf_Update(DirectResultBuf *dr, int ismuted, int iscalib, int frame_time, double playback_time ) {

  if(dr == NULL) return 0;

  dr->write = (dr->write+1)%dr->bufLen;
  dr->read = (dr->read+1)%dr->bufLen;
  dr->ismuted[dr->write] = ismuted;
  dr->iscalib[dr->write] = iscalib;
  dr->frame_time[dr->write] = frame_time;
  dr->playback_time[dr->write] = playback_time;

  return 0;
}

/* internal to Recognizer.c */
int DirectResultBuf_IsMuted(DirectResultBuf *dr) {
  return dr->ismuted[dr->read];
}

/* internal to Recognizer.c */
int DirectResultBuf_IsCalib(DirectResultBuf *dr) {
  return dr->iscalib[dr->read];
}

/* internal to Recognizer.c */
int DirectResultBuf_FrameTime(DirectResultBuf *dr) {
  return dr->frame_time[dr->read];
}

/* internal to Recognizer.c */
double DirectResultBuf_PlaybackTime(DirectResultBuf *dr) {
  return dr->playback_time[dr->read];
}

/* This is the processing chain in this version of the recognizer: rather than
   building a chain of objects as in the asr package
   SoundIO -> SoundProc -> Features -> Likelihood -> Viterbi
   we have a flat architecture where the Recognizer object controls everything
   SoundIO ->  Recognizer
                    |-> Decimator
                    |-> FeatureExtraction
                    |-> Likelihood
                    |    `-> Features
                    `-> Viterbi
*/

/* int Recognizer_AllocateBuffers(Recognizer *r, int rec_buf_blocklen) { */
/*   /\* send data at least every 10 ms *\/ */
/*   int audioSamples10ms = r->s->samplingRate/100; */

/*   DBGPRINTF("entering\n"); */
/*   r->rec_buf_len = (int) audioSamples10ms/r->smp_ratio; */
/*   r->rec_buf_blocklen = rec_buf_blocklen; */
/*   if(r->rec_buf_len < rec_buf_blocklen) { */
/*     DBGPRINTF("rec_buf_blocklen > than 10 ms\n") */
/*     r->rec_buf_len = rec_buf_blocklen; */
/*     r->rec_buf_nblocks = 1; */
/*   } else { */
/*     r->rec_buf_nblocks = r->rec_buf_len/r->rec_buf_blocklen; */
/*     if(r->rec_buf_len % r->rec_buf_blocklen) { */
/*       DBGPRINTF("warning: rec_buf_blocklen not multiple of 10ms!!!\n"); */
/*     } */
/*   } */

/*   //  r->rec_buf_nblocks = r->s->samplingRate/(100*r->s->framesPerBuffer); */
/*   //if(r->rec_buf_nblocks<1) r->rec_buf_nblocks = 1; */
/*   //r->rec_buf_blocklen = (int) r->s->framesPerBuffer/r->smp_ratio; */
/*   //r->rec_buf_len = r->rec_buf_blocklen * r->rec_buf_nblocks; */
/*   DBGPRINTF("allocating temporary buffers...\n"); */
/*   DBGPRINTF("  rec_buf_len = %d\n", r->rec_buf_len); */
/*   DBGPRINTF("  rec_buf_blocklen = %d\n", r->rec_buf_blocklen); */
/*   DBGPRINTF("  rec_buf_nblocks = %d\n", r->rec_buf_nblocks); */
/*   if(r->rec_buf != NULL) free(r->rec_buf); */
/*   r->rec_buf = (short *) malloc(r->rec_buf_len*sizeof(short)); */
/*   r->rec_buf_ptr = r->rec_buf; */
/*   r->rec_buf_block = 0; */

/*   return 0; */
/* } */

void Recognizer_ApplyVolume(Recognizer *r) {
  int i;

  /* adjust input level for ASR */
  if(r->asr_in_level_ratio != 1.0) {
    short *pout = r->asr_buffer;
    /* we split this into two cases for efficiency */
    if(r->asr_in_level_ratio < 1.0) {
      /* no need to bother about clipping */
      for(i=0;i<r->asr_buffer_len;i++) {
	*pout = (short) (r->asr_in_level_ratio * (*pout));
	pout++;
      }
    } else { /* s->asr_in_level_ratio > 1.0 */
      /* here we might be clipping */
      for(i=0;i<r->asr_buffer_len;i++) {
	/* clipping */
	if(abs(*pout)>= r->asr_max_input_level) {
	  *pout = *pout>0 ? SHRT_MAX:SHRT_MIN; 
	} else {
	  *pout = (short) (r->asr_in_level_ratio * (*pout));
	}
	pout++;
      }
    }
  }
}

int countInitialSamples = 0;
int outputInitialSamples = 1;
int Recognizer_PushSpeech(Recognizer *r, const short *audio_buffer, int audio_buffer_len) {
  int frames = 0;
  //  int i;
  
  if(audio_buffer_len != r->audio_buffer_len) {
    DBGPRINTF("audio buffer length has changed, reallocating asr buffer...\n");
    r->audio_buffer_len = audio_buffer_len;
    r->asr_buffer_len = (int) audio_buffer_len/r->smp_ratio;
    if(r->asr_buffer == NULL)
      r->asr_buffer = (short *) malloc(r->asr_buffer_len*sizeof(short));
    else
      r->asr_buffer = (short *) realloc(r->asr_buffer, r->asr_buffer_len*sizeof(short));
  }

  /* downsample */
  Decimator_ProcessBuffer(r->d, r->asr_buffer, audio_buffer, audio_buffer_len);

  /* adjust volume */
  Recognizer_ApplyVolume(r);

  /* Pushing speech into the frame generator */
  FeatureExtraction_PushSpeech(r->fe, r->asr_buffer, r->asr_buffer_len);
  countInitialSamples += r->asr_buffer_len;

  /* Get feature vector if there are enough samples
     (used to be part of LikelihoodGen_ConsumeFrame) */
  while((r->features = FeatureExtraction_PopFeatures(r->fe)) != NULL) {
    /* copy data to simulator (should change to memcpy) */
    int i;
    for (i = 0; i < r->lg->inputsize; i++) {
      r->lg->sim->input_stream[0][i] = r->features[i];
    }
    free(r->features);

    if(LikelihoodGen_ConsumeFrame(r->lg)) {
      if(outputInitialSamples) {
	DBGPRINTF("initial samples for first frame = %d\n", countInitialSamples);
	DBGPRINTF("max_delay in the NN = %d\n",r->lg->sim->max_delay);
	outputInitialSamples = 0;
      }
      Recognizer_ConsumeFrame(r);
      frames++;
    }
  }

  return frames;
}

//int debug =0;

void printmax(float* v,int size, int res) {
  int i,imax=-1;
  float vmax=0;
  static int count=0;
  
  if ((count++)%10==0) {
    for (i=0;i<size;i++) {
      if (v[i]>vmax) {
	vmax=v[i];
	imax=i;
      }
    }
    printf("%d(%f) res=%d\n",imax,vmax,res);
  }
}


void Recognizer_ConsumeFrame(Recognizer *r) {
  int res = 0;
  int idx, frame_time;
  float playback_time;
  
  if(r->vd != NULL) {
    res = ViterbiDecoder_ConsumeFrame(r->vd, r->lg->lh,
				      r->lg->outputStreamSizes[0]);
#ifdef xxDEBUG
    printmax( r->lg->lh, r->lg->outputStreamSizes[0],res);
#endif
  }
  else res = MaximumAPosteriori(r->lg->lh, r->lg->outputStreamSizes[0]);
  r->currFrame++;

  //  if(debug++%100) DBGPRINTF("Recognizer_ConsumeFrame res=%d\n",res);

  /* first get the istantaneous values and update the buffer... */
  if (r->s) {
    DirectResultBuf_Update(r->dr,
			   SoundIO_IsMuted(r->s),
			   0, //Speetures_IsCalib(S),
			   r->currFrame*10, // make time flexible!!
			   SoundIO_GetStreamTime(r->s));
  } else {
    DirectResultBuf_Update(r->dr,
			   0,
			   0, //Speetures_IsCalib(S),
			   r->currFrame*10, // make time flexible!!
			   countInitialSamples);
   
  }
  /* ...then get the delayed values. */
  if(DirectResultBuf_IsMuted(r->dr)) res = r->muteIdx;
  if(DirectResultBuf_IsCalib(r->dr)) res = r->calibIdx;

  if(res != r->prevRes) {
    int currTimeHTK = DirectResultBuf_FrameTime(r->dr)*10000;
    ResultQueue_Push(r->rq,
		     res,
		     DirectResultBuf_FrameTime(r->dr),
		     DirectResultBuf_PlaybackTime(r->dr));  
    r->prevRes = res;
    r->prevTimeHTK = currTimeHTK;

    /* get results for synchronous processing */
    if(r->callback != NULL) {
      Recognizer_GetResult(r, &idx, &frame_time, &playback_time);
      r->callback((void *) r, idx, frame_time, (double) playback_time);
    }
  }
}

int Recognizer_Start(Recognizer *r) {
  int err;
  double lowpasscoeff2[] = LOWPASS_FACTOR_2_FILTER;
  int lowpasslen2 = LOWPASS_FACTOR_2_LEN;
  double lowpasscoeff4[] = LOWPASS_FACTOR_4_FILTER;
  int lowpasslen4 = LOWPASS_FACTOR_4_LEN;
  double lowpasscoeff6[] = LOWPASS_FACTOR_6_FILTER;
  int lowpasslen6 = LOWPASS_FACTOR_6_LEN;
  /* this will be modified when the synchronous processing
     is fully implemented. Now it's done */

  DBGPRINTF("entering\n");
  if(!r->stopped) return 0;

  /* some initialization */
  r->currFrame = 0;
  r->prevRes = 0;
  r->prevTimeHTK = 0;

  if (r->s != NULL) {
    DBGPRINTF("opening audio device\n");
    err = SoundIO_Open(r->s, NULL, 0);
    if(err != 0) { /* fix this*/
      DBGPRINTF("failed opening device\n");
      return err;
    }
  }
  /* check that this function does all is necessary */
  DBGPRINTF("activating FeatureExtraction\n");
  FeatureExtraction_Activate(r->fe);
  DBGPRINTF("activating LikelihoodGen\n");
  LikelihoodGen_Activate(r->lg);

  if (r->s != NULL) {
    r->smp_ratio = r->s->samplingRate/r->fe->inputrate;
  } else {
    // SMPRATE should come from the file input
    r->smp_ratio = SMPRATE/r->fe->inputrate;
  }
  /* buffers allocation moved to Recognizer_PushSpeech and
     Recognizer_AllocateBuffers */

  /* configure deciamtor */
  DBGPRINTF("configuring Decimator... ratio\n");
  //  printf("smp_ratio = %d\n",(int) r->smp_ratio);
  switch((int) r->smp_ratio) {
  case 2:
    Decimator_ConfigureFilter(r->d, lowpasscoeff2, lowpasslen2); break;
  case 4:
    Decimator_ConfigureFilter(r->d, lowpasscoeff4, lowpasslen4); break;
  case 6:
    Decimator_ConfigureFilter(r->d, lowpasscoeff6, lowpasslen6); break;
  default:
    DBGPRINTF("sampling ratio not supported...\n");
  }
  Decimator_ConfigureFactor(r->d, r->smp_ratio);

  if (r->s != NULL) {
    DBGPRINTF("starting sound stream...\n");
    err = SoundIO_Start(r->s);
    if(err != 0) {
      DBGPRINTF( "could not start SoundIO\n");
      return err;
    };
    
    r->stopped = 0;
  }
  countInitialSamples = 0;
  outputInitialSamples = 1;

  return 0;
}

int Recognizer_Stop(Recognizer *r) {

  if(r->stopped) return 0;
  if (r->s) {
    if(SoundIO_Stop(r->s)!=0) { /* frees what SoundIO_Start allocated */
      DBGPRINTF( "could not stop SoundIO\n");
      return -1;
    };
    if(SoundIO_Close(r->s)!=0) { /* frees what SoundIO_Open allocated */
      DBGPRINTF( "could not close SoundIO\n");
      return -1;
    };
  }
  /* free temporary buffers */
  //DBGPRINTF("deallocating temporary buffer\n");
  //if(r->asr_buffer != NULL) free(r->asr_buffer); r->asr_buffer = NULL;
  ///* the following also forces Recognizer_PushSpeech to reallocate when necessary */
  //r->asr_buffer_len = 0; 

  /* eventually consume last frames */
  while((r->features = FeatureExtraction_PopFeatures(r->fe)) != NULL) {
    /* copy data to simulator (should change to memcpy) */
    DBGPRINTF("consuming last frames...\n");
    int i;
    for (i = 0; i < r->lg->inputsize; i++) {
      r->lg->sim->input_stream[0][i] = r->features[i];
    }
    free(r->features);

    if(LikelihoodGen_ConsumeFrame(r->lg)) {
      Recognizer_ConsumeFrame(r);
    }
  }

  /* check that this function does all is necessary */
  FeatureExtraction_Deactivate(r->fe);
  if(r->vd != NULL) ViterbiDecoder_Reset(r->vd);

  r->stopped = 1;

  return 0;
}

int MaximumAPosteriori(float *app, int size) {
  float max;
  int res, i;//, size;
  //  float *app = r->lg->lh; /* vector of a posteriori probs */

  //size = r->lg->outputStreamSizes[0];
  /* find winning phone */
  max = app[0]; res = 0;
  for(i=1;i<size;i++) {
    if(app[i]>max) {
      max = app[i];
      res = i;
    }
  }

  return res;
}

int ResultQueue_Push(ResultQueue *rq, int idx, int frame_time, double playback_time) {

  if(rq->acc == MAXQUEUELEN) return -1;
  rq->data[rq->in].idx = idx;
  rq->data[rq->in].frame_time = frame_time;
  rq->data[rq->in].playback_time = playback_time;
  //rq->data[rq->in].calibprob = calibprob;
  rq->in++; if(rq->in == MAXQUEUELEN) rq->in = 0;
  rq->acc++;
  //  rq->len++;

  return 1;
}

/* returns 1 if there is a result to return or 0 otherwise */
int Recognizer_GetResult(Recognizer *r, int *idx, int *frame_time, double *playback_time) {
  ResultQueue *rq = r->rq;

  if(rq->acc == 0) return 0;
  //res = (Result *) malloc(sizeof(Result));
  *idx = rq->data[rq->out].idx;
  *frame_time = rq->data[rq->out].frame_time;
  *playback_time = rq->data[rq->out].playback_time;
  //res->calibprob = rq->data[rq->out].calibprob;
  rq->out++; if(rq->out == MAXQUEUELEN) rq->out = 0;
  rq->acc--;

  return 1;
}

void Recognizer_SetDebug(Recognizer *r, int level) {
  if(r != NULL) r->debug = level;
  if(r->s != NULL) SoundIO_SetDebug(r->s,level);
  if(r->lg != NULL) LikelihoodGen_SetDebug(r->lg, level);
  if(r->vd != NULL) ViterbiDecoder_SetDebug(r->vd, level);
}

void Recognizer_SetCallback(Recognizer *r, RecognizerCallbackProc *proc) {
  r->callback = proc;
}
