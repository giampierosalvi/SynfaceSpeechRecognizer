/***************************************************************************
                            rec.c  -  description
                             -------------------
    begin                : Tue Jan 7 2003
    copyright            : (C) 2003 by Giampiero Salvi
    email                : giampi@kth.se
 ***************************************************************************/

/* Provides a tcl-tk package called rec. The main functions are:
   rec create r
   r start
   r stop
   r destroy
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <tcl.h>
#include "Recognizer.h"
#include "Common.h"

//#include "RTDNN_custom.h" /* to parse net from memory buffer */
// this should be included only for mingw
//#include "fmemopen.h"

//#define REC_VERSION "0.1.8"

/* these are for debug info. The names are defined in Common.h */
Tcl_Channel dbgchan;

/* this holds the recognizer object and the Tcl command id, which
   is used in "delete" to run Tcl_DeleteCommandFromToken */
typedef struct {
  Recognizer *r;
  Tcl_Command commandId;
} TclRec;

/* this is for debugging. Use with care: nobody is freing it in the end */
SparseMatrix *tmptransmat = NULL;


/* Utitlity functions */
float *createFloatArrayFromTclList(Tcl_Interp *interp, Tcl_Obj *list, int *np);
int *createIntArrayFromTclList(Tcl_Interp *interp, Tcl_Obj *list, int *np);
int createMatrixFromTclListList(Tcl_Interp *interp, Tcl_Obj *list, Matrix **res);
int createSparseMatrixFromTclList(Tcl_Interp *interp,Tcl_Obj *list,SparseMatrix **res);
int createVectorFromTclList(Tcl_Interp *interp, Tcl_Obj *list, Vector **res);
int createIntVectorFromTclList(Tcl_Interp *interp, Tcl_Obj *list, IntVector **res);

