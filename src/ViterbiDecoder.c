/***************************************************************************
                         ViterbiDecoder.c  -  description
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

#include "ViterbiDecoder.h"
#include "Common.h"

/* utilities */
int **AllocPsi(int nStates, int backTrackLen);
int FreePsi(int **psi, int nStates);

/* this is limited because we need to know the size of
   the recognition network to allocate the decoder data.
   this is done in InitializeViterbiDecoder */
ViterbiDecoder *ViterbiDecoder_Create() {
  ViterbiDecoder *vd;

  vd = (ViterbiDecoder *) malloc(sizeof(ViterbiDecoder));

  /* set defaults, just to be picky */
  vd->debug = 0;
  vd->hasPrior = 0;
  vd->nStates = 0;
  vd->maxDelay = 0;
  vd->backTrackLen = vd->maxDelay+1;
  vd->relT = 0;
  vd->delta = NULL;
  vd->preDelta = NULL;
  vd->stPrior = NULL;
  vd->transmat = NULL;
  vd->phPrior = NULL;
  vd->fisStateId = NULL;
  vd->psi = NULL;
  vd->tempPath = NULL;
  vd->obslik = NULL;
  vd->frameLen = 0;
  /* to be consistent with backTrackLen = 1 at this point */
  ViterbiDecoder_SetLookahead(vd,vd->maxDelay);
  /* these are needed for off-line proc. Move up to tcl */
  // vd->obslik = NULL;
  // vd->obspost = NULL;
  // vd->outpath = NULL;
  // vd->entropy = NULL;
  // vd->sumact = NULL;


  return(vd);
}

void ViterbiDecoder_SetFrameLen(ViterbiDecoder *vd, int frameLen) {
  if(frameLen > 0) {
    vd->frameLen = frameLen;
    if(vd->obslik != NULL) free(vd->obslik);
    vd->obslik = (float *) malloc(frameLen*sizeof(float));
  }
}

int ViterbiDecoder_Free(ViterbiDecoder **vdptr) {
  ViterbiDecoder *vd = *vdptr;
  //  int i=0;
  
  if(vd==NULL) return 0;

  /* free decoder variables and data */
  ViterbiDecoder_FreeGrammar(vd);
  /* free data dependent variables */
  if(vd->obslik != NULL) free(vd->obslik);
  /*  if(r->obslik != NULL) FreeMatrix(r->obslik); fix this */
  /* remove output files eventually remained and free out */
  /* finally free ViterbiDecoder */
  free(vd);
  *vdptr = NULL;

  return 0;
}

/* Comes from Rec.c (tclvit) TEST! */
/* frees also psi and tempPath */
int ViterbiDecoder_FreeGrammar(ViterbiDecoder *vd) {
  //int i;

  if(vd == NULL) return 0;

  DBGPRINTF("freing vd->transmat if necessary...\n");
  SparseMatrix_Free(&vd->transmat);

  DBGPRINTF("freing vd->stPrior if necessary...\n");
  Vector_Free(&vd->stPrior);

  DBGPRINTF("freing vd->fisStateId if necessary...\n");
  IntVector_Free(&vd->fisStateId);

  if(vd->delta != NULL) {
    DBGPRINTF("vd->delta not NULL, freing...\n");
    free(vd->delta); vd->delta = NULL;
  }
  if(vd->preDelta != NULL) {
    DBGPRINTF("vd->preDelta not NULL, freing...\n");
    free(vd->preDelta); vd->preDelta = NULL;
  }
  if(vd->psi != NULL) {
    DBGPRINTF("vd->psi not NULL, freing...\n");
    FreePsi(vd->psi,vd->nStates); vd->psi = NULL;
  }
  //DBGPRINTF("free vd->psi\n");
  //if(vd->psi != NULL) FreePsi(vd->psi,vd->nStates);
			//  for(i=0;i<vd->nStates;i++) 
			//if(vd->psi[i]!=NULL) free(vd->psi[i]);
			//free(vd->psi);
  if(vd->tempPath != NULL) {
    DBGPRINTF("vd->tempPath not NULL, freing...\n");
    free(vd->tempPath); vd->tempPath = NULL;
  }
  vd->nStates = 0;

  DBGPRINTF("return\n");
  return 0;
}

