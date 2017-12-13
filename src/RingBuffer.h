/***************************************************************************
                         RingBuffer.h  -  description
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

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

typedef struct RingBuffer {
  int length;
  int in_idx;
  int out_idx;
  short *buffer;
} RingBuffer;

/* framesize in bytes */
RingBuffer* RingBuffer_Create(int length);
int RingBuffer_Free(RingBuffer **rbptr);
void RingBuffer_Push(RingBuffer *rb, short* data, int size);
void RingBuffer_Pull(RingBuffer *rb, short* dest, int size);
void RingBuffer_Reset(RingBuffer *rb);
int RingBuffer_GetDelay(RingBuffer *rb);
void RingBuffer_SetDelay(RingBuffer *rb, int delay);

#endif /* _RINGBUFFER_H_ */