/* configuration command */
static int ConfigCmd(ClientData cdata, Tcl_Interp *interp,int objc,
		      Tcl_Obj *CONST objv[]) {

  Tcl_HashEntry *hPtr = (Tcl_HashEntry *) cdata;
  TclRec *tr;
  Recognizer *r;
  int l;
  static char *c;
  //char returnInfo[2048];

  tr = (TclRec *) Tcl_GetHashValue(hPtr);
  r = tr->r;

  /*
  if (objc != 4) {
    Tcl_WrongNumArgs(interp,1,objv,"option value");
    return TCL_ERROR;
  }
  */
  c = Tcl_GetStringFromObj(objv[2],&l);
  DBGPRINTF("%s\n",c);
  //l = (int)strlen(c);
  
  /* configure -fbdelay */
  if (strncmp(c,"-fbdelay",l)==0) {
    double delay;
    if(objc == 3) {
      delay = SoundSource_GetPlaybackDelay(r->s);
      Tcl_SetObjResult(interp, Tcl_NewDoubleObj(delay));
      return TCL_OK;
    }
    if(objc == 4) {
      if (Tcl_GetDoubleFromObj(interp,objv[3],&delay) != TCL_OK)
	return TCL_ERROR;
      SoundSource_SetPlaybackDelay(r->s, delay);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -fbdelay [delay]");
    return TCL_ERROR;
  /* configure -fbflag */
  } else if (strncmp(c,"-fbflag",l)==0) {
    if(objc == 3) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(r->s->playback_flag));
      return TCL_OK;
    }
    if(objc == 4) {
      if (Tcl_GetBooleanFromObj(interp,objv[3],&(r->s->playback_flag))!=TCL_OK)
	return TCL_ERROR;
      DBGPRINTF("playback_flag set to %d\n",r->s->playback_flag);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -fbflag [flag]");
    return TCL_ERROR;
  /* configure input device */
  } else if (strncmp(c,"-indev",l)==0) {
    int idx, res;
    if(objc == 3) {
      idx = SoundSource_GetInDevice(r->s);
      Tcl_SetObjResult(interp, Tcl_NewIntObj(idx));
      return TCL_OK;
    }
    if(objc == 4) {
      Tcl_GetIntFromObj(interp, objv[3],&idx);
      res = SoundSource_SetInDevice(r->s, idx);
      if(res) Tcl_SetObjResult(interp, Tcl_NewIntObj(res));
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -indev [idx]");
    return TCL_ERROR;
  } else if (strncmp(c, "-outdev", l)==0) {
    int idx, res;
    if(objc == 3) {
      idx = SoundSource_GetOutDevice(r->s);
      Tcl_SetObjResult(interp, Tcl_NewIntObj(idx));
      return TCL_OK;
    }
    if(objc == 4) {
      Tcl_GetIntFromObj(interp, objv[3],&idx);
      res = SoundSource_SetOutDevice(r->s, idx);
      if(res) Tcl_SetObjResult(interp, Tcl_NewIntObj(res));
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -outdev [idx]");
    return TCL_ERROR;
  /* configure -lmsflag */
  } else if (strncmp(c,"-lmsflag",l)==0) {
    if(objc == 3) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(r->s->lms_flag));
      return TCL_OK;
    }
    if(objc == 4) {
      if (Tcl_GetBooleanFromObj(interp,objv[3],&(r->s->lms_flag)) != TCL_OK)
	return TCL_ERROR;
      DBGPRINTF("lms_flag set to %d\n", r->s->lms_flag);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -lmsflag [flag]");
    return TCL_ERROR;
  /* configure -lmsfiltlen */
    /*  } else if (strncmp(c,"-lmsfiltlen",l)==0) {
    int lmslen = 1;
    if (Tcl_GetIntFromObj(interp,objv[3],&lmslen) != TCL_OK) {
      return TCL_ERROR;
    }
    LMS_Configure(r->s->lms,lmslen,-1);
    DBGPRINTF("lmsfiltlen set to %d\n",lmslen);*/
  /* configure -simplexflag */
  } else if (strncmp(c,"-simplexflag",l)==0) {
    if(objc == 3) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(r->s->simplex_flag));
      return TCL_OK;
    }
    if(objc == 4) {
      if (Tcl_GetBooleanFromObj(interp,objv[3],&(r->s->simplex_flag))!=TCL_OK)
	return TCL_ERROR;
      DBGPRINTF("simplex_flag set to %d\n", r->s->simplex_flag);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -simplexflag [flag]");
    return TCL_ERROR;
  /* configure -simplexthreshold */
  } else if (strncmp(c,"-simplexthreshold",l)==0) {
    if(objc == 3) {
      Tcl_SetObjResult(interp, Tcl_NewDoubleObj(r->s->simplex_threshold));
      return TCL_OK;
    }
    if(objc == 4) {
      if (Tcl_GetDoubleFromObj(interp,objv[3],&(r->s->simplex_threshold))
	  != TCL_OK)
	return TCL_ERROR;
      DBGPRINTF("simplex_threshold = %e\n",r->s->simplex_threshold);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -simplexthreshold [threshold]");
    return TCL_ERROR;
  /* configure -debug */
  } else if (strncmp(c,"-debug", l) == 0) {
    int level;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -debug level");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&level) != TCL_OK) {
      return TCL_ERROR;
    }
    Recognizer_SetDebug(r, level);
  /* configure -asrinputlevel */
  } else if (strncmp(c,"-asrinputlevel", l) == 0) {
    double level;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -asrinputlevel level");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&level) != TCL_OK) {
      return TCL_ERROR;
    }
    /* this is to avoid multiplications if the ratio is close to 1.0 */
    if(abs(level-1.0) < 0.01) level = 1.0;
    r->asr_in_level_ratio = (float) level;
    r->asr_max_input_level = (short) SHRT_MAX/level;
  } else if (strncmp(c,"-fe:inputrate", l) == 0) {
    int rate;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:inputrate rate");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&rate) != TCL_OK) {
      return TCL_ERROR;
    }
    if(rate % 8000 != 0) {
     Tcl_WrongNumArgs(interp,1,objv,"fe:inputrate must be multiple of 8000");
     return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->inputrate = rate;
  } else if (strncmp(c,"-fe:numfilters", l) == 0) {
    int num;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:numfilters num");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&num) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->numfilters = num;
  } else if (strncmp(c,"-fe:numceps", l) == 0) {
    int num;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:numceps num");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&num) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->numceps = num;
  } else if (strncmp(c,"-fe:lowfreq", l) == 0) {
    double freq;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:lowfreq Hz");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&freq) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->lowfreq = (float) freq;
  } else if (strncmp(c,"-fe:hifreq", l) == 0) {
    double freq;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:hifreq Hz");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&freq) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->hifreq = (float) freq;
  } else if (strncmp(c,"-fe:framestep", l) == 0) {
    double step;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:framestep sec");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&step) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->framestep = (float) step;
  } else if (strncmp(c,"-fe:framelen", l) == 0) {
    double len;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:framelen sec");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&len) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->framelen = (float) len;
  } else if (strncmp(c,"-fe:preemph", l) == 0) {
    double preemph;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:preemph float");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&preemph) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->preemph = (float) preemph;
  } else if (strncmp(c,"-fe:lifter", l) == 0) {
    double lifter;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -fe:lifter float");
      return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp,objv[3],&lifter) != TCL_OK) {
      return TCL_ERROR;
    }
    /* assuming that FeatureExtraction was created already */
    r->fe->lifter = (float) lifter;
  /* configure -audiorate */
  } else if (strncmp(c,"-audiorate", l) == 0) {
    int rate;
    if(!r->stopped) {
      return TCL_ERROR;
    }
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -audiorate rate");
      return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp,objv[3],&rate) != TCL_OK) {
      return TCL_ERROR;
    }
    if(rate % 8000 != 0) {
     Tcl_WrongNumArgs(interp,1,objv,"audiorate must be multiple of 8000");
     return TCL_ERROR;
    }
    SoundSource_SetSamplingRate(r->s, rate);
  /* configure -ann */
  } else if (strncmp(c,"-ann", l) == 0) {
    //char *annFn;
    unsigned char *net;
    //FILE *netstream;
    int n;
    BinaryBuffer *buf;
    DBGPRINTF("before Tcl_ByteArrayFromObj\n");
    if (objc != 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -ann net");
      return TCL_ERROR;
    }
    /* net should not be freed according to Tcl_GetByteArrayFromObj */
    /* change this to Tcl_GetByteArrayFromObj() and fmemopen() 
       look at http://www.codecomments.com/message233491.html */
    net = Tcl_GetByteArrayFromObj(objv[3], &n);
    //Tcl_Eval(interp,"report \"before fmemopen\"");
    //netstream = fmemopen((void *) net, (size_t) n, "r");
    buf = BinaryBuffer_Create((char *) net,n);
    DBGPRINTF("before LikelihoodGen_LoadANNFromBuffer\n");
    if(r->lg == NULL) DBGPRINTF("r->lg is NULL!!!\n");
    //if(netstream == NULL) Tcl_Eval(interp,"report netstream is NULL!!!");
    if(LikelihoodGen_LoadANNFromBuffer(r->lg, buf)) return TCL_ERROR;
    //fclose(netstream);
    BinaryBuffer_Free(buf);
    DBGPRINTF("before Recognizer_GetOutSym\n");
    if(Recognizer_GetOutSym(r)) return TCL_ERROR;
    DBGPRINTF("after Recognizer_GetOutSym\n");
    if(r->vd != NULL)
      ViterbiDecoder_SetFrameLen(r->vd, LikelihoodGen_GetOutSize(r->lg));
    DBGPRINTF("ann is set\n");
    /* net should not be freed according to Tcl_GetStringFromObj */
  } else if (strncmp(c,"-calibtemplate", l) == 0) {
    return TCL_OK;
  /*   float *template; */
  /*   int len; */
  /*   if (objc != 4) { */
  /*     Tcl_WrongNumArgs(interp,1,objv,"configure -calibtemplate template"); */
  /*     return TCL_ERROR; */
  /*   } */
  /*   //DBGPRINTF("trying to free template\n"); */
  /*   //Speetures_FreeCalibCheck(r->lg->speetures); */
  /*   DBGPRINTF("loading new calib template\n"); */
  /*   /\* change the next to createFloatArrayFromTclList *\/ */
  /*   template = createFloatArrayFromTclList(interp, objv[3], &len); */
  /*   DBGPRINTF("the template has len %d\n", len); */
  /*   /\* get len, get template*\/ */
  /*   DBGPRINTF("configuring new template\n"); */
  /*   /\* note that Speetures_CalibCheckConf does not free template anymore: */
  /*      it was causing segfault. Perhaps because of ckalloc? Check if it */
  /*      should be freed some other way*\/ */
  /*   Speetures_CalibCheckConf(r->fe->speetures,len,template); */
  } else if (strncmp(c,"-calibthreshold", l) == 0) {
    return TCL_OK;
  /*   double thres; */
  /*   if(objc == 3) { */
  /*     thres = (double) r->fe->speetures->calib_check_thresh; */
  /*     Tcl_SetObjResult(interp, Tcl_NewDoubleObj(thres)); */
  /*     return TCL_OK; */
  /*   } */
  /*   if(objc == 4) { */
  /*     if (Tcl_GetDoubleFromObj(interp,objv[3],&thres) != TCL_OK) */
  /* 	return TCL_ERROR; */
  /*     r->fe->speetures->calib_check_thresh = (float) thres; */
  /*     return TCL_OK; */
  /*   } */
  /*   Tcl_WrongNumArgs(interp,1,objv,"configure -calibthreshold [threshold]"); */
  /*   return TCL_ERROR; */
    /* from vit.c */
  /* configure -phprior */
  } else if (strncmp(c,"-phprior",l)==0) {
    Vector *pp;
    if(objc != 4){
      Tcl_WrongNumArgs(interp,1,objv, "configure -phprior prior");
      return TCL_ERROR;
    }
    createVectorFromTclList(interp, objv[3], &pp);
    LikelihoodGen_SetPhPrior(r->lg,pp);
  /* configure -vt:lookahead */
  } else if (strncmp(c,"-vt:lookahead", l) == 0) {
    int lookahead;
    if(objc == 4) {
      if (Tcl_GetIntFromObj(interp,objv[3],&(lookahead)) != TCL_OK)
	return TCL_ERROR;
      Recognizer_SetLookahead(r,lookahead);
      return TCL_OK;
    }
    Tcl_WrongNumArgs(interp,1,objv,"configure -vt:lookahead [value]");
    return TCL_ERROR;
  /* configure -readintarray debug only */
  } else if (strncmp(c,"-readintarray", l) == 0) {
    IntVector *v;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp, 1, objv, "type");
      return TCL_ERROR;
    }
    createIntVectorFromTclList(interp,objv[3],&v);
    FreeIntVector(&v);
  /* configure -readfloatarray debug only */
  } else if (strncmp(c,"-readfloatarray", l) == 0) {
    Vector *v;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp, 1, objv, "type");
      return TCL_ERROR;
    }
    createVectorFromTclList(interp,objv[3],&v);
    FreeVector(&v);
  } else if (strncmp(c,"-decoder", l) == 0) {
    char *decoder;
    int len;
    if(objc > 4) {
      Tcl_WrongNumArgs(interp,1,objv,"configure -decoder name");
      return TCL_ERROR;
    }
    if(objc == 3) {
      if(r->vd == NULL)
	Tcl_SetObjResult(interp, Tcl_NewStringObj("map", strlen("map")));
      else
	Tcl_SetObjResult(interp, Tcl_NewStringObj("viterbi", strlen("viterbi")));
      return TCL_OK;
    }
    decoder = Tcl_GetStringFromObj(objv[3],&(len));
    if(strcmp(decoder, "viterbi")==0) {
      if(r->vd == NULL) r->vd = ViterbiDecoder_Create();
    } else {
      if(r->vd != NULL) ViterbiDecoder_Free(&r->vd);
    }
    return TCL_OK;
  } else if (strncmp(c,"-grammar", l) == 0) {
    int n;
    SparseMatrix *transmat;
    Vector *stPrior;
    IntVector *fisStateId;
    Tcl_Obj **tmp;
    if (objc != 4) {
      Tcl_WrongNumArgs(interp, 1, objv, "type");
      return TCL_ERROR;
    }

    if(r->vd == NULL) r->vd = ViterbiDecoder_Create();

    Tcl_ListObjGetElements(interp, objv[3], &n, &tmp);
    if(n!=3) {
      DBGPRINTF("panic! expected 3 element list, got %d\n",n);
      return TCL_ERROR;
    }
    if(createSparseMatrixFromTclList(interp,tmp[0],&transmat) == TCL_ERROR)
      return TCL_ERROR;
    if(createVectorFromTclList(interp, tmp[1],&stPrior) == TCL_ERROR)
      return TCL_ERROR;
    if(createIntVectorFromTclList(interp, tmp[2],&fisStateId) == TCL_ERROR)
      return TCL_ERROR;
    /* This frees the old grammar in case */
    ViterbiDecoder_SetGrammar(r->vd,transmat,stPrior,fisStateId);
    /* This shouldn't be done every time we change the grammar, but it's
       easier this way */
    ViterbiDecoder_SetFrameLen(r->vd, LikelihoodGen_GetOutSize(r->lg));

    /* diagnostics */
    /* FILE *fh = fopen("/tmp/synface_dyagnostics.txt", "w"); */
    /* int i,k; */
    /* for(i=0;i<r->vd->nTrans;i++) { */
    /*   int fs = r->vd->fisStateId->data[i]; */
    /*   for(k=0; k<r->vd->transmat->nels[i];k++) { */
    /* 	//fprintf(fh, "st idx: %d ph idx: %d ph str: %s #out: %d\n", */
    /* 	//      i, fs, r->ph[fs], r->vd->transmat->nels[i]); */
    /* 	fprintf(fh, "%d %d\n", i, r->vd->transmat->idxs[i][k]); */
    /*   } */
    /* } */
    /* fclose(fh); */
  /* configure -nothing (used for debugging) */
  } else if (strncmp(c,"-nothing", l) == 0) {
  } else {
	  Tcl_AppendResult(interp,"Unknown option: \"",c,"\"",NULL);
	  return TCL_ERROR;
  }
  return TCL_OK;
}