int ViterbiDecoder_CreateAndSetGrammar(ViterbiDecoder *vd, int nPhones, int statesPerPhone) {
  /* statesPerPhone + one state to connect to the other phones */
  int statesPerModel = statesPerPhone+1;
  int nStates = nPhones * statesPerModel;
  /* temporary variables for transitions */
  int nTrans, *from, *to, *kind;
  LogFloat *weight;
   /* current phone, state and transition number */
  int ph, st, tr, j;
  LogFloat phoneTransWeight = -log(1.0/nPhones);

  /* left-to-right model for each phone + connecting transitions */
  nTrans = nPhones * statesPerPhone * 2 + nPhones*nPhones;

  DBGPRINTF("freing previous grammar...\n");
  ViterbiDecoder_FreeGrammar(vd);

  DBGPRINTF("creating state prior...\n");
  vd->stPrior = Vector_Create(nStates);
  for(ph=0; ph<nPhones; ph++) {
    fprintf(stderr, "[%d]", ph); fflush(stdout);
    vd->stPrior->data[ph*statesPerModel] = phoneTransWeight;
  }
  fprintf(stderr, "\n"); fflush(stdout);

  DBGPRINTF("creating state to phone mapping...\n");
  vd->fisStateId = IntVector_Create(nStates);
  for(st=0; st<nStates; st++) {
    fprintf(stderr, "[%d]", st); fflush(stdout);
    vd->fisStateId->data[st] = st/statesPerModel;
  }
  fprintf(stderr, "\n"); fflush(stdout);
  
  DBGPRINTF("creating transitions...\n");
  from = (int *) malloc(nTrans*sizeof(int));
  to = (int *) malloc(nTrans*sizeof(int));
  kind = (int *) malloc(nTrans*sizeof(int));
  weight = (LogFloat *) malloc(nTrans*sizeof(LogFloat));
  
  DBGPRINTF("computing transitions...\n");
  st=0; tr=0;
  for(ph=0; ph<nPhones; ph++) {
    for(j=0; j<statesPerPhone; j++) {
      /* self transition */
      from[tr] = st;
      to[tr] = st;
      kind[tr] = PHONE_TRANSITION;
      weight[tr] = SELF_TRANSITION_WEIGHT;
      tr++;
      /* next transition */
      from[tr] = st;
      to[tr] = st+1;
      kind[tr] = PHONE_TRANSITION;
      weight[tr] = NEXT_TRANSITION_WEIGHT;
      tr++; st++;
    }
    /* "grammar" transitions */
    for(j=0; j<nPhones; j++) {
      from[tr] = st;
      to[tr] = j*statesPerModel;
      kind[tr] = GRAMMAR_TRANSITION;
      weight[tr] = phoneTransWeight;
      tr++;
    }
    st++;
    fprintf(stderr, "[%d]", tr); fflush(stdout);
  }
  fprintf(stderr, "\n"); fflush(stdout);
  
  DBGPRINTF("creating new grammar structures...\n");
  vd->transmat = SparseMatrix_CreateWithData(from, to, weight, kind, nTrans);
  vd->nStates = nStates;

  /* free temporary structures */
  DBGPRINTF("freing temporary structures...\n");
  free(from);
  free(to);
  free(weight);
  free(kind);

  /* viterbi variables */
  DBGPRINTF("creating Viterbi temp variables\n");
  vd->delta = (LogFloat *) malloc(vd->nStates * sizeof(LogFloat));
  vd->preDelta = (LogFloat *) malloc(vd->nStates * sizeof(LogFloat));
  /* allocates and sets psi and tempPath to 0 */
  ViterbiDecoder_SetLookahead(vd,vd->maxDelay);
  /* this forces computing prior (first file t=0) */
  vd->hasPrior = 0;

  return 0;
}

/* Comes from Rec.c (tclvit) TEST! */
int ViterbiDecoder_SetGrammar(ViterbiDecoder *vd, SparseMatrix *transmat,
			   Vector *stPrior, IntVector *fisStateId) {

  ViterbiDecoder_FreeGrammar(vd);

  vd->transmat = transmat;
  //DisplaySparseMatrix(transmat);
  vd->stPrior = stPrior;
  vd->fisStateId = fisStateId;
  vd->nStates = vd->transmat->ncols;
  /* viterbi variables */
  vd->delta = (LogFloat *) malloc(vd->nStates * sizeof(LogFloat));
  vd->preDelta = (LogFloat *) malloc(vd->nStates * sizeof(LogFloat));
  /* allocates and sets psi and tempPath to 0 */
  ViterbiDecoder_SetLookahead(vd,vd->maxDelay);
  /* this forces computing prior (first file t=0) */
  vd->hasPrior = 0;

  return 0;
}

