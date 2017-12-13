/***************************************************************************
                         RingBuffer.c  -  description
                             -------------------
    begin                : Wed Oct 9 2013
    copyright            : (C) 2013-2017 by Giampiero Salvi
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

#include "RingBuffer.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// debug
#include "Common.h"

/* framesize in bytes */
RingBuffer* RingBuffer_Create(int length) {
  RingBuffer *rb = malloc(sizeof(RingBuffer));

  rb->length = length;
  rb->in_idx = 0;
  rb->out_idx = 0;
  rb->buffer = (short *) malloc(length*sizeof(short));

  return rb;
}

int RingBuffer_Free(RingBuffer **rbptr) {
  RingBuffer *rb = *rbptr;

  if(rb == NULL) return 0;

  if(rb->buffer != NULL) free(rb->buffer);
  free(rb);
  *rbptr = NULL;

  return 0;
}

void RingBuffer_Reset(RingBuffer *rb) {
  memset(rb->buffer, 0, rb->length*sizeof(short));
  rb->in_idx = 0;
  rb->out_idx = 0;
}


void RingBuffer_Push(RingBuffer* rb, short* data, int size) {
  assert(size<=rb->length);
  // make sure in and out don't cross
  //assert((size+rb->in_idx)%rb->length <= rb->out_idx);

  if(rb->in_idx + size < rb->length) { // simple case
    memcpy(&rb->buffer[rb->in_idx], data, size*sizeof(short));
    rb->in_idx += size;
  } else { // we are across the boundary, split into two memcpy
    int size1 = size - (rb->length - rb->in_idx);
    int size2 = size-size1;
    memcpy(&rb->buffer[rb->in_idx], data, size1*sizeof(short));
    memcpy(&rb->buffer[0], &data[size1], size2*sizeof(short));
    rb->in_idx = size2;
  }
}

void RingBuffer_Pull(RingBuffer* rb, short* dest, int size) {
  assert(size<=rb->length);
  // make sure in and out don't cross
  //assert((size+rb->out_idx)%rb->length <= rb->in_idx);

  if(rb->out_idx + size < rb->length) { // simple case
    memcpy(dest, &rb->buffer[rb->out_idx], size*sizeof(short));
    rb->out_idx += size;
  } else { // we are across the boundary, split into two memcpy
    int size1 = size - (rb->length - rb->out_idx);
    int size2 = size-size1;
    memcpy(dest, &rb->buffer[rb->out_idx], size1*sizeof(short));
    memcpy(&dest[size1], &rb->buffer[0], size2*sizeof(short));
    rb->out_idx = size2;
  }
}

int RingBuffer_GetDelay(RingBuffer *rb) {
  int delay;

  if(rb->in_idx >= rb->out_idx)
    delay = rb->in_idx - rb->out_idx;
  else
    delay = rb->length - rb->out_idx + rb->in_idx;

  return delay;
}

void RingBuffer_SetDelay(RingBuffer *rb, int delay) {
  int current_delay;

  assert(delay <= rb->length);

  current_delay = RingBuffer_GetDelay(rb);

  DBGPRINTF("current delay = %d desired delay = %d\n", current_delay, delay);

  if(current_delay == delay) {  // nothing needs to be done
    return;
  }

  if(current_delay > delay) { // pull and throw away some samples
    int buflen = current_delay - delay;
    short *buf = (short*) malloc(buflen*sizeof(short));
    RingBuffer_Pull(rb, buf, buflen);
    free(buf);
  } else { // move out_idx backward
    int displace = delay - current_delay;
    if(displace <= rb->out_idx) { // simple case
      rb->out_idx -= displace;
    } else {
      rb->out_idx = rb->length - displace + rb->out_idx;
    }
  }
}