/* contains all the methods assigned to the recognizer object */
int
recObjCmd(ClientData cdata, Tcl_Interp *interp, int objc,
        Tcl_Obj *CONST objv[])
{
  Tcl_HashEntry *hPtr = (Tcl_HashEntry *) cdata;
  TclRec *tr;
  Recognizer *r;
  int len;
  //int i;
  const char *str;
  //char returnInfo[2048];

  tr = (TclRec *) Tcl_GetHashValue(hPtr);
  r = tr->r;
  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "type");
    return TCL_ERROR;
  }
  str = Tcl_GetStringFromObj(objv[1], &len);
  //DBGPRINTF("%s\n",str);

  if (strncmp("getqueue", str, len) == 0) {
    int res_idx, res_frame_time;
    double res_playback_time;
    Tcl_Obj *tclList = Tcl_NewListObj(0, NULL);
    while(Recognizer_GetResult(r, &res_idx, &res_frame_time, &res_playback_time)) {
      Tcl_Obj *ph = Tcl_NewStringObj(r->ph[res_idx], strlen(r->ph[res_idx]));
      Tcl_Obj *frame_time = Tcl_NewIntObj(res_frame_time);
      Tcl_Obj *playback_time = Tcl_NewDoubleObj(res_playback_time);
      //Tcl_Obj *calibprob = Tcl_NewDoubleObj(res->calibprob);
      Tcl_Obj *tclLine = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, tclLine, ph); // label
      Tcl_ListObjAppendElement(interp, tclLine, frame_time); // time
      Tcl_ListObjAppendElement(interp, tclLine, playback_time); // time
      //Tcl_ListObjAppendElement(interp, tclLine, calibprob); // calibration
      Tcl_ListObjAppendElement(interp, tclList, tclLine);
    }
    Tcl_SetObjResult(interp, tclList);
  } else if (strncmp("getoutpos", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewIntObj(SoundSource_GetOutPos(r->s));
    Tcl_SetObjResult(interp, res);
  } else if (strncmp("getinpos", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewIntObj(SoundSource_GetInPos(r->s));
    Tcl_SetObjResult(interp, res);
  } else if (strncmp("getstreamtime", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewDoubleObj(SoundSource_GetStreamTime(r->s));
    Tcl_SetObjResult(interp, res);
  } else if (strncmp("getcalibprob", str, len) == 0) {
    //Tcl_Obj *res = Tcl_NewDoubleObj(Speetures_GetCalibProb(r->fe->speetures));
    Tcl_Obj *res = Tcl_NewDoubleObj(0.0);
    Tcl_SetObjResult(interp, res);
    /* Here were getindevs, getoutdevs and getdevinfo now in Sound.cxx */
  } else if (strncmp("start", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewIntObj(Recognizer_Start(r));
    Tcl_SetObjResult(interp, res);
    return TCL_OK;
  } else if (strncmp("stop", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewIntObj(Recognizer_Stop(r));
    Tcl_SetObjResult(interp, res);
    return TCL_OK;
  } else if (strncmp("configure", str, len) == 0) {
    return ConfigCmd(cdata,interp,objc,objv);
  } else if (strncmp("destroy", str, len) == 0) {
      if(Recognizer_Free(&r)!=0) return TCL_ERROR;
      DBGPRINTF("after Recognizer_Free\n");
      Tcl_DeleteCommandFromToken(interp, tr->commandId);
      Tcl_DeleteHashEntry(hPtr);
      ckfree((char *)tr);
      DBGPRINTF("end of destroy procedure\n");
  } else if (strncmp("getpeakdb", str, len) == 0) {
    /* this works on y-channel, which is the only one in mono mode */
    double peakdb;
    nrg_ReturnPeakDB(r->s->nrgY,&peakdb);
    //    printf("peakdb=%lf\n",peakdb);
    Tcl_Obj *e = Tcl_NewDoubleObj(peakdb);
    Tcl_SetObjResult(interp, e);
    return TCL_OK;
    
  } else if (strncmp("getxenergy", str, len) == 0) {
    Tcl_Obj *e = Tcl_NewDoubleObj(nrg_ReturnEnergy(r->s->nrgX));
    Tcl_SetObjResult(interp, e);
    /* note that:
       nrgY <-> channel 0 (mix of local and remote voices)
       nrgX <-> channel 1 (local voice) */
  } else if (strncmp("getyenergy", str, len) == 0) {
    Tcl_Obj *e = Tcl_NewDoubleObj(nrg_ReturnEnergy(r->s->nrgY));
    Tcl_SetObjResult(interp, e);
  } else if (strncmp("getyavenergy", str, len) == 0) {
    int n; Tcl_Obj *e;
    if (objc < 3) return TCL_ERROR;
    Tcl_GetIntFromObj(interp, objv[2], &n);
    e = Tcl_NewDoubleObj(nrg_ReturnAvEnergy(r->s->nrgY, n));
    Tcl_SetObjResult(interp, e);
  } else if (strncmp("getxavenergy", str, len) == 0) {
    int n; Tcl_Obj *e;
    if (objc < 3 || r->s->inputParameters.channelCount < 2) return TCL_ERROR;
    Tcl_GetIntFromObj(interp, objv[2], &n);
    e = Tcl_NewDoubleObj(nrg_ReturnAvEnergy(r->s->nrgX, n));
    Tcl_SetObjResult(interp, e);
  } else if (strncmp("update", str, len) == 0) {
    // rewrite this function, if needed
    //Tcl_SetResult(interp,LGConsumeFrame(r->lg), (char *) NULL);
  } else if (strncmp("check", str, len) == 0) {
    //DBGPRINTF("overlap = %d",CheckOverlap());
    DBGPRINTF("overlap check is not supported anymore");
  } else if (strncmp("getannoutsize", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewIntObj(LikelihoodGen_GetOutSize(r->lg));
    Tcl_SetObjResult(interp, res);
  } else if (strncmp("getlikelihoodvec", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewListObj(0, NULL);
    int i;
    double el;
    for(i=0;i<LikelihoodGen_GetOutSize(r->lg);i++) {
      el = (double) r->lg->lh[i];
      Tcl_ListObjAppendElement(interp, res, Tcl_NewDoubleObj(el));
    }
    Tcl_SetObjResult(interp, res);
    return TCL_OK;
  } else if (strncmp("getfeaturevec", str, len) == 0) {
    Tcl_Obj *res = Tcl_NewListObj(0, NULL);
    int i;
    double el;
    //if(r->stopped || r->lg->features==NULL) {
    //  Tcl_SetObjResult(interp, res);
    //  return TCL_OK;
    //}
    for(i=0;i<LikelihoodGen_GetInSize(r->lg);i++) {
      el = (double) r->lg->sim->input_stream[0][i];
      Tcl_ListObjAppendElement(interp, res, Tcl_NewDoubleObj(el));
    }
    Tcl_SetObjResult(interp, res);
    return TCL_OK;
  } else if (strncmp("grammargen", str, len) == 0) {
    /* generate random paths from the transition matrix.
       arguments: start_state num_steps
       returns:list of steps (state phone_number phone_symbol) */
    int n, i=0, state, from, fromidx, to, ntos, tolist[r->vd->nTrans], toidx;
    float rnd;
    Tcl_Obj *tclList = Tcl_NewListObj(0, NULL);
    if (objc < 4) return TCL_ERROR;
    Tcl_GetIntFromObj(interp, objv[2], &state);
    Tcl_GetIntFromObj(interp, objv[3], &n);
    if(state>=r->vd->nTrans) state=0;
    while(i<n) {
      int fisstate = r->vd->fisStateId->data[state];
      Tcl_Obj *tcl_state = Tcl_NewIntObj(state);
      Tcl_Obj *tcl_fisstate = Tcl_NewIntObj(fisstate);
      Tcl_Obj *ph = Tcl_NewStringObj(r->ph[fisstate], strlen(r->ph[fisstate]));
      Tcl_Obj *tclLine = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, tclLine, tcl_state);
      Tcl_ListObjAppendElement(interp, tclLine, tcl_fisstate);
      Tcl_ListObjAppendElement(interp, tclLine, ph);
      Tcl_ListObjAppendElement(interp, tclList, tclLine);
      /* get a list of possible successors */
      ntos = 0;
      for(to=0;to<r->vd->nTrans;to++) {
	for(fromidx=0; fromidx<r->vd->transmat->nels[to]; fromidx++) {
	  if(r->vd->transmat->idxs[to][fromidx]==state) {
	    tolist[ntos] = to;
	    ntos++;
	  }
	}
      }
      /* generate new random index */
      rnd = (float)rand();
      rnd *= ntos;
      rnd /= RAND_MAX;
      toidx = (int) rnd;
      state = tolist[toidx];
      i++;
    }
    Tcl_SetObjResult(interp, tclList);
    return TCL_OK;
  } else if (strncmp("testgrammar", str, len) == 0) {
    int framelen = LikelihoodGen_GetOutSize(r->lg);
    int i,j,idx=0;
    int res_idx, res_frame_time;
    float res_playback_time;
    Tcl_Obj *tclList = Tcl_NewListObj(0, NULL);
    //r->vd->hasPrior = 1;
    DBGPRINTF("entering testgrammar\n"); 
    for(i=0;i<3*framelen;i++) {
      for(j=0;j<framelen;j++) r->lg->lh[j]= 0.1;
      //idx = rand()/(((double)RAND_MAX + 1) / framelen);
      idx = framelen-i%framelen-1;
      r->lg->lh[idx] = 1.0;
      for(j=0;j<10;j++) {
	DBGPRINTF("[%d,%d,%d]\n", i,j,idx);
	Recognizer_ConsumeFrame(r);
	while(Recognizer_GetResult(r, &res_idx, &res_frame_time, &res_playback_time)) {
	  Tcl_Obj *ph = Tcl_NewStringObj(r->ph[res_idx], strlen(r->ph[res_idx]));
	  Tcl_Obj *frame_time = Tcl_NewIntObj(res_frame_time);
	  Tcl_Obj *playback_time = Tcl_NewDoubleObj(res_playback_time);
	  //Tcl_Obj *calibprob = Tcl_NewDoubleObj(res->calibprob);
	  Tcl_Obj *tclLine = Tcl_NewListObj(0, NULL);
	  Tcl_ListObjAppendElement(interp, tclLine, ph); // label
	  Tcl_ListObjAppendElement(interp, tclLine, frame_time); // time
	  Tcl_ListObjAppendElement(interp, tclLine, playback_time); // time
	  //Tcl_ListObjAppendElement(interp, tclLine, calibprob); // calibration
	  Tcl_ListObjAppendElement(interp, tclList, tclLine);
	}
      }
    }
    Tcl_SetObjResult(interp, tclList);
    return TCL_OK;
  } else if (strncmp("grammarduplicate", str, len) == 0) {
    if(tmptransmat != NULL) FreeSparseMatrix(&tmptransmat);
    tmptransmat = DuplicateSparseMatrix(r->vd->transmat);
    return TCL_OK;
  } else if (strncmp("grammarequal", str, len) == 0) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(SparseMatrixEqual(r->vd->transmat, tmptransmat)));
  } else if (strncmp("dumpviterbiinfo", str, len) == 0) {
    ViterbiDecoder_DumpModel(r->vd);
    ViterbiDecoder_DumpState(r->vd);
    return TCL_OK;
  } else if (strncmp("donothing", str, len) == 0) {
  } else {
    Tcl_AppendResult(interp, "bad option \"", str,
		     "\": must be start, stop or destroy...", (char *) NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/* this creates the object and the object command */

int
Rec_CreateCmd(ClientData cdata, Tcl_Interp *interp, int objc,
        Tcl_Obj *CONST objv[])
{
  Tcl_HashTable *hTab = (Tcl_HashTable *) cdata;
  Tcl_HashEntry *hPtr;
  int flag;
  int len;
  const char *str;
  char *name; /* remove this as it is not needed */
  //char *netFileName;
  int asrSmpRate = 8000; /* now this is configurable with -asrinputrate */
  TclRec *tr;
  //  Recognizer *r;
  //char returnInfo[2048];

  DBGPRINTF("entering recCreateCmd...\n");

  if (objc < 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "name");
    return TCL_ERROR;
  }
  str = Tcl_GetStringFromObj(objv[1], &len);
  if(strncmp("create", str, len)) {
    Tcl_AppendResult(interp, "The format is\n rec create <name>", str, NULL);
    return TCL_ERROR;
  }

  tr = (TclRec *) ckalloc(sizeof(TclRec));

  DBGPRINTF("%s\n",str);

  /* create the recognizer */
  str = Tcl_GetStringFromObj(objv[2], &len);
  name = (char *) str;
  hPtr = Tcl_FindHashEntry(hTab, name);
  if(hPtr != NULL) {
    Tcl_AppendResult(interp, "The object already exists ", str, NULL);
    return TCL_ERROR;
  }

  /* this creates LikelihoodGen as well as SoundSource */
  if ((tr->r = Recognizer_Create()) == NULL)
    return TCL_ERROR;
  
  /* just for debugging */
  //LikelihoodGen_GetTclInterp(tr->r->lg, interp);

  if(objc > 3) { /* process input arguments */
    Tcl_WrongNumArgs(interp, 1, objv, "name");
    return TCL_ERROR;
  }

  hPtr = Tcl_CreateHashEntry(hTab, name, &flag);
  Tcl_SetHashValue(hPtr, (ClientData) tr); /* check this */
  tr->commandId = Tcl_CreateObjCommand(interp, name, recObjCmd,
				       (ClientData) hPtr,
		       (Tcl_CmdDeleteProc *) NULL);  /* and this */
  Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));

  return TCL_OK;
}



/* this is run when calling "package require rec" */
int DLLEXPORT Rec_Init(Tcl_Interp *interp) {
  //ClientData *cd;
  Tcl_HashTable *recHashTable;

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8", 0) == NULL) {
    return TCL_ERROR;
  }
#endif
  dbgchan = Tcl_GetStdChannel(TCL_STDERR);

  if(Tcl_PkgProvideEx(interp, "rec", REC_VERSION,
		      (ClientData) NULL) != TCL_OK) {
    return TCL_ERROR;
  }

  recHashTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));

  Tcl_CreateObjCommand(interp, "rec",
		       (Tcl_ObjCmdProc*) Rec_CreateCmd,
		       (ClientData) recHashTable, (Tcl_CmdDeleteProc *)NULL);
  
  Tcl_InitHashTable(recHashTable, TCL_STRING_KEYS);

  return TCL_OK;
}