int ViterbiDecoder_SetLookahead(ViterbiDecoder *vd, int lookahead) {
  //  int i;

  if(vd == NULL || lookahead < 0) return 0;

  DBGPRINTF("vd->nStates=%d, vd->backTrackLen=%d, lookahead=%d\n", vd->nStates, vd->backTrackLen, lookahead);
  vd->maxDelay = lookahead;
  vd->backTrackLen = lookahead+1;
  if(vd->psi != NULL) {
    DBGPRINTF("vd->psi not NULL, freing\n");
    FreePsi(vd->psi,vd->nStates); vd->psi=NULL;
  }
  if(vd->tempPath != NULL) {
    DBGPRINTF("vd->tempPath not NULL, freing\n");
    free(vd->tempPath); vd->tempPath = NULL;
  }
  if(vd->nStates != 0) {
    DBGPRINTF("before malloc vd->psi (nStates = %d, backTrackLen = %d)\n",vd->nStates, vd->backTrackLen);
    vd->psi = AllocPsi(vd->nStates,vd->backTrackLen);
  }
  DBGPRINTF("before malloc vd->tempPath\n");
  vd->tempPath = (int *) malloc(vd->backTrackLen*sizeof(int));
  memset(vd->tempPath,0,vd->backTrackLen*sizeof(int));

  DBGPRINTF("finished\n");
  return 0;
}

/* clears tempPath and psi */
int ViterbiDecoder_Reset(ViterbiDecoder *vd) {
  int i; //,j;
  //LogFloat *preDelta = vd->preDelta;
  //  Vector   *stPrior    = rec->stPrior;
  //int       nStates   = vd->nStates;

  vd->relT = 0;
  vd->totMaxDelta = NINF;

  /* this forces computing prior (first file t=0) */
  vd->hasPrior = 0;

  /* sets psi amd tempPath to 0, needed for first file */
  memset(vd->tempPath,0,vd->backTrackLen*sizeof(int));
  //  for(j=0;j<vd->backTrackLen;j++) {
    //vd->tempPath[j] = 0;
  for(i=0;i<vd->nStates;i++)
    memset(vd->psi[i],0,vd->backTrackLen*sizeof(int));
    //  vd->psi[i][j] = 0;

  /* this should not be needed, but just to be sure...
     make sure that 0 is a good initialization */
  //memset(vd->preDelta, 0, vd->nStates*sizeof(float));
  //memset(vd->delta, 0, vd->nStates*sizeof(float));

  return 1;
}

int BestDeltaIdx(LogFloat *delta, int nStates) {
  int i,maxidx=-1;
  LogFloat max=NINF;

  for(i=0;i<nStates;i++)
    if(isinf(delta[i]) != -1 && delta[i]>max) {
      max = delta[i];
      maxidx = i;
    }

  //if(maxidx==-1) DBGPRINTF("all NINF delta!!!\n");

  return maxidx;
}

#if 0
/* backtrack backTrackLen times */
/* makes use of circular arrays psi and tempPath */
int BackTrack(ViterbiDecoder *vd) {
  int relT = vd->relT;
  int t, prevIdx;
  int relBt,relBtp1; /* please change names !! */
  int backTrackLen = vd->backTrackLen;
  int maxDelay = vd->maxDelay;
/*int checkLen = 0;*/

  vd->tempPath[relT] = BestDeltaIdx(vd->delta, vd->nStates);
  for(t=relT-1;t>relT-maxDelay;t--) {
/*checkLen++;*/
    relBt = Mod(t,backTrackLen); relBtp1 = Mod(t+1,backTrackLen);
    prevIdx = vd->psi[vd->tempPath[relBtp1]][relBtp1];
    if(prevIdx<0) Error("negative index","");
    if(vd->tempPath[relBt] == prevIdx) break; /* the rest of the path is the same */
    vd->tempPath[relBt] = prevIdx;
  }
/*  DBGPRINTF("actual length: %d\n",checkLen); */
/* for checking !!!
for(t=relT;t>relT-maxDelay;t--)
  DBGPRINTF("%d ",r->tempPath[Mod(t,backTrackLen)]);
DBGPRINTF("\n");
 end for checking !!!*/
  return vd->tempPath[Mod(relT-maxDelay,backTrackLen)];
}
#endif

