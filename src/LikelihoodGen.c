/***************************************************************************
                          LikelihoodGen.c  -  description
                             -------------------
    begin                : Sat Dec 21 2002
    copyright            : (C) 2002-2017 by Giampiero Salvi
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

/*
  gcc -L/afs/tmh.kth.se/tmh/misc/hacks/pkg/nico/1.2.0/lib/os -lnico -lm -I/afs/tmh.kth.se/tmh/misc/hacks/pkg/nico/1.2.0/include -o PostProbGen PostProbGen.c
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* just for debugging */
#include "Common.h"

//#include <System.h>
#include "LikelihoodGen.h"
#include <stdio.h>
#include <time.h> // check speed

int MaxIdx(float *vec, int size);

LikelihoodGen *LikelihoodGen_Create(int smpRate) {
  LikelihoodGen *g;
  g = (LikelihoodGen *) malloc(sizeof(LikelihoodGen));

  /* allocated later */
  g->inputsize = 0;  /* this refers to the ann */
  g->outputsize = 0; /* this refers to the ann */
  //g->features = NULL;
  g->lh = NULL;
  g->pp = NULL;
  g->phPrior = NULL;
  g->sim = NULL;
  g->numOutputStreams = 0;
  g->outputStreamSizes = NULL;

  return g;
}


int LikelihoodGen_Free(LikelihoodGen **gptr) {
  LikelihoodGen *g = *gptr;

  if(g->sim!=NULL) {
    DBGPRINTF("freing RTSim...\n");
    FreeNetWork(g->sim->net); /* this is not done in FreeRTSim!!! */
    FreeRTSim(g->sim);
  }
  //DBGPRINTF("freing outputStreamSizes...\n");
  //free(g->outputStreamSizes); /* check that it's needed */
  if(g->lh!=NULL) free(g->lh);
  if(g->pp!=NULL) free(g->pp);
  if(g->phPrior!=NULL) FreeVector(&g->phPrior);
  DBGPRINTF("freing LG strtucture...\n");
  free(g);
  *gptr = NULL;

  return 0;
}

/* TODO: this shouldn't access g->numOutputStreams, g->outputStreamSizes */
int LGLoadANN(LikelihoodGen *g, BinaryBuffer *buf) {
  int i;
  Net *net;

  if(g->sim!=NULL) {
    DBGPRINTF("g->sim is not NULL, freing\n");
    if(g->sim->net!=NULL) {
      DBGPRINTF("g->sim->net is not NULL, freing\n");
      FreeNetWork(g->sim->net); /* this is not done in FreeRTSim!!! */
    }
    FreeRTSim(g->sim);
  }
  if(g->lh!=NULL) free(g->lh);
  if(g->pp!=NULL) free(g->pp);

  DBGPRINTF("before ReadNet\n");
  /* change this to ReadNet() and get FILE *stream from arguments */
  net = ParseNet(buf);

  DBGPRINTF("before CompileRTSim\n");
  g->sim = CompileRTSim(net);     /* in RTSim.c */
  DBGPRINTF("number of inputs streams: %d\n",
	  g->sim->num_input_streams);
  for(i=0;i<g->sim->num_input_streams;i++)
    DBGPRINTF("   input stream %d: size: %d\n",
	    i,g->sim->input_stream_size[i]);
  RTSimOutputSize(g->sim, &(g->numOutputStreams), &(g->outputStreamSizes));
  DBGPRINTF("number of outputs streams: %d\n",
	  g->numOutputStreams);
  for(i=0;i<g->numOutputStreams;i++)
    DBGPRINTF("  output stream %d: size: %d\n",
	    i,g->outputStreamSizes[i]);
  /* output_component_name should hold the phone names */

  /* now assume sinlge input and output stream */
  g->inputsize = g->sim->input_stream_size[0];
  g->outputsize = g->sim->output_stream_size[0];
  /* allocate holders */
  g->lh = malloc(g->outputsize*sizeof(float));
  g->pp = malloc(g->outputsize*sizeof(float));

  DBGPRINTF("before FreeNet\n");
  FreeNet(net);

  return 0;
}

float pLogP(float p) {
  return (p>0.000000000001) ? (p*log(p)) : 0.0;
}

int LikelihoodGen_ConsumeFrame(LikelihoodGen *g) {
  //int res; /* store the results */
  float **out;
  int i;
  //float max, mean, entropy, sumDebug;
  /* to print out mfccs (control) */

  //if(g->stopped) return 0;
    
	//  (g->callback->proc)(g->callback->data,
	//		g->sim->output_component_name[0][res]);
  //return g->sim->output_component_name[0][g->prevRes];

  RTSimInput(g->sim); /* costly */
  if(RTSimOutput(g->sim)==0) { /* costless */
    out = GetRTSimOutputVectors(g->sim);

  //sumDebug = 0.0;
  //for(i=0;i<g->sim->output_stream_size[0];i++) sumDebug += out[0][i];
  //printf("LGConsumeFrame: adding a vector to recognizer, sumDebug = %f\n",sumDebug);

  /* this is the output vector for the first and only stream */
    memcpy(g->pp, out[0], g->outputsize*sizeof(float));
    for (i = 0; i < g->outputsize; i++) {
      g->lh[i] = g->pp[i]/g->phPrior->data[i];
    }
    return 1;
  }
  return 0;
}

/* these are used for starting and stopping, perhaps not complete */
int LikelihoodGen_Activate(LikelihoodGen *g) {
  RTSimInputRestart(g->sim);

  return 0;
}

int ResetLikelihoodGen(LikelihoodGen *g) {
  /* SpeechEnd is called by SoundSource */
  // SpeechEnd(g->speetures); /* nothing more to push */
  /* this is built in StartRecLoop (it will consume remaining frames) */
  //while((PopFeatures(g->speetures)) != NULL) { }

  //RTSimInputEnd(g->sim); /* nothing more to push */
  // should I use this? RTSimInitialize(RTSimulator *work, char *file_name) {
  return 0;
}

int MaxIdx(float *vec, int size) {
  int i;
  float max=vec[0];
  int maxi=0;

  for(i=1;i<size;i++)
    if(vec[i]>max) {
      max = vec[i];
      maxi = i;
    }

  return maxi;
}

int LikelihoodGen_SetPhPrior(LikelihoodGen *g, Vector *phPrior) {

  if(g==NULL) return -1;
  if(phPrior==NULL) return -1;
  if(g->phPrior != NULL) FreeVector(&g->phPrior);
  g->phPrior = phPrior;

  return 0;
}

/* This checks if ann and phPrior have been properly loaded */
/* this has been substituted by the check in rec::start */
int LikelihoodGen_CheckData(LikelihoodGen *g) {

  if(g->sim==NULL) {
    DBGPRINTF("ann missing\n");
    return 1;
  }
  if(g->phPrior==NULL) {
    DBGPRINTF("phPrior missing\n");
    return 1;
  }
  if(g->phPrior->nels!=g->outputsize) {
    DBGPRINTF("phPrior wrong length\n");
    return 1;
  }

  return 0;
}

/* returns size of the (only) output stream */
int LikelihoodGen_GetOutSize(LikelihoodGen *g) {
  int res = 0;
  if(g!=NULL) res = g->outputsize;
  return res;
}

/* returns size of the (only) input stream */
int LikelihoodGen_GetInSize(LikelihoodGen *g) {
  int res = 0;
  if(g!=NULL) res = g->inputsize;
  return res;
}

void LikelihoodGen_SetDebug(LikelihoodGen *g, int level) {
  if(g != NULL) g->debug = level;
}