/* these functions are used to pass structures from tcl to C */

int *createIntArrayFromTclList(Tcl_Interp *interp, Tcl_Obj *list, int *np) {
  Tcl_Obj **objPtrArray;
  int tmp, *res;
  int i;

  if(Tcl_ListObjGetElements(interp, list, np, &objPtrArray)==TCL_ERROR)
    return NULL;
  res = (int *) ckalloc((*np)*sizeof(int));
  for(i=0;i<(*np);i++) {
    if(Tcl_GetIntFromObj(interp, objPtrArray[i], &tmp)==TCL_ERROR)
      return NULL;
    //DBGPRINTF("%d ",tmp);
    res[i] = tmp;
  }
  return res;
}

/* note that the elements can contain the string Inf as
   well as floats */
float *createFloatArrayFromTclList(Tcl_Interp *interp, Tcl_Obj *list, int *np) {
  Tcl_Obj **objPtrArray;
  char *check;
  double tmp;
  float *res;
  int i, len;

  Tcl_ListObjGetElements(interp, list, np, &objPtrArray);
  res = (float *) ckalloc((*np)*sizeof(float));
  for(i=0;i<(*np);i++) {
    /* quite funny that this one has a totally different syntax */
    check = Tcl_GetStringFromObj(objPtrArray[i], &len);
    if(strcmp(check,"Ninf") == 0) res[i] = NINF;
    else {
      Tcl_GetDoubleFromObj(interp, objPtrArray[i], &tmp);
      res[i] = (float) tmp;
    }
  }

  return res;
}


