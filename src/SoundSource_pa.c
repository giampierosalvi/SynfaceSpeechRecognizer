/***************************************************************************
                          SoundSource_pa.c  -  description
                             -------------------
    begin                : Fri Jan 10 2003
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

#include <stdlib.h>
#include "Common.h"
#include "SoundSource.h"
//#include "Feedback.h"
//#include "LMS.h"
#include "nrg.h"
#include <stdio.h>
#include <fcntl.h>
#ifndef _MSC_VER
#  include <unistd.h>
#endif

#include <limits.h>

#ifdef HAH_COMPATIBILITY
# define SMPRATE 32000.0
#else
# define SMPRATE 48000.0
#endif

// This is a very crytical parameter. If too small, portaudio will fail
#define AUDIO_BUFFER_LEN_SEC 0.02

#define NCHANNELS 1

// enough for 2 seconds at 48kHz
#define RINGBUFFER_LENGTH 96000

/*
  To avoid confusion with terminology, here is some clarification:
  A 'frame' is:
  - portaudio: one audio sample (including several channels in case)
  - recognizer: a number of speech samples that are used to compute the
    spectral based coefficients at a certain time step.

  In this source file we take the portaudio meaning of frame, in other
  C files we may take the recognizer meaning.
   */

int debugcount = 0;

void ShortMinMax(short *min, short *max, short *buffer, int len) {
  int i;

  *min = SHRT_MAX;
  *max = SHRT_MIN;
  for(i=0; i<len; i++) {
    if(buffer[i]<*min) *min = buffer[i];
    if(buffer[i]>*max) *max = buffer[i];
  }
}

int paSoundSourceCallback( const void *inputBuffer, void *outputBuffer,
			   unsigned long framesPerBuffer,
			   const PaStreamCallbackTimeInfo* timeInfo,
			   PaStreamCallbackFlags statusFlags,
			   void *userData ) {
  SoundSource* s;

  s = (SoundSource*)userData;
  s->outtime = timeInfo->outputBufferDacTime;

  if( inputBuffer == NULL || s->stopped) return 0;

  ShortMinMax(&s->audio_sample_min, &s->audio_sample_max, inputBuffer, framesPerBuffer);

  /* compute energy */
  nrg_GetEnergy(s->nrgY, (short*) inputBuffer, framesPerBuffer);

  RingBuffer_Push(s->audiorate_buffer, (short*) inputBuffer,
		  framesPerBuffer*s->inputParameters.channelCount);

  RingBuffer_Pull(s->audiorate_buffer, (short*) outputBuffer,
		  framesPerBuffer*s->inputParameters.channelCount);

  if(s->hasCallback)
    (s->callback)(s->callbackData, inputBuffer, framesPerBuffer);

  return 0;
}

