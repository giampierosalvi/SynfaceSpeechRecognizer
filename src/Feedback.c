/***************************************************************************
                         Feedback.c  -  description
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

#include <stdlib.h>
#include <stdio.h>
#include "Feedback.h"
#include "Common.h"
#define XBUFFERS 64 /* extra output buffers */

void** allocBuffers(int nbuffers, int size) {
  int i;
  void **p;
  p = calloc(nbuffers,sizeof(void*));
  if (p!=NULL) {
    for (i=0;i<nbuffers;i++) {
      p[i] = malloc(size);
      if (p[i]==NULL) {
	return NULL;
      }
      memset(p[i], 0, size);
    }
  }
  return p;
}

void freeBuffers(void **p, int nbuffers) {
  int i;
  for (i=0;i<nbuffers;i++) {
    free(p[i]);
  }
  free(p);
}

Feedback* Feedback_Create(int framesize) {

  Feedback* f;

  DBGPRINTF("entering\n");

  f = (Feedback*)malloc(sizeof(Feedback));
  if (f == NULL) {
    DBGPRINTF("malloc failed");
    return NULL;
  }
  f->nsdelay   = 0;
  f->nbdelay   = 0;
  f->nbufs = 0;
  f->framesize = framesize;
  //f->framesizebytes = framesize*samplesize;
  f->bufsize = framesize;
  //f->bufsizebytes = framesize*samplesize;
  f->outbuf = 0;
  f->inbuf = 0;
  f->compensation = 0;
  f->bufs = NULL;
    
  return f;
}


/* set delay time (in bytes) */
void Feedback_SetDelay(Feedback* f,int delay) {

  DBGPRINTF("delay = %d\n", delay);
  if (f->bufs != NULL) {
    freeBuffers(f->bufs,f->nbufs);
    // free(f->waveHdrOut); // to SoundSource
  }
  
  f->nbdelay = delay/f->bufsize;
  if (f->nbdelay < 1) f->nbdelay = 1;
  f->nbufs = f->nbdelay+XBUFFERS;
  DBGPRINTF("nbdelay=%d,nbufs=%d\n",
	  f->nbdelay,f->nbufs);
  f->bufs       = allocBuffers(f->nbufs,f->bufsize);

  if (f->bufs == NULL) {
    DBGPRINTF("failed to allocate buffers!\n");
  }
  f->inbuf  = f->nbdelay-1;
  f->outbuf = 0;
}

int Feedback_Free(Feedback *f) {

  if(f == NULL) return 0;
  if (f->bufs != NULL) {
    DBGPRINTF("freing buffers...\n");
    freeBuffers(f->bufs,f->nbufs);
  }

  DBGPRINTF("freing object...\n");
  free(f);

  return 0;
}

void* Feedback_ProcessBytes(Feedback* f, void* inbuf, int inbufsize) {
  void* res;

  if (inbufsize > f->bufsize) {
    DBGPRINTF("oops: inbufsize = %d, bufsizebytes = %d",
	    inbufsize, f->bufsize);
  }

  memcpy(f->bufs[f->inbuf],inbuf,inbufsize);
  f->inbuf++;
  f->inbuf = f->inbuf % f->nbufs;

  res = f->bufs[f->outbuf];

  f->outbuf++;
  f->outbuf = f->outbuf % f->nbufs;

  return res;
}

/* mute last n buffers */
void Feedback_Mute(Feedback* f, int n) {
  int i, buf;

  if (n >= f->nbdelay) {
    n = f->nbdelay - 1;
  }
  for (i= 0;i<n;i++) {
    buf = f->inbuf-i-1;
    if (buf < 0)
      buf = buf + f->nbufs;
    if (buf >= f->nbufs)
      buf = buf - f->nbufs;
    memset(f->bufs[buf],0,f->bufsize);
  }
}

void Feedback_Reset(Feedback* f) {
  f->inbuf  = f->nbdelay-1;
  f->outbuf = 0;
}
