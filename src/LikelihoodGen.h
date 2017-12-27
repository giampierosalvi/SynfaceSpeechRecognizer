/***************************************************************************
                          LikelihoodGen.h  -  description
                             -------------------
    begin                : Tue Jan 7 2003
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

#ifndef _LIKELIHOODGEN_H_
#define _LIKELIHOODGEN_H_

#include "RecMath.h"
#include "RTSim.h"


/* Callback from LikelihoodGen to Recognizer */
//typedef int (LikelihoodGenCallbackProc)(void *cbData);

/* Likelihood generator: computes P(x|o) where x is a class
   and o the observation */
typedef struct {
  int debug;
  int         stopped;     /* stop generation */
  int         inputsize;
  int         outputsize;
  float *lh;               /* vector of ann output activities */
  float       *pp;         /* vector of posteriors */
  Vector      *phPrior;    /* vector of class priors for likelihood/posterior conversion */
  //  Net *net;                /* temporary net */
  RTSimulator *sim;        /* real time impl. of ann */
  int numOutputStreams;
  int *outputStreamSizes;
} LikelihoodGen;

LikelihoodGen *LikelihoodGen_Create();

int LikelihoodGen_Free(LikelihoodGen **gptr);
int LikelihoodGen_LGLoadANN(LikelihoodGen *g, BinaryBuffer *buf);

/* consumes a frame and returns the likelihood vector in g->lh,
   waits if no speech avaliable jet, and returns -1 if end of speech */
int LikelihoodGen_ConsumeFrame(LikelihoodGen *g);

int LikelihoodGen_SetPhPrior(LikelihoodGen *g, Vector *phPrior);
int LikelihoodGen_CheckData(LikelihoodGen *g);

/* returns size of the (only) output stream */
int LikelihoodGen_GetOutSize(LikelihoodGen *g);
int LikelihoodGen_GetInSize(LikelihoodGen *g);

void LikelihoodGen_SetDebug(LikelihoodGen *g, int level);

/* these are used for starting and stopping. */
int LikelihoodGen_Activate(LikelihoodGen *g);

#endif /* _LIKELIHOODGEN_H_ */
