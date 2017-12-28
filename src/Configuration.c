/***************************************************************************
                         Configuration.c  -  description
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

#include "Configuration.h"
#include "SoundIO.h"
#include "SoundProc.h"
#include "FeatureExtraction.h"
#include "LikelihoodGen.h"
#include "ViterbiDecoder.h"

/* there is a simpler function in LikelihoodGen.h, but this is the way it's done from Tcl */
int ReadANNAsInTCL(Recognizer *r, char *filename) {
  FILE *f;
  unsigned char *buffer;
  int n;
  BinaryBuffer *binbuf;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    error();
  }
  buffer = (unsigned char *) malloc(MAX_FILE_SIZE*sizeof(unsigned char));
  n = fread(buffer, 1, MAX_FILE_SIZE, f);
  fclose(f);
  binbuf = BinaryBuffer_Create((char *) buffer, n);
  free(buffer);
  LikelihoodGen_LoadANNFromBuffer(r->lg, binbuf);
  BinaryBuffer_Free(binbuf);
  Recognizer_GetOutSym(r);

  return 0;
}

int ReadANN(Recognizer *r, char *filename) {
  FILE *f;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    error();
  }
  LikelihoodGen_LoadANNFromFile(r->lg, f);
  fclose(f);
  Recognizer_GetOutSym(r);

  return 0;
}

int ReadPhPrior(Recognizer *r, char *filename) {
  FILE *f;
  float tmp[200]; // priors are usually around of length 50
  int n = 0;
  Vector *pp;

  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    error();
  }
  while(fscanf(f, "%f\n", &tmp[n]) != EOF) n++;
  fclose(f);

  pp = CreateVector(tmp,n);
  LikelihoodGen_SetPhPrior(r->lg,pp);
  return 0;
}

int ReadGrammar(Recognizer *r, char *filebasename, float gramfact, float insweight) {
  FILE *f;
  char filename[512];
  int n = 0;
  int from[4000], to[4000], type[4000];
  float weight[4000];
  SparseMatrix *transmat;
  Vector *stPrior;
  IntVector *fisStateId;

  /* transition matrix */
  sprintf(filename, "%s.transmat", filebasename);
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%d %d %f %d\n", &from[n], &to[n], &weight[n], &type[n]) != EOF) {
    from[n]--; to[n]--; // convert from matlab to C indexes
    // here check Inf values (but there are none in the files I have checked)
    weight[n] *= -1;
    if(type[n]) // apply grammar factor and insertion weight
      weight[n] = weight[n] * gramfact + insweight;
    n++;
  }
  fclose(f);
  transmat = CreateSparseMatrix(from, to, weight, type, n);

  /* state prior */
  sprintf(filename, "%s.prior", filebasename);
  n=0;
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%f\n", &weight[n]) != EOF) {
    from[n] *= -1; // this should work with the inf valuse as well
    n++;
  }
  fclose(f);
  stPrior = CreateVector(weight,n);

  /* fis state id */
  sprintf(filename, "%s.map", filebasename);
  n=0;
  f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", filename);
    error();
  }
  while(fscanf(f, "%d\n", &from[n]) != EOF) {
    from[n]--; // convert from matlab to C indexes
    n++;
  }
  fclose(f);
  fisStateId = CreateIntVector(from,n);

  ViterbiDecoder_SetGrammar(r->vd,transmat,stPrior,fisStateId);
  return 0;
}

/* Warning! There is no checking on the values!!! */
int Configuration_ApplyOption(char *option, char *value, Recognizer *r) {
  switch(option) {
    /* General parameters */
  case "-ann":
    ReadANN(r, value);
    break;
  case "-phprior":
    ReadPhPrior(r, value);
    break;
  case "-decoder":
    // fill this
    break;
    /* Feature Extraction parameters */
  case "-fe:inputrate":
    r->fe->inputrate = atoi(value);
    break;
  case "-fe:numfilters":
    r->fe->numfilters = atoi(value);
    break;
  case "-fe:numceps":
    r->fe->numceps = atoi(value);
    break;
  case "-fe:lowfreq":
    r->fe->lowfreq = atof(value);
    break;
  case "-fe:hifreq":
    r->fe->hifreq = atof(value);
    break;
  case "-fe:framestep":
    r->fe->framestep = atof(value);
    break;
  case "-fe:framelen":
    r->fe->framelen = atof(value);
    break;
  case "-fe:preemph":
    r->fe->preemph = atof(value);
    break;
  case "-fe:lifter":
    r->fe->lifter = atof(value);
    break;
    /* Viterbi decoder parameters */
  case "-vt:grambasename":
    ReadGrammar(r, value, 1, 0);
    break;
  case "-vt:lookahead":
    Recognizer_SetLookahead(r, atoi(value));
    break;
  case "-vt:gramfact":
    break;
  case "-vt:insweight":
    break;
  default:
    return -1
      }
}

int joinPath(char *dest, const char *dirname, const char* filename) {
  int dirlen = strlen(dirname);
  int filelen = strlen(filename);

  strcpy(dest, dirname);
  dest[dirlen] = '/'; // this should be portable across platforms
  for(i=0;i<filelen, i++)
    dest[dirlen+1+i] = filename[i];
  // check null symbol at the end
  
  return dirlen+1+filelen;
}

int Configuration_ApplyConfig(char *dirname, Recognizer *r) {
  char *tmpname, nameptr;
  int dirlen = strlen(dirname);
  int i;
  
  DBGPRINTF("applyng configuration in %s\n", dirname);
  /* allocate enough memory for file names */
  tmpname = (char *) malloc(dirlen+100);

  joinPath(tmpname, dirname, PARAMETERS_FN);

  joinPath(tmpname, dirname, RNN_FN);

  joinPath(tmpname, dirname, PHONE_PRIOR_FN);

  joinPath(tmpname, dirname, HMM_MAP_FN);

  joinPath(tmpname, dirname, HMM_PRIOR_FN);

  joinPath(tmpname, dirname, HMM_TRANSMAT_FN);

  free(tmpname);
}

#endif /* _CONFIGURATION_H_ */