/* note that the elements can contain the string Inf as
   well as floats */
int createMatrixFromTclListList(Tcl_Interp *interp,
					Tcl_Obj *list, Matrix **resp) {
  Tcl_Obj **objPtrArray;
  Tcl_Obj **objPtrArray2;
  Matrix *res;
  int n,m;
  //char *check;
  double tmp;
  int i, j; // len;

  Tcl_ListObjGetElements(interp, list, &n, &objPtrArray);
  res = (Matrix *) ckalloc(sizeof(Matrix));
  res->nrows = n;
  Tcl_ListObjLength(interp, objPtrArray[0], &m);
  res->ncols = m;
  res->data = (float **) ckalloc(n*sizeof(float *));
  for(i=0;i<n;i++) {
    res->data[i] = (float *) ckalloc(res->ncols*sizeof(float));
    Tcl_ListObjGetElements(interp, objPtrArray[i], &m, &objPtrArray2);
    if(m!=res->ncols) return TCL_ERROR;
    for(j=0;j<m;j++) {
      Tcl_GetDoubleFromObj(interp, objPtrArray2[j], &tmp);
      res->data[i][j] = (float) tmp;
    }
  }

  *resp = res;
  return 0;
}

/* remember that the indexes have to be in C style already:
   they are converted in the Tcl part */