SoundSource *SoundSource_Create() {
  SoundSource *s;
  int dev;
  const PaDeviceInfo *dev_info;

  /* This was done in the package audio (audio_initialize). It works on
     Linux but not on Windows, no clue why. So we do it here as well. Note
     that it is no problem to initialize portaudio several time, as long as
     we terminate it also as many times*/
  DBGPRINTF("running Pa_Initialize (Windows compatibility)\n");
  Pa_Initialize();

  DBGPRINTF("before malloc\n");
  s = (SoundSource *) malloc(sizeof(SoundSource));
  DBGPRINTF("after malloc\n");

  s->hasCallback = 0;

  /* set default and static in/output parameters */
  dev = Pa_GetDefaultInputDevice();
  dev_info = Pa_GetDeviceInfo(dev);
  s->inputParameters.device = dev;
  if(dev_info != NULL)
    s->inputParameters.suggestedLatency = dev_info->defaultLowInputLatency;
  else {
    s->inputParameters.suggestedLatency = 0.0;
    DBGPRINTF("no device info, setting latency to 0.0\n");
  }
  s->inputParameters.channelCount = NCHANNELS;
  s->inputParameters.hostApiSpecificStreamInfo = NULL;
  dev = Pa_GetDefaultOutputDevice();
  dev_info = Pa_GetDeviceInfo(dev);
  s->outputParameters.device = dev;
  if(dev_info != NULL)
    s->outputParameters.suggestedLatency = dev_info->defaultLowOutputLatency;
  else {
    s->outputParameters.suggestedLatency = 0.0;
    DBGPRINTF("no device info, setting latency to 0.0\n");
  }
  s->outputParameters.channelCount = NCHANNELS;
  s->outputParameters.hostApiSpecificStreamInfo = NULL;

  /* here modifying global variables... */
  s->stopped = 1;
  s->mute_flag = 0;

  /* this is the energy level estimators */
  DBGPRINTF("create nrg structs\n");
  s->nrgY = nrg_Create(NCHANNELS,0,.999); // nchannels is fixed here!!
  if(NCHANNELS==2) s->nrgX = nrg_Create(NCHANNELS,1,.999);
  else s->nrgX = NULL;
  s->simplex_flag = 0;
  s->simplex_threshold = .02;

  s->samplingRate = SMPRATE;
  //s->framesPerBuffer = s->samplingRate/100; /* 10ms */
  //s->snd_buf_len = s->framesPerBuffer * NCHANNELS;
  //s->snd_buf_len = 100; /* dummy value, adjusted dynamically */
  //s->snd_buf = NULL;
  //s->snd_buf_mono = NULL;

  /* this is created even if playback_flag is 0 because is used to
     execute getoutpos */
  //DBGPRINTF("creating feedback struct...\n");
  //DBGPRINTF("...with buffer length = %ld bytes\n",
  //	    s->snd_buf_len*sizeof(short));
  //s->fb = Feedback_Create(s->snd_buf_len*sizeof(short));
  s->playback_flag = 0;
  s->playback_delay_sec = 0.2;
  /* check that it's safe to run Feedback_SetDelay several times */
  //SoundSource_SetFBDelay(s,s->playback_delay_sec);

  /* note that the filter is optimised for 48kHz -> 8kHz conversion */
  //s->d = Decimator_Create();

  s->audiorate_buffer = RingBuffer_Create(RINGBUFFER_LENGTH);
  //s->asrrate_buffer = RingBuffer_Create(RINGBUFFER_LENGTH);

  return s;
}

/* some of the freing is done in SoundSource_Close for consistency
   with SoundSource_Open */
int SoundSource_Free(SoundSource **sptr) {
  SoundSource *s = *sptr;

  if(s == NULL) return 0;

  /* changed from close to stop (close does only stop)*/
  DBGPRINTF("stopping...\n");
  SoundSource_Stop(s);
  DBGPRINTF("stopping done.\n");
  DBGPRINTF("closing...\n");
  SoundSource_Close(s);
  DBGPRINTF("closing done.\n");

  if(s->nrgX != NULL) {
    DBGPRINTF("s->nrgX is not NULL, freing...\n");
    nrg_Delete(s->nrgX);
    s->nrgX = NULL;
  }
  if(s->nrgY != NULL) {
    DBGPRINTF("s->nrgY is not NULL, freing...\n");
    nrg_Delete(s->nrgY);
    s->nrgY = NULL;
  }

  /* This was done in the package audio (audio_terminate). It works on
     Linux but not on Windows, no clue why. So we do it here as well. Note
     that it is not a problem to initialize portaudio several time, as long as
     we terminate it as many times */
  DBGPRINTF("running Pa_Terminate (Windows compatibility)\n");
  Pa_Terminate();

  DBGPRINTF("freing s->audiorate_buffer if necessary\n");
  RingBuffer_Free(&s->audiorate_buffer);

  DBGPRINTF("now freing object...\n");
  free(s);
  *sptr = NULL;

  return 0;
}

/* some of the allocation is done here because the smpRate is
   needed. Not nice, but to keep consistent with the OSS version */