/* backtrack backTrackLen times */
/* makes use of circular arrays psi and tempPath */
int BackTrack(ViterbiDecoder *vd) {
  int relT = vd->relT;
  int t, prevIdx;
  int trelprev,trel; /* please change names !! */
  int backTrackLen = vd->backTrackLen;
  int maxDelay = vd->maxDelay;

  for(t=relT;t>relT-maxDelay;t--) {
    trel = Mod(t,backTrackLen);
    trelprev = Mod(t-1,backTrackLen);
    if(vd->tempPath[trel]<0 || vd->tempPath[trel]>=vd->nStates) {
      DBGPRINTF( "panic! vd->tempPath[%d] = %d [%d,%d], "
		 "setting to 0\n", trel, vd->tempPath[trel], 0, vd->nStates);
      vd->tempPath[trel] = 0;
    }
    prevIdx = vd->psi[vd->tempPath[trel]][trel];
    if(prevIdx<0 || prevIdx>=vd->nStates) {
      DBGPRINTF("panic! prevIdx=%d [%d,%d], setting to 0\n",
	      prevIdx, 0, vd->nStates);
      prevIdx=0;
    }
    /* stop backtrack because the rest of the path is the same */
    if(vd->tempPath[trelprev] == prevIdx) break;
    vd->tempPath[trelprev] = prevIdx;
  }
  return vd->tempPath[Mod(relT-maxDelay,backTrackLen)];
}


/* -------------- function implementation -------------- */

/* make sure that fisStateId is in vd instead of vd->out */
/* and that frames go through all the steps as in viterbi */
/* this takes a frame vector with linear likelihoods
   (the row output of the ANN scaled by the prior probs).
   If necessary it takes the log. Note that this
   modifies the input vector (but it shouldn't be a problem) */
