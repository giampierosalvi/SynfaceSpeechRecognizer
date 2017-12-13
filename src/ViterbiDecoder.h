/***************************************************************************
                         ViterbiDecoder.h  -  description
                             -------------------
    begin                : Tue June 3 2003
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

#ifndef _VITERBIDECODER_H_
#define _VITERBIDECODER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "RecMath.h"
//#include "Common.h"

/* kind of weights */
#define TRANSITION 0
#define GRAMMAR 1

/* ViterbiDecoder */
typedef struct {
  /* decoder variables and data */
  int debug;
  short           hasPrior;     /* forces computing prior for the first time */
  int   frameLen; /* it's the same as the length of phPrior */
  short frameIsLinPostProb; /* wether the incoming frames are linear */
  //int           nFrames;
  //int           sizeObs; /* CHECK that it's OK to remove it */
  int           nTrans;
  int           maxDelay;
  int           backTrackLen; /* always maxDelay + 1 */
  int           relT;         /* this is used in the circular arrays */
  LogFloat      totMaxDelta;  /* to reset the value of delta now and then */
  LogFloat     *delta;        /* [nTrans] */
  LogFloat     *preDelta;     /* [nTrans] */
  Vector       *stPrior;      /* [nTrans] */
  SparseMatrix *transmat;     /* [nTrans][variable] */
  Vector       *phPrior;      /* [sizeObs] */
  IntVector    *fisStateId;    /* */
  int         **psi;          /* [nTrans][backTrackLen] circular array */
  int          *tempPath;     /* [backTrackLen] circular array */
  /* Grammar scales */
  // float grammarFactor; /* sums logaritmically to the grammar kind weights */
  /* data dependent variables */
  //Matrix       *obslik;       /* [nFrames][sizeObs] */
  /* defines how output should be porcessed */
  //Output       *out;
  float *obslik; /* [frameLen] holds a frame of observation probabilities */
} ViterbiDecoder;

/* allocate memory just for the structure */
ViterbiDecoder *ViterbiDecoder_Create();
			
/* reset deltas and partial path */
int ViterbiDecoder_Reset(ViterbiDecoder *vd);

/* frees everything */
int ViterbiDecoder_Free(ViterbiDecoder **vdptr);

/* this is just for some rescaling and log taking */
/* not used anymore */
//int FixFormats(ViterbiDecoder *vd);

/* consumes one frame */
int ViterbiDecoder_ConsumeFrame(ViterbiDecoder *vd, float *frame, int framelen);

/* new functions from Rec.c (tclvit) */
int ViterbiDecoder_FreeGrammar(ViterbiDecoder *vd);
int ViterbiDecoder_SetGrammar(ViterbiDecoder *vd, SparseMatrix *transmat,
			      Vector *stPrior, IntVector *fisStateId);
int ViterbiDecoder_SetLookahead(ViterbiDecoder *vd, int lookahead);
void ViterbiDecoder_SetFrameLen(ViterbiDecoder *vd, int frameLen);

void ViterbiDecoder_SetDebug(ViterbiDecoder *v, int level);

/* functions for debugging */
void ViterbiDecoder_DumpModel(ViterbiDecoder *v);
void ViterbiDecoder_DumpState(ViterbiDecoder *v);

#endif /* _VITERBIDECODER_H_ */