int SoundSource_Open(SoundSource *s, char *device, int smpRate) {
  PaError err;
  const PaStreamInfo *stream_info;
  const PaDeviceInfo *dev_info;
  const PaStreamParameters *ip, *op; /* pointers to input and output param*/

  if(s->inputParameters.device < 0 && s->outputParameters.device < 0) {
    DBGPRINTF("input and output devices can not be both disabled (%d, %d)\n", s->inputParameters.device, s->outputParameters.device);
    return -1;
  }

  DBGPRINTF("using portaudio with sample rate=%d\n", s->samplingRate);

  s->inputParameters.sampleFormat = paInt16;  /* 16 bit integer (short) */
  s->outputParameters.sampleFormat = paInt16;  /* 16 bit integer (short) */

  if(s->inputParameters.device < 0) {
    ip = NULL;
    DBGPRINTF("inputParameters = NULL\n");
  } else {
    ip = &s->inputParameters;
    DBGPRINTF("inputParameters.device=%d\n", s->inputParameters.device);
    if((dev_info = Pa_GetDeviceInfo(s->inputParameters.device)) != NULL) {
      DBGPRINTF("input device name=%s\n", dev_info->name);
    }      
    DBGPRINTF("inputParameters.channelCount=%d\n", s->inputParameters.channelCount);
    DBGPRINTF("inputParameters.sampleFormat=%ld\n", s->inputParameters.sampleFormat);
    DBGPRINTF("inputParameters.suggestedLatency=%f\n", s->inputParameters.suggestedLatency);
  }

  if(s->outputParameters.device < 0) {
    op = NULL;
    DBGPRINTF("outputParameters = NULL\n");
  } else {
    op = &s->outputParameters;
    DBGPRINTF("outputParameters.device=%d\n", s->outputParameters.device);
    if((dev_info = Pa_GetDeviceInfo(s->outputParameters.device)) != NULL) {
      DBGPRINTF("output device name=%s\n", dev_info->name);
    }      
    DBGPRINTF("outputParameters.channelCount=%d\n", s->outputParameters.channelCount);
    DBGPRINTF("outputParameters.sampleFormat=%ld\n", s->outputParameters.sampleFormat);
    DBGPRINTF("outputParameters.suggestedLatency=%f\n", s->outputParameters.suggestedLatency);
  }

  if ((err = Pa_IsFormatSupported(ip, op, (double) s->samplingRate))) {
    DBGPRINTF("Device cannot be opened with current parameters because:\n");
    DBGPRINTF("%s\n", Pa_GetErrorText(err));
    return err;
  }

  err = Pa_OpenStream(
		      &s->pa_stream,
		      ip,
		      op,
		      (double) s->samplingRate,
		      (int) s->samplingRate * AUDIO_BUFFER_LEN_SEC,//paFramesPerBufferUnspecified,
		      //s->framesPerBuffer,  /* frames per buffer */
		      paClipOff, /* we won't output out of range samples so don't bother clipping them */
		      paSoundSourceCallback,
		      (void *) s );
  if(err != paNoError) {
    DBGPRINTF("could not open portaudio stream because:\n");
    DBGPRINTF("%s\n", Pa_GetErrorText(err));
    return err;
  }
  s->starttime = Pa_GetStreamTime(s->pa_stream);
  DBGPRINTF("stream time reported at startup:: %f\n", s->starttime);
  stream_info = Pa_GetStreamInfo(s->pa_stream);
  DBGPRINTF("streamInfo.structVersion: %d\n", stream_info->structVersion);
  DBGPRINTF("streamInfo.inputLatency: %f\n", stream_info->inputLatency);
  DBGPRINTF("streamInfo.outputLatency: %f\n", stream_info->outputLatency);
  DBGPRINTF("streamInfo.sampleRate: %f\n", stream_info->sampleRate);

  //  printf("SoundSource_Open: configuration:\n");
  //printf(" pa buffer length:        %d\n", s->snd_buf_len);

  return 0;
}

int SoundSource_Start(SoundSource *s) {
  PaError err;

  if(!s->stopped) return 0;

  DBGPRINTF("starting...\n");
  //s->inPosMs = 0;
  //s->outPosMs = 0; // from Feedback

  DBGPRINTF("resetting streamtime struct\n");
  s->streamtime.old_smp = 0.0;
  s->streamtime.old_ms = 0;
  s->streamtime.rest_ms = 0.0;

  /* these are for debugging */
  s->audio_sample_min = SHRT_MAX;
  s->audio_sample_max = SHRT_MIN;

  DBGPRINTF("Pa_StartStream\n");
  err = Pa_StartStream(s->pa_stream);
  if(err != paNoError) {
    DBGPRINTF("could not start stream because:\n");
    DBGPRINTF("%s\n", Pa_GetErrorText(err));
    return err;
  }

  s->stopped = 0;

  DBGPRINTF("done.\n");

  return 0;
}

int SoundSource_Stop(SoundSource *s) {
  PaError err;
  //int i;

  if (s->stopped == 1) return 0;
  /* I'd like this to be in the end, but it doesn't work. I suppose that
     SoundSource_Stop is called more than once (for example through
     SoundSource_Free), from different threads, and the second call hungs
     because it's not aware of the first. Check, check, check!*/
  s->stopped = 1;

  DBGPRINTF("stopping...\n");

  err = Pa_StopStream(s->pa_stream);
  if(err != paNoError) {
    DBGPRINTF("could not stop portaudio stream\n");
    return err;
  }

  //Feedback_Reset(s->fb);
  RingBuffer_Reset(s->audiorate_buffer);
  //RingBuffer_Reset(s->asrrate_buffer);

  return 0;
}