int createSparseMatrixFromTclList(Tcl_Interp *interp,
				  Tcl_Obj *list, SparseMatrix **res) {
  int n;
  Tcl_Obj **tmp;
  int *from, *to, *type;
  float *weight;

  /* note that tmp should not be freed (managed by Tcl )*/
  Tcl_ListObjGetElements(interp, list, &n, &tmp);
  if(n!=4) return TCL_ERROR;
  from   = createIntArrayFromTclList(interp, tmp[0], &n);
  to     = createIntArrayFromTclList(interp, tmp[1], &n);
  weight = createFloatArrayFromTclList(interp, tmp[2], &n);
  type   = createIntArrayFromTclList(interp, tmp[3], &n);
  *res = CreateSparseMatrix(from, to, weight, type, n);
  ckfree((char *)from); ckfree((char *)to);
  ckfree((char *)weight); ckfree((char *)type);    
  return 0;
}

int createVectorFromTclList(Tcl_Interp *interp, Tcl_Obj *list, Vector **res) {
  float *tmp;
  int n;

  tmp = createFloatArrayFromTclList(interp, list, &n);
  *res = CreateVector(tmp,n);
  ckfree((char *)tmp);
  return 0;
}

int createIntVectorFromTclList(Tcl_Interp *interp, Tcl_Obj *list, IntVector **res) {
  int *tmp,n;

  tmp = createIntArrayFromTclList(interp, list, &n);
  *res = CreateIntVector(tmp,n);
  ckfree((char *)tmp);
  return 0;
}