int vd_debugcount=0;
int ViterbiDecoder_ConsumeFrame(ViterbiDecoder *vd, float *frame, int framelen) {
  int i;
  int fromidx, from, to, maxfrom;
  float obs, maxval, val;
  float *tmp; /* for delta pointer switching */
  float *obslik = vd->obslik;
  int res;
  int allnull = 1;

  if(framelen != vd->frameLen)
    DBGPRINTF("panic! framelen != vd->frameLen, (%d,%d)\n", framelen, vd->frameLen);


  /* this is to check that not all the elements in the frame are 0.0 */
  for(i=0;i<framelen;i++)
    if(frame[i]!=0.0) {
      allnull = 0;
      break;
    }
  /* in case set one element to 1.0 */
  if(allnull) {
    DBGPRINTF("warning! all features are zero, setting 0th to 1.0\n");
    frame[0] = 1.0;
  }

  /* in case normalize and take the log of the obs vector */
  //if(vd->frameIsLinPostProb)
  for(i=0;i<framelen;i++) {
    obslik[i] = log(frame[i]); /* log takes care of the infinite cases */
    //if(obslik[j]==NINF) { DBGPRINTF("-inf "); }
    //else DBGPRINTF("%f ",obslik[j]);
  }
  //DBGPRINTF("\n");

  /* first time use prior to define the first
     psi (not extremely important in real-time app) */
  if(!vd->hasPrior) { /* prior evaluation */
    for(to=0;to<vd->nStates;to++) {
      obs = obslik[vd->fisStateId->data[to]];
      //DBGPRINTF("stPrior[%d] = %f, obs = %f\n", i,vd->stPrior->data[to],obs);
      //if(isinf(vd->stPrior->data[to]) == -1 || isinf(obs) == -1)
      //vd->preDelta[to] = NINF;
      //else vd->preDelta[to] = vd->stPrior->data[to] + obs;
      vd->preDelta[to] = vd->stPrior->data[to] + obs; // + works with infinity
      /* r->psi[i][t] = -1; I leave it to 0 (trick to handle first file) */
    }
    //DBGPRINTF("prior: obs =");
    //for(i=0;i<vd->nStates;i++) {
    //  DBGPRINTF(" %f",obslik[vd->fisStateId->data[i]]);
    //}
    //DBGPRINTF("\n");
    //DBGPRINTF("prior: preDelta =");
    //for(i=0;i<vd->nStates;i++) {
    //  DBGPRINTF(" %f",vd->preDelta[i]);
    //}
    //DBGPRINTF("\n");

    vd->hasPrior = 1;
    return 0; /* fix this possibly to sil (?) */
  }

  /* Viterbi step */
  /* update relative time */
  vd->relT++; vd->relT = Mod(vd->relT,vd->backTrackLen);
  for(to=0;to<vd->nStates;to++) {
    maxval = NINF; maxfrom = -1; /* could be anything */
    obs = obslik[vd->fisStateId->data[to]];
    if(isinf(obs) == -1) {
      /* depth[t]--; */
      vd->delta[to] = NINF; vd->psi[to][vd->relT] = maxfrom;
      continue; }
    else {
      for(fromidx=0;fromidx<vd->transmat->nels[to];fromidx++) {
	from = vd->transmat->idxs[to][fromidx];
	if(isinf(vd->preDelta[from]) == -1) continue;
	else {
	  val = vd->preDelta[from]+vd->transmat->data[to][fromidx];
	  if(val>maxval) { maxval=val; maxfrom = from; }
	}
      }
      if(isinf(maxval) == -1) vd->delta[to] = NINF;
      else {
	vd->delta[to] = maxval + obs;
	if(maxval>vd->totMaxDelta) vd->totMaxDelta = maxval;
      }
      vd->psi[to][vd->relT] = maxfrom;
    }
  }
  //DBGPRINTF("obs =");
  //for(i=0;i<vd->nStates;i++) {
  //  DBGPRINTF(" %f",obslik[vd->fisStateId->data[i]]);
  //}
  //DBGPRINTF("\n");
  //DBGPRINTF("preDelta");
  //for(i=0;i<vd->nStates;i++) {
  //if(vd->delta[j] == NINF) DBGPRINTF(" Ninf");
  //else DBGPRINTF(" %f",vd->delta[j]);
  //  if(vd->preDelta[i] == NINF) DBGPRINTF(" Ninf");
  //  else DBGPRINTF( " %f", vd->preDelta[i]);
  //}
  //DBGPRINTF("\n");
  /* update temporary path */
  vd->tempPath[vd->relT] = BestDeltaIdx(vd->delta, vd->nStates);
  
  /* this is a hack to avoid the case (that should never happen)
     when all deltas are NINF and the resulting indez is -1 */
  if(vd->tempPath[vd->relT]<0) vd->tempPath[vd->relT] = 0;
  //DBGPRINTF("tempPath before backtrack:");
  //for(i=0;i<vd->backTrackLen;i++)
  //  if(i==vd->relT)	DBGPRINTF(" <%d>",vd->tempPath[i]);
  //  else	DBGPRINTF(" %d",vd->tempPath[i]);
  //DBGPRINTF("\n");
  //DBGPRINTF("psi before backtrack:\n");
  //for(i=0;i<vd->nStates;i++) {
  //  int ii;
  //  for(ii=0;ii<vd->backTrackLen;ii++) {
  //	DBGPRINTF(" %d", vd->psi[i][ii]);
  //  }
  //  DBGPRINTF("\n");
  //}
  /* backtrack maxDelay times */
  /* check if it returns network or fisical states */
  res = BackTrack(vd);
  /*   if(check<vd->nStates && check>=0) */
  /*     /\* get rid of this part *\/ */
  /*     vd->out->list->path[vd->out->list->framesDone++] = check; */
  /*   else { */
  /*     DBGPRINTF("abs t = %d, relT = %d, idx = %d\n",t,r->relT,check); */
  /*     Error("path idx out of range",""); */
  /*   } */
  /*   if(r->out->list->framesDone == r->out->list->nFrames) { */
  /*     PrintResults(r->out); */
  /*     r->out->list = RemFromOutput(r->out->list); */
  /*   } */
  /* switch pointers */
  tmp = vd->delta; vd->delta = vd->preDelta; vd->preDelta = tmp;
  
  
  /* check machine precision */
  /*DBGPRINTF("maximum so far: %.7f\n",totMax);*/
  /* this one is to keep it running ad libitum and avoid
     overflow errors. On the other hand it's gona take a very
     long time befor this is a problem */
  if(0) { /* vd->totMaxDelta > ...) */ 
    for(i=0;i<vd->nStates;i++)
      if(isinf(vd->preDelta[i])!=-1)
	vd->preDelta[i] -= vd->totMaxDelta;
    vd->totMaxDelta = NINF;
  }
  
  return vd->fisStateId->data[res];
}

int **AllocPsi(int nStates, int backTrackLen) {
  int **psi, i;

  psi = (int **) malloc (nStates * sizeof(int *));
  DBGPRINTF("before malloc vd->psi[i]\n");
  for(i=0;i<nStates;i++) {
    //DBGPRINTF("[%i] ml ",i);
    psi[i] = (int *) malloc(backTrackLen*sizeof(int));
    //DBGPRINTF("ms ");
    memset(psi[i],0,backTrackLen*sizeof(int));
  }
  DBGPRINTF2("\n");
  return psi;
}