int SoundSource_Close(SoundSource *s) {
  PaError err;
  //int i,res;

  SoundSource_Stop(s);

  DBGPRINTF("closing portaudio stream...\n");
  if(Pa_IsStreamActive(s->pa_stream) == paNoError) { /* valid stream */
    err = Pa_CloseStream(s->pa_stream);
    if(err != paNoError) {
      DBGPRINTF("could not close portaudio stream\n");
      return err;
    }
  }

  //Sleep(10000);
  //Pa_Terminate();

  /* temporary buffers for audio callback */
  //DBGPRINTF("deallocating temporary buffers\n");
  //if(s->snd_buf != NULL) free(s->snd_buf); s->snd_buf = NULL;
  //if(s->snd_buf_mono != NULL) free(s->snd_buf_mono); s->snd_buf_mono = NULL;

  DBGPRINTF("closing...\n");

  return 0;
}

int SoundSource_SetCallback(SoundSource *s, SoundSourceCallbackProc *proc, void *data) {
  s->hasCallback = 1;
  s->callback = proc;
  s->callbackData = data;

  return 0;
}

/* set the feedback delay in seconds */
void SoundSource_SetPlaybackDelay(SoundSource *s, double delay_sec) {
  int delay_samples = (int)
    (delay*s->samplingRate*s->inputParameters.channelCount);

  if(!s->stopped) return; // safeguard

  DBGPRINTF("delay in seconds: %f\n",delay);
  s->playback_delay_sec = delay;
  //Feedback_SetDelay(s->fb,delay_samples*sizeof(short));
  RingBuffer_SetDelay(s->audiorate_buffer, delay_samples);
}

double SoundSource_GetPlaybackDelay(SoundSource *s) {
  if(s!=NULL) return(s->playback_delay_sec);
  return -1.0;
}

int SoundSource_SetSamplingRate(SoundSource *s, int audio_rate) {
  s->samplingRate = audio_rate;
  DBGPRINTF("new rate = %d\n", audio_rate);
  //s->framesPerBuffer = s->samplingRate/100; /* 10ms */
  //DBGPRINTF("framesPerBuffer = %d\n", s->framesPerBuffer);

  return 0;
}

int SoundSource_GetInPos(SoundSource *s) {
  return 0;
}

int SoundSource_GetOutPos(SoundSource *s) { // from Feedback
  return 0;
}

double SoundSource_GetOutTime(SoundSource *s) { // from Feedback
  return(s->outtime);
}

/* v19 version returning seconds (double) */
double SoundSource_GetStreamTime(SoundSource *s) {
  double t;
  if(s->stopped) return 0.0;
  if(s->pa_stream == NULL) return 0.0;
  t = Pa_GetStreamTime(s->pa_stream);
  //  DBGPRINTF("stream time reported: %f\n", t);
  return(t - s->starttime);
}

int SoundSource_IsMuted(SoundSource *s) {
  return s->mute_flag;
}

void SoundSource_SetDebug(SoundSource *s, int level) {
  if(s != NULL) s->debug = level;
}
 
int SoundSource_SetInDevice(SoundSource *s, int idx) {
  int ndevs = Pa_GetDeviceCount();

  if(0 <= idx && idx < ndevs) {
    s->inputParameters.device = idx;
    DBGPRINTF("indev changed to %d\n",idx);
    return 0;
  }
  DBGPRINTF("indev out of range (%d, %d)\n",idx,ndevs);
  return -1;
}

int SoundSource_SetOutDevice(SoundSource *s, int idx) {
  int ndevs = Pa_GetDeviceCount();

  if(idx < ndevs) {
    s->outputParameters.device = idx;
    DBGPRINTF("outdev changed to %d\n",idx);
    return 0;
  }
  DBGPRINTF("outdev out of range (%d, %d)\n",idx,ndevs);
  return -1;
}

int SoundSource_GetInDevice(SoundSource *s) {
  return s->inputParameters.device;
}

int SoundSource_GetOutDevice(SoundSource *s) {
  return s->outputParameters.device;
}

//double SoundSource_GetBufLenSec(SoundSource *s) {
//  return (double) s->snd_buf_len/(s->inputParameters.channelCount * s->samplingRate);
//}
