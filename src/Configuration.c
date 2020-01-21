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
#include <libgen.h>

/* this is used to run a switch on strings */
const unsigned long hash(const char *str) {
    unsigned long hash = 5381;  
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


/* there is a simpler function in LikelihoodGen.h, but this is the way it's done from Tcl 
int ReadANNAsInTCL(Recognizer *r, char *filename) {
  FILE *f;
  unsigned char *buffer;
  int n;
  BinaryBuffer *binbuf;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "cannot open file %s", filename);
    return -1;
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
}*/

/* expecting three filenames separated by commas without spaces */
int ReadGrammar(Recognizer *r, char *filenames, float gramfact, float insweight) {
  FILE *f;
  int fn_idx;
  char transmat_fn[MAX_PATH_LEN], prior_fn[MAX_PATH_LEN], map_fn[MAX_PATH_LEN], *fn_p[3], *src_p;
  int n = 0;
  int from[4000], to[4000], type[4000];
  float weight[4000];
  SparseMatrix *transmat;
  Vector *stPrior;
  IntVector *fisStateId;

  /* first copy path name */
  fn_p[0] = transmat_fn;
  fn_p[1] = prior_fn;
  fn_p[2] = map_fn;
  for(n=0; n<strlen(r->configDirName); n++) {
    *fn_p[0]++ = r->configDirName[n];
    *fn_p[1]++ = r->configDirName[n];
    *fn_p[2]++ = r->configDirName[n];
  }
  *fn_p[0]++ = '\0';
  *fn_p[1]++ = '\0';
  *fn_p[2]++ = '\0';

  /* now parse input parameter */
  src_p = filenames;
  fn_idx = 0;
  while(*src_p) {
      if(*src_p==',') {
	*fn_p[fn_idx] = '\0';
	src_p++;
	fn_idx++;
	if(fn_idx>2) return -1; /* too many commas */
      }
      *fn_p[fn_idx] = *src_p;
      src_p++;
      fn_p[fn_idx]++;
  }
  *fn_p[fn_idx] = '\0';
  if(fn_idx!=2) return -1; /* too few commas */
 
  /* transition matrix */
  f = fopen(transmat_fn, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", transmat_fn);
    return -1;
  }
  while(fscanf(f, "%d %d %f %d\n", &from[n], &to[n], &weight[n], &type[n]) != EOF) {
    from[n]--; to[n]--; // convert from matlab to C indexes
    // here check Inf values (but there are none in the files I have checked)
    weight[n] *= -1;
    /* This is now done by ViterbiDecoder_Activate
    if(type[n]) // apply grammar factor and insertion weight
      weight[n] = weight[n] * gramfact + insweight;
    */
    n++;
  }
  fclose(f);
  transmat = SparseMatrix_CreateWithData(from, to, weight, type, n);

  /* state prior */
  n=0;
  f = fopen(prior_fn, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", prior_fn);
    return -1;
  }
  while(fscanf(f, "%f\n", &weight[n]) != EOF) {
    from[n] *= -1; // this should work with the inf valuse as well
    n++;
  }
  fclose(f);
  stPrior = Vector_CrearteWithData(weight,n);

  /* fis state id */
  n=0;
  f = fopen(map_fn, "r");
  if (!f) {
    fprintf(stderr, "cannot open file %s\n", map_fn);
    return -1;
  }
  while(fscanf(f, "%d\n", &from[n]) != EOF) {
    from[n]--; // convert from matlab to C indexes
    n++;
  }
  fclose(f);
  fisStateId = IntVector_CreateWithData(from,n);

  ViterbiDecoder_SetGrammar(r->vd,transmat,stPrior,fisStateId);
  return 0;
}

/* assuming MAX_PATH_LEN is allocated for dest */
int joinPath(char *dest, const char *dirname, const char* filename) {
  int dirlen = strlen(dirname);
  int filelen = strlen(filename);
  int destlen = dirlen+filelen+1;

  if(destlen>=MAX_PATH_LEN)
    return -1;

  strcpy(dest, dirname);
  dest[dirlen] = '/'; // this should be portable across platforms
  for(int i=0; i<filelen; i++)
    dest[dirlen+1+i] = filename[i];
  dest[destlen] = '\0';
  
  return dirlen+1+filelen;
}

int Configuration_ApplyOption(Recognizer *r, char *option, char *value) {
  char filename[MAX_PATH_LEN];

  switch(hash(option)) {
    /* General parameters */
      case CONF_LG_ANN: /* -lg:ann */
    if(joinPath(filename, r->configDirName, value) != 0)
      return -1;
    LikelihoodGen_LoadANNFromFilename(r->lg, filename);
    Recognizer_GetOutSym(r);
    break;
      case CONF_LG_PHPRIOR: /* -lg:phprior */
    if(joinPath(filename, r->configDirName, value) != 0)
      return -1;
    Vector *pp = Vector_LoadFromFilename(filename);
    LikelihoodGen_SetPhPrior(r->lg,pp);
    break;
      case CONF_DECODER: /* -decoder */
    // fill this
    break;
    /* Feature Extraction parameters */
      case CONF_FE_INPUTRATE: /* -fe:inputrate */
    r->fe->inputrate = atoi(value);
    break;
      case CONF_FE_NUMFILTERS: /* -fe:numfilters */
    r->fe->numfilters = atoi(value);
    break;
      case CONF_FE_NUMCEPS: /* -fe:numceps */
    r->fe->numceps = atoi(value);
    break;
      case CONF_FE_LOWFREQ: /* -fe:lowfreq */
    r->fe->lowfreq = atof(value);
    break;
      case CONF_FE_HIFREQ: /* -fe:hifreq */
    r->fe->hifreq = atof(value);
    break;
      case CONF_FE_FRAMESTEP: /* -fe:framestep */
    r->fe->framestep = atof(value);
    break;
      case CONF_FE_FRAMELEN: /* -fe:framelen */
    r->fe->framelen = atof(value);
    break;
      case CONF_FE_PREEMPH: /* -fe:preemph */
    r->fe->preemph = atof(value);
    break;
      case CONF_FE_LIFTER: /* -fe:lifter */
    r->fe->lifter = atof(value);
    break;
    /* Viterbi decoder parameters */
      case CONF_VT_HMM: /* -vt:hmm */
    ReadGrammar(r, value, 1, 0);
    break;
      case CONF_VT_LOOKAHEAD: /* -vt:lookahead */
    Recognizer_SetLookahead(r, atoi(value));
    break;
      case CONF_VT_GRAMFACT: /* -vt:gramfact */
    r->vd->gramfact = atof(value);
    break;
      case CONF_VT_INSWEIGHT: /* -vt:insweight */
    r->vd->insweight = atof(value);
    break;
  default:
    return -1;
      }
  return 0;
}

int Configuration_ApplyConfigFromFilename(Recognizer *r, char *filename) {
  char tmpname[MAX_PATH_LEN];
  FILE *fp = NULL;
  char option[1024], value[1024];

  strcpy(tmpname, filename); /* make a copy because dirname modifies it */
  strcpy(r->configDirName, dirname(tmpname));
  
  DBGPRINTF("applyng configuration in %s\n", filename);

  fp = fopen(filename, "r");
  if (fp == NULL) {
      fprintf(stderr, "could not open file: %s\n", filename);
      return -1;
  }
  
  while(fscanf(fp, "%s %s\n", option, value) == 2) {
      Configuration_ApplyOption(r, option, value);
  }

  fclose(fp);
  
  return 0;
}
