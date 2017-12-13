/***************************************************************************
                         Feedback.h  -  description
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

#ifndef _FEEDBACK_H_
#define _FEEDBACK_H_

typedef struct Feedback {
  int nsdelay;  /* desired delay, in bytes */
  int nbdelay;  /* desired delay, in buffers */
  int nbufs;    /* number of allocated buffers */
  int framesize;/* frame size, in bytes (input) */
  //int framesizebytes;/* frame size, in bytes (input) */
  int bufsize;  /* buffer size in bytes (output) */
  //int bufsizebytes;  /* buffer size in bytes (output) */
  int inbuf;    /* index of current input buffer */
  int outbuf;   /* index of current output buffer */
  int compensation;
  void** bufs; /* output buffer array */
} Feedback;

/* framesize in bytes */
Feedback* Feedback_Create(int framesize);
int Feedback_Free(Feedback *f);
/* delay is in bytes */
void Feedback_SetDelay(Feedback* f,int delay);
/* inbuf size is in bytes */
void* Feedback_ProcessBytes(Feedback* f,void* inbuf,int inbufsize);
void Feedback_Mute(Feedback* f, int n); 
#endif /* _FEEDBACK_H_ */
