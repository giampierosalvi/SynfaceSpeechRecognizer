/***************************************************************************
                         rec.h  -  Tcl rec object
                             -------------------
    begin                : Tue Nov 10 2009
    copyright            : (C) 2009 by Giampiero Salvi
    email                : giampi@kth.se
 ***************************************************************************/

#ifndef _REC_H_
#define _REC_H_

/* To be wrapped into a tcl object: used to map tcl commands into rec objects */

#include "Recognizer.h"
#include "Common.h"

#include <string>
#include <vector>
using namespace std;

class Rec {
public:
  Rec();
  ~Rec();

  /* configurable variables */
  double fbdelay;  /* feedback delay */
  int fbflag;      /* if feedback is played out */
  int indev;       /* input audio device */
  int outdev;      /* output audio device */
  int lmsflag;     /*  */
  //-lmsfiltlen
  int simplexflag; /*  */
  double simplexthreshold; /*  */
  int debug;       /* debug level */
  double asrinputlevel; /* scale audio for asr input */
  int fe:inputrate;    /*  */
  int fe:numfilters;   /* r->fe->numfilters */
  int fe:numceps;      /* r->fe->numceps */
  double fe:lowfreq;   /* (float) r->fe->lowfreq */
  double fe:hifreq;    /* (float) r->fe->hifreq */
  double fe:framestep; /* (float) r->fe->framestep */
  double fe:framelen;  /* (float) r->fe->framelen */
  double fe:preemph;   /* (float) r->fe->preemph */
  double fe:lifter;    /* (float) r->fe->lifter */
  int audiorate; /* audio sampling rate, must be multiple of 8000Hz */
  vector<unsigned char> ann; /* binary buffer used by loadnet */
    /* unsigned char *net; */
    /* //FILE *netstream; */
    /* int n; */
    /* BinaryBuffer *buf; */
    /* DBGPRINTF("before Tcl_ByteArrayFromObj\n"); */
    /* if (objc != 4) { */
    /*   Tcl_WrongNumArgs(interp,1,objv,"configure -ann net"); */
    /*   return TCL_ERROR; */
    /* } */
    /* /\* net should not be freed according to Tcl_GetByteArrayFromObj *\/ */
    /* /\* change this to Tcl_GetByteArrayFromObj() and fmemopen()  */
    /*    look at http://www.codecomments.com/message233491.html *\/ */
    /* net = Tcl_GetByteArrayFromObj(objv[3], &n); */
    /* //Tcl_Eval(interp,"report \"before fmemopen\""); */
    /* //netstream = fmemopen((void *) net, (size_t) n, "r"); */
    /* buf = BinaryBuffer_Create((char *) net,n); */
    /* DBGPRINTF("before LGLoadANN\n"); */
    /* if(r->lg == NULL) DBGPRINTF("r->lg is NULL!!!\n"); */
    /* //if(netstream == NULL) Tcl_Eval(interp,"report netstream is NULL!!!"); */
    /* if(LGLoadANN(r->lg, buf)) return TCL_ERROR; */
    /* //fclose(netstream); */
    /* BinaryBuffer_Free(buf); */
    /* DBGPRINTF("before Recognizer_GetOutSym\n"); */
    /* if(Recognizer_GetOutSym(r)) return TCL_ERROR; */
    /* DBGPRINTF("after Recognizer_GetOutSym\n"); */
    /* if(r->vd != NULL) */
    /*   ViterbiDecoder_SetFrameLen(r->vd, LikelihoodGen_GetOutSize(r->lg)); */
    /* DBGPRINTF("ann is set\n"); */
  vector<double> phprior;
  int vt:lookahead /* Viterbi look-ahead length */
  string decoder /* map or viterbi */
-grammar
    /* public methods */
  string getqueue();
 int getoutpos(); /* SoundSource_GetOutPos(r->s) */
 int getinpos();  /* SoundSource_GetInPos(r->s) */
 double getstreamtime(); /* SoundSource_GetStreamTime(r->s) */
 //getcalibprob
 int start(); /* Configure and start recognizer */
 int stop();  /* Stop recognizer */

 //configure
 //destroy
 double getpeakdb(); /* nrg_ReturnPeakDB(r->s->nrgY,&peakdb); */
 double getxenergy(); /* nrg_ReturnEnergy(r->s->nrgX) */
 double getyenergy(); /* nrg_ReturnEnergy(r->s->nrgY) */
 double getyavenergy(int len); /* nrg_ReturnAvEnergy(r->s->nrgY, len) */
 double getxavenergy(int len); /* nrg_ReturnAvEnergy(r->s->nrgX, len) */
 int getannoutsize(); /* LikelihoodGen_GetOutSize(r->lg) */
 //getlikelihoodvec
 //getfeaturevec
 //grammargen
 //testgrammar
 //grammarduplicate
 //grammarequal
 //dumpviterbiinfo
 //donothing
 //create
  
  private:
 int applysettings(); /* applies all settings before starting the recognizer */
 int loadnet(); /* loads ann into the C recognizer (memory waste)*/
 Recognizer r;
};

#endif // _REC_H_