int FreePsi(int **psi, int nStates) {
  int i;

  if(psi == NULL) return 0;

  DBGPRINTF("before free vd->psi[i]\n");
  for(i=0;i<nStates;i++) {
    //DBGPRINTF(" [%d]",i);
    if(psi[i]!=NULL) {
      free(psi[i]); psi[i] = NULL;
    }
  }
  DBGPRINTF2("\n");
  DBGPRINTF("before free psi\n");
  free(psi);
  DBGPRINTF("exiting\n");

  return 0;
}

void ViterbiDecoder_SetDebug(ViterbiDecoder *v, int level) {
  if(v != NULL) v->debug = level;
}

void ViterbiDecoder_DumpState(ViterbiDecoder *v) {
  FILE *fh;
  int i,j;

  if(v == NULL) return;
  fh = fopen("vd-statedump.txt", "w");
  fprintf(fh,"Dumping data for the current viterbi state\n");
  /* this is to check why it gets stuck */
  int currentposition = v->tempPath[v->relT];
  int el;
  fprintf(fh, "We are in state: %d, possible precessors:\n", currentposition);
  if(v->transmat->nels[currentposition] == 0) {
    fprintf(fh, "Panic!!! There's no way we could have got here!!\n");
  } else {
    for(el=0; el<v->transmat->nels[currentposition]; el++) {
      fprintf(fh, " %d", v->transmat->idxs[currentposition][el]);
    }
    fprintf(fh, "\n");
  }
/*   float min_delta=INF; */
/*   float max_delta=NINF; */
/*   float avg_delta=0.0; */
/*   for(j=0;j<vd->nStates;j++) { */
/*     if(vd->delta[j]<min_delta) min_delta=vd->delta[j]*1.1; */
/*     if(vd->delta[j]>max_delta) max_delta=vd->delta[j]; */
/*     avg_delta+=vd->delta[j]; */
/*   } */
/*   avg_delta/=vd->nStates; */
/*   fprintf(stderr, "delta: min=%f, mean=%f, max=%f\n", min_delta, avg_delta, max_delta); */

  fprintf(fh,"(*) simple dumps of circular arrays, the current position\n");
  fprintf(fh,"    is defined by the relative time index.\n");
  fprintf(fh,"Relative time index: %d\n", v->relT);
  fprintf(fh,"Current path buffer (*):\n");
  for(i=0; i<v->backTrackLen; i++) fprintf(fh, " %d", v->tempPath[i]);
  fprintf(fh,"\nCurrent delta values:\n");
  for(i=0; i<v->nStates; i++)
    if(isinf(v->delta[i]) != -1) fprintf(fh, " %f", v->delta[i]);
    else fprintf(fh, " -Inf");
  fprintf(fh, "\nPrevious delta values:\n");
  for(i=0; i<v->nStates; i++)
    if(isinf(v->preDelta[i]) != -1) fprintf(fh, " %f", v->preDelta[i]);
    else fprintf(fh, " -Inf");
  fprintf(fh,"\nCurrent psi (*):\n");
  for(i=0; i<v->nStates; i++) {
    for(j=0; j<v->backTrackLen; j++) fprintf(fh, " %d", v->psi[i][j]);
    fprintf(fh, "\n");
  }
  fprintf(fh, "dump finished, have a nice day\n");
  fclose(fh);
}

void ViterbiDecoder_DumpModel(ViterbiDecoder *v) {
  FILE *fh;

  if(v == NULL) return;
  fh = fopen("vd-modeldump.txt", "w");
  fprintf(fh,"Dumping model data for the viterbi decoder\n");
  fprintf(fh,"Frame length: %d\n", v->frameLen);
  fprintf(fh,"Number of states: %d\n", v->nStates);
  fprintf(fh,"Maximum delay: %d\n", v->maxDelay);
  fprintf(fh,"Backtracking length: %d\n", v->backTrackLen);
  fprintf(fh,"Vector of phonetic priors:\n");
  DisplayVector (fh, v->phPrior);
  fprintf(fh,"Vector of state priors:\n");
  DisplayVector (fh, v->stPrior);
  fprintf(fh,"Mapping from states to phonemes (positions in the feature vectors):\n");
  DisplayIntVector (fh, v->fisStateId);
  fprintf(fh,"Transition matrix (sparse):\n");
  DisplaySparseMatrix(fh, v->transmat);
  fprintf(fh, "dump finished, have a nice day\n");
  fclose(fh);
}
